#include "Lock.h"

// 使用 Windows CNG（bcrypt）实现加密与哈希，避免引入第三方加密库依赖。
// 需要链接 bcrypt.lib（加解密/哈希）与 crypt32.lib（Base64 编解码）。
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>

namespace
{
    // 密钥长度：AES-256 固定 32 字节
    constexpr size_t kKeySize = 32;
    // GCM 认证标签长度：16 字节足以满足完整性校验需求
    constexpr size_t kTagSize = 16;
    // GCM 推荐的非重复 IV 长度
    constexpr size_t kIvSize = 12;
    // 加密文件头部魔数，用于在解密时快速识别文件格式
    constexpr char kFileMagic[4] = { 'L', 'K', '0', '1' };

    /// <summary>
    /// 打开指定算法提供程序句柄，并在离开作用域时自动关闭，避免资源泄漏。
    /// </summary>
    class AlgHandle
    {
    public:
        /// <summary>
        /// 按算法 ID 打开 CNG 算法句柄。
        /// </summary>
        /// <param name="algId">BCrypt 算法标识，如 BCRYPT_AES_ALGORITHM。</param>
        explicit AlgHandle(LPCWSTR algId)
        {
            if (BCryptOpenAlgorithmProvider(&handle, algId, nullptr, 0) < 0)
            {
                handle = nullptr;
            }
        }

        ~AlgHandle()
        {
            if (handle != nullptr)
            {
                BCryptCloseAlgorithmProvider(handle, 0);
            }
        }

        // 句柄不可复制，避免重复释放
        AlgHandle(const AlgHandle&) = delete;
        AlgHandle& operator=(const AlgHandle&) = delete;

        /// <summary>底层 BCrypt 算法句柄；打开失败时为 nullptr。</summary>
        BCRYPT_ALG_HANDLE Get() const { return handle; }

        /// <summary>算法句柄是否可用。</summary>
        bool IsValid() const { return handle != nullptr; }

    private:
        BCRYPT_ALG_HANDLE handle = nullptr;
    };

    /// <summary>
    /// 读取文件的全部字节内容。
    /// </summary>
    /// <param name="path">文件路径。</param>
    /// <param name="out">读取结果。</param>
    /// <returns>成功返回 true。</returns>
    bool ReadAllBytes(const std::wstring& path, std::vector<uint8_t>& out)
    {
        HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file, &size) || size.QuadPart < 0)
        {
            CloseHandle(file);
            return false;
        }

        out.resize(static_cast<size_t>(size.QuadPart));
        size_t readTotal = 0;
        while (readTotal < out.size())
        {
            DWORD chunk = static_cast<DWORD>(std::min<size_t>(out.size() - readTotal, 1u << 20));
            DWORD read = 0;
            if (!ReadFile(file, out.data() + readTotal, chunk, &read, nullptr) || read == 0)
            {
                CloseHandle(file);
                return false;
            }
            readTotal += read;
        }

        CloseHandle(file);
        return true;
    }

    /// <summary>
    /// 将字节流整体写入文件，已存在则截断覆盖。
    /// </summary>
    /// <param name="path">文件路径。</param>
    /// <param name="data">待写入的数据。</param>
    /// <returns>成功返回 true。</returns>
    bool WriteAllBytes(const std::wstring& path, const std::vector<uint8_t>& data)
    {
        HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        size_t writtenTotal = 0;
        while (writtenTotal < data.size())
        {
            DWORD chunk = static_cast<DWORD>(std::min<size_t>(data.size() - writtenTotal, 1u << 20));
            DWORD written = 0;
            if (!WriteFile(file, data.data() + writtenTotal, chunk, &written, nullptr) || written == 0)
            {
                CloseHandle(file);
                return false;
            }
            writtenTotal += written;
        }

        CloseHandle(file);
        return true;
    }

    /// <summary>
    /// 获取算法对象的属性值（如 GCM 认证标签长度上限）。
    /// </summary>
    /// <param name="alg">已打开的算法句柄。</param>
    /// <param name="prop">属性名，如 BCRYPT_AUTH_TAG_LENGTH。</param>
    /// <param name="out">输出缓冲区，大小需与属性类型匹配。</param>
    /// <param name="outSize">输出缓冲区字节数。</param>
    /// <returns>成功返回 true。</returns>
    bool GetAlgProperty(BCRYPT_ALG_HANDLE alg, LPCWSTR prop, void* out, ULONG outSize)
    {
        ULONG result = 0;
        NTSTATUS status = BCryptGetProperty(alg, prop,
            static_cast<PUCHAR>(out), outSize, &result, 0);
        return status >= 0;
    }

    /// <summary>
    /// 创建可复用的 AES 对称密钥句柄（含 GCM 链式模式设置）。
    /// </summary>
    /// <param name="alg">已打开的 AES 算法句柄。</param>
    /// <param name="key">32 字节密钥。</param>
    /// <returns>密钥句柄；创建失败返回 nullptr，调用方负责 BCryptDestroyKey。</returns>
    BCRYPT_KEY_HANDLE CreateAesKey(BCRYPT_ALG_HANDLE alg, const std::string& key)
    {
        BCRYPT_KEY_HANDLE keyHandle = nullptr;
        ULONG objectSize = 0, result = 0;
        if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &result, 0) < 0)
        {
            return nullptr;
        }

        // 密钥对象缓冲区由 CNG 内部使用，必须在密钥句柄生命周期内保持有效
        thread_local std::vector<uint8_t> keyObject;
        keyObject.resize(objectSize);

        if (BCryptGenerateSymmetricKey(alg, &keyHandle, keyObject.data(), objectSize,
            reinterpret_cast<PUCHAR>(const_cast<char*>(key.data())),
            static_cast<ULONG>(key.size()), 0) < 0)
        {
            return nullptr;
        }
        return keyHandle;
    }

    /// <summary>
    /// GCM 单次调用加解密的核心实现。
    /// </summary>
    /// <param name="sealed">
    /// 加密时为明文输入；解密时为 iv + tag + cipherText 组合输入。
    /// </param>
    /// <param name="key">32 字节密钥。</param>
    /// <param name="encrypt">true 表示加密，false 表示解密。</param>
    /// <returns>加密返回 iv+tag+cipher；解密返回明文；失败返回空 vector。</returns>
    std::vector<uint8_t> CryptGcm(const std::vector<uint8_t>& sealed, const std::string& key, bool encrypt)
    {
        if (key.size() != kKeySize)
        {
            return {};
        }

        // 解密时输入至少要包含 iv 与 tag
        if (!encrypt && sealed.size() < kIvSize + kTagSize)
        {
            return {};
        }

        AlgHandle alg(BCRYPT_AES_ALGORITHM);
        if (!alg.IsValid())
        {
            return {};
        }

        // 显式设置为 GCM 链式模式
        if (BCryptSetProperty(alg.Get(), BCRYPT_CHAINING_MODE,
            reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)),
            sizeof(BCRYPT_CHAIN_MODE_GCM), 0) < 0)
        {
            return {};
        }

        // 确认当前系统支持所需的认证标签长度
        BCRYPT_AUTH_TAG_LENGTHS_STRUCT tagLengths{};
        if (!GetAlgProperty(alg.Get(), BCRYPT_AUTH_TAG_LENGTH, &tagLengths, sizeof(tagLengths)))
        {
            return {};
        }
        if (kTagSize < tagLengths.dwMinLength || kTagSize > tagLengths.dwMaxLength)
        {
            return {};
        }

        BCRYPT_KEY_HANDLE keyHandle = CreateAesKey(alg.Get(), key);
        if (keyHandle == nullptr)
        {
            return {};
        }

        std::vector<uint8_t> iv(kIvSize, 0);
        std::vector<uint8_t> tag(kTagSize, 0);
        const uint8_t* inputPtr = nullptr;
        size_t inputSize = 0;

        if (encrypt)
        {
            // 每次加密都使用全新随机 IV，防止相同明文产生相同密文
            if (BCryptGenRandom(nullptr, iv.data(), static_cast<ULONG>(iv.size()),
                BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
            {
                BCryptDestroyKey(keyHandle);
                return {};
            }
            inputPtr = sealed.data();
            inputSize = sealed.size();
        }
        else
        {
            // 解密时从输入头部拆出 iv 与 tag
            memcpy(iv.data(), sealed.data(), kIvSize);
            memcpy(tag.data(), sealed.data() + kIvSize, kTagSize);
            inputPtr = sealed.data() + kIvSize + kTagSize;
            inputSize = sealed.size() - kIvSize - kTagSize;
        }

        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
        BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
        authInfo.pbNonce = iv.data();
        authInfo.cbNonce = static_cast<ULONG>(iv.size());
        authInfo.pbTag = tag.data();
        authInfo.cbTag = static_cast<ULONG>(tag.size());

        std::vector<uint8_t> output(inputSize, 0);
        ULONG produced = 0;

        NTSTATUS status;
        if (encrypt)
        {
            status = BCryptEncrypt(keyHandle, const_cast<PUCHAR>(inputPtr),
                static_cast<ULONG>(inputSize), &authInfo, nullptr, 0,
                output.data(), static_cast<ULONG>(output.size()), &produced, 0);
        }
        else
        {
            status = BCryptDecrypt(keyHandle, const_cast<PUCHAR>(inputPtr),
                static_cast<ULONG>(inputSize), &authInfo, nullptr, 0,
                output.data(), static_cast<ULONG>(output.size()), &produced, 0);
        }

        BCryptDestroyKey(keyHandle);

        // 解密失败通常意味着密钥错误或密文被篡改
        if (status < 0)
        {
            return {};
        }
        output.resize(produced);

        if (encrypt)
        {
            // 输出布局：iv(12) + tag(16) + cipherText
            std::vector<uint8_t> result;
            result.reserve(kIvSize + kTagSize + output.size());
            result.insert(result.end(), iv.begin(), iv.end());
            result.insert(result.end(), tag.begin(), tag.end());
            result.insert(result.end(), output.begin(), output.end());
            return result;
        }

        return output;
    }

    /// <summary>
    /// 使用口令的 SHA-256 摘要作为 AES 密钥。
    /// </summary>
    /// <param name="password">用户口令。</param>
    /// <returns>32 字节密钥。</returns>
    std::string DeriveKeyFromBytes(const std::vector<uint8_t>& data)
    {
        std::vector<uint8_t> digest = Lock::Hash::SHA256(data);
        return std::string(digest.begin(), digest.end());
    }

    /// <summary>
    /// 将宽字符串按 UTF-8 编码为字节流。
    /// </summary>
    /// <param name="text">宽字符文本。</param>
    /// <returns>UTF-8 字节流；转换失败返回空 vector。</returns>
    std::vector<uint8_t> WideToUtf8Bytes(const std::wstring& text)
    {
        if (text.empty())
        {
            return {};
        }
        int need = WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
            static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (need <= 0)
        {
            return {};
        }
        std::string buffer(static_cast<size_t>(need), '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
            buffer.data(), need, nullptr, nullptr);
        return std::vector<uint8_t>(buffer.begin(), buffer.end());
    }

    /// <summary>
    /// 将 UTF-8 字节流解码为宽字符串。
    /// </summary>
    /// <param name="data">UTF-8 字节流。</param>
    /// <returns>宽字符文本；转换失败返回空串。</returns>
    std::wstring Utf8BytesToWide(const std::vector<uint8_t>& data)
    {
        if (data.empty())
        {
            return {};
        }
        const char* raw = reinterpret_cast<const char*>(data.data());
        int need = MultiByteToWideChar(CP_UTF8, 0, raw, static_cast<int>(data.size()), nullptr, 0);
        if (need <= 0)
        {
            return {};
        }
        std::wstring buffer(static_cast<size_t>(need), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, raw, static_cast<int>(data.size()),
            buffer.data(), need);
        return buffer;
    }
}

namespace Lock
{
    namespace AES
    {
        std::vector<uint8_t> Encrypt(const std::vector<uint8_t>& plain, const std::string& key)
        {
            return CryptGcm(plain, key, true);
        }

        std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& sealed, const std::string& key)
        {
            return CryptGcm(sealed, key, false);
        }

        bool EncryptFile(const std::wstring& srcPath, const std::wstring& dstPath, const std::string& key)
        {
            std::vector<uint8_t> plain;
            if (!ReadAllBytes(srcPath, plain))
            {
                return false;
            }

            std::vector<uint8_t> cipher = Encrypt(plain, key);
            if (cipher.empty() && !plain.empty())
            {
                return false;
            }

            // 空文件也会生成 iv + tag 头部，因此密文始终非空
            std::vector<uint8_t> output;
            output.reserve(sizeof(kFileMagic) + cipher.size());
            output.insert(output.end(), kFileMagic, kFileMagic + sizeof(kFileMagic));
            output.insert(output.end(), cipher.begin(), cipher.end());

            return WriteAllBytes(dstPath, output);
        }

        bool DecryptFile(const std::wstring& srcPath, const std::wstring& dstPath, const std::string& key)
        {
            std::vector<uint8_t> raw;
            if (!ReadAllBytes(srcPath, raw))
            {
                return false;
            }

            if (raw.size() < sizeof(kFileMagic) ||
                memcmp(raw.data(), kFileMagic, sizeof(kFileMagic)) != 0)
            {
                return false;
            }

            std::vector<uint8_t> sealed(raw.begin() + sizeof(kFileMagic), raw.end());
            std::vector<uint8_t> plain = Decrypt(sealed, key);
            if (plain.empty() && !sealed.empty())
            {
                return false;
            }

            return WriteAllBytes(dstPath, plain);
        }

        bool EncryptFileInPlace(const std::wstring& path, const std::string& key)
        {
            // 先写临时文件再替换，防止加密中途失败破坏原始数据
            std::wstring temp = path + L".locktmp";
            if (!EncryptFile(path, temp, key))
            {
                DeleteFileW(temp.c_str());
                return false;
            }

            if (!MoveFileExW(temp.c_str(), path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            {
                DeleteFileW(temp.c_str());
                return false;
            }
            return true;
        }

        bool DecryptFileInPlace(const std::wstring& path, const std::string& key)
        {
            std::wstring temp = path + L".unlocktmp";
            if (!DecryptFile(path, temp, key))
            {
                DeleteFileW(temp.c_str());
                return false;
            }

            if (!MoveFileExW(temp.c_str(), path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            {
                DeleteFileW(temp.c_str());
                return false;
            }
            return true;
        }
    }

    namespace String
    {
        std::string Encrypt(const std::string& plain, const std::string& key)
        {
            std::vector<uint8_t> input(plain.begin(), plain.end());
            std::vector<uint8_t> cipher = AES::Encrypt(input, key);
            if (cipher.empty())
            {
                return {};
            }
            return Hash::Base64Encode(cipher);
        }

        std::string Decrypt(const std::string& cipherBase64, const std::string& key)
        {
            std::vector<uint8_t> sealed = Hash::Base64Decode(cipherBase64);
            if (sealed.empty())
            {
                return {};
            }
            std::vector<uint8_t> plain = AES::Decrypt(sealed, key);
            if (plain.empty() && !sealed.empty())
            {
                return {};
            }
            return std::string(plain.begin(), plain.end());
        }

        std::string Encrypt(const std::wstring& plain, const std::string& key)
        {
            // 宽字符使用 UTF-8 作为中间编码，保证跨平台/跨进程一致
            std::vector<uint8_t> input = WideToUtf8Bytes(plain);
            std::vector<uint8_t> cipher = AES::Encrypt(input, key);
            if (cipher.empty())
            {
                return {};
            }
            return Hash::Base64Encode(cipher);
        }

        std::wstring DecryptW(const std::string& cipherBase64, const std::string& key)
        {
            std::vector<uint8_t> sealed = Hash::Base64Decode(cipherBase64);
            if (sealed.empty())
            {
                return {};
            }
            std::vector<uint8_t> plain = AES::Decrypt(sealed, key);
            if (plain.empty() && !sealed.empty())
            {
                return {};
            }
            return Utf8BytesToWide(plain);
        }

        bool IsValidKey(const std::string& key)
        {
            return key.size() == kKeySize;
        }

        std::string DeriveKey(const std::string& password)
        {
            std::vector<uint8_t> data(password.begin(), password.end());
            return DeriveKeyFromBytes(data);
        }

        std::string DeriveKey(const std::wstring& password)
        {
            return DeriveKeyFromBytes(WideToUtf8Bytes(password));
        }
    }

    namespace Hash
    {
        std::vector<uint8_t> SHA256(const std::vector<uint8_t>& data)
        {
            AlgHandle alg(BCRYPT_SHA256_ALGORITHM);
            if (!alg.IsValid())
            {
                return {};
            }

            BCRYPT_HASH_HANDLE hashHandle = nullptr;
            ULONG objectSize = 0, result = 0;
            if (BCryptGetProperty(alg.Get(), BCRYPT_OBJECT_LENGTH,
                reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &result, 0) < 0)
            {
                return {};
            }

            std::vector<uint8_t> hashObject(objectSize, 0);
            if (BCryptCreateHash(alg.Get(), &hashHandle, hashObject.data(), objectSize,
                nullptr, 0, 0) < 0)
            {
                return {};
            }

            NTSTATUS status = BCryptHashData(hashHandle,
                const_cast<PUCHAR>(data.data()), static_cast<ULONG>(data.size()), 0);
            if (status < 0)
            {
                BCryptDestroyHash(hashHandle);
                return {};
            }

            std::vector<uint8_t> digest(kKeySize, 0);
            status = BCryptFinishHash(hashHandle, digest.data(),
                static_cast<ULONG>(digest.size()), 0);
            BCryptDestroyHash(hashHandle);

            if (status < 0)
            {
                return {};
            }
            return digest;
        }

        std::string SHA256Hex(const std::string& text)
        {
            std::vector<uint8_t> data(text.begin(), text.end());
            return ToHex(SHA256(data));
        }

        std::string SHA256Hex(const std::wstring& text)
        {
            return ToHex(SHA256(WideToUtf8Bytes(text)));
        }

        std::string SHA256File(const std::wstring& path)
        {
            // 大文件分块读取，避免一次性占用过多内存
            HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE)
            {
                return {};
            }

            AlgHandle alg(BCRYPT_SHA256_ALGORITHM);
            if (!alg.IsValid())
            {
                CloseHandle(file);
                return {};
            }

            BCRYPT_HASH_HANDLE hashHandle = nullptr;
            ULONG objectSize = 0, result = 0;
            if (BCryptGetProperty(alg.Get(), BCRYPT_OBJECT_LENGTH,
                reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &result, 0) < 0)
            {
                CloseHandle(file);
                return {};
            }

            std::vector<uint8_t> hashObject(objectSize, 0);
            if (BCryptCreateHash(alg.Get(), &hashHandle, hashObject.data(), objectSize,
                nullptr, 0, 0) < 0)
            {
                CloseHandle(file);
                return {};
            }

            std::vector<uint8_t> buffer(64 * 1024, 0);
            bool ok = true;
            for (;;)
            {
                DWORD read = 0;
                if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr))
                {
                    ok = false;
                    break;
                }
                if (read == 0)
                {
                    break;
                }
                if (BCryptHashData(hashHandle, buffer.data(), read, 0) < 0)
                {
                    ok = false;
                    break;
                }
            }
            CloseHandle(file);

            std::vector<uint8_t> digest(kKeySize, 0);
            if (ok)
            {
                ok = BCryptFinishHash(hashHandle, digest.data(),
                    static_cast<ULONG>(digest.size()), 0) >= 0;
            }
            BCryptDestroyHash(hashHandle);

            return ok ? ToHex(digest) : std::string();
        }

        std::string Base64Encode(const std::vector<uint8_t>& data)
        {
            if (data.empty())
            {
                return {};
            }

            DWORD need = 0;
            if (!CryptBinaryToStringA(data.data(), static_cast<DWORD>(data.size()),
                CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &need))
            {
                return {};
            }

            std::string buffer(need, '\0');
            if (!CryptBinaryToStringA(data.data(), static_cast<DWORD>(data.size()),
                CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buffer.data(), &need))
            {
                return {};
            }

            // CryptBinaryToStringA 的 need 包含结尾 '\0'，需要裁掉
            while (!buffer.empty() && buffer.back() == '\0')
            {
                buffer.pop_back();
            }
            return buffer;
        }

        std::vector<uint8_t> Base64Decode(const std::string& base64)
        {
            if (base64.empty())
            {
                return {};
            }

            DWORD need = 0;
            if (!CryptStringToBinaryA(base64.c_str(), 0, CRYPT_STRING_BASE64,
                nullptr, &need, nullptr, nullptr))
            {
                return {};
            }

            std::vector<uint8_t> buffer(need, 0);
            if (!CryptStringToBinaryA(base64.c_str(), 0, CRYPT_STRING_BASE64,
                buffer.data(), &need, nullptr, nullptr))
            {
                return {};
            }

            buffer.resize(need);
            return buffer;
        }

        std::string ToHex(const std::vector<uint8_t>& data)
        {
            static constexpr char kDigits[] = "0123456789abcdef";
            std::string text;
            text.reserve(data.size() * 2);
            for (uint8_t byte : data)
            {
                text.push_back(kDigits[byte >> 4]);
                text.push_back(kDigits[byte & 0x0F]);
            }
            return text;
        }

        std::vector<uint8_t> RandomBytes(size_t length)
        {
            if (length == 0)
            {
                return {};
            }

            std::vector<uint8_t> buffer(length, 0);
            if (BCryptGenRandom(nullptr, buffer.data(), static_cast<ULONG>(length),
                BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
            {
                return {};
            }
            return buffer;
        }
    }
}
