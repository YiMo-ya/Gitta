#pragma once

#include "Xs/Xs.h"

#include <string>
#include <vector>
#include <cstdint>

/// <summary>
/// 数据加解密与哈希工具箱。
/// 内部统一使用 Windows CNG（bcrypt）实现 AES-256-GCM 与 SHA-256，
/// 不引入任何第三方加密库，避免额外依赖。
/// 所有字符串接口的默认编码假定为 UTF-8；wstring 接口会自动在 UTF-8 与 UTF-16 之间转换。
/// </summary>
namespace Lock
{
    /// <summary>
    /// 对称加密原始字节流与文件。
    /// 采用 AES-256-GCM：同时提供机密性与完整性校验，密文被篡改时解密会失败。
    /// </summary>
    namespace AES
    {
        /// <summary>
        /// 加密任意字节数据。
        /// </summary>
        /// <param name="plain">待加密的明文数据。</param>
        /// <param name="key">32 字节（AES-256）密钥；长度不足 32 时返回空结果。</param>
        /// <returns>
        /// 输出布局为 iv(12) + tag(16) + cipherText 的二进制缓冲区；
        /// 失败时返回空 vector。
        /// </returns>
        std::vector<uint8_t> Encrypt(const std::vector<uint8_t>& plain, const std::string& key);

        /// <summary>
        /// 解密由 <see cref="Encrypt"/> 生成的密文缓冲区。
        /// </summary>
        /// <param name="sealed">布局为 iv(12) + tag(16) + cipherText 的密文。</param>
        /// <param name="key">与加密时一致的 32 字节密钥。</param>
        /// <returns>解密后的明文；密钥错误、认证标签不匹配或数据过短时返回空 vector。</returns>
        std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& sealed, const std::string& key);

        /// <summary>
        /// 使用随机 IV 加密文件内容。
        /// 输出文件布局为魔数 "LK01"(4) + iv(12) + tag(16) + cipherText，
        /// 便于读取时快速识别文件是否为加密格式。
        /// </summary>
        /// <param name="srcPath">源文件路径（明文）。</param>
        /// <param name="dstPath">目标文件路径（密文），可覆盖已有文件。</param>
        /// <param name="key">32 字节密钥。</param>
        /// <returns>成功返回 true；文件不存在、读取失败或密钥非法时返回 false。</returns>
        bool EncryptFile(const std::wstring& srcPath, const std::wstring& dstPath, const std::string& key);

        /// <summary>
        /// 解密由 <see cref="EncryptFile"/> 生成的文件。
        /// </summary>
        /// <param name="srcPath">源文件路径（密文）。</param>
        /// <param name="dstPath">目标文件路径（明文）。</param>
        /// <param name="key">与加密时一致的 32 字节密钥。</param>
        /// <returns>成功返回 true；魔数不符、密钥错误或认证失败时返回 false。</returns>
        bool DecryptFile(const std::wstring& srcPath, const std::wstring& dstPath, const std::string& key);

        /// <summary>
        /// 原地加密文件：先写入临时文件，成功后再替换源文件，避免中途失败造成数据丢失。
        /// </summary>
        /// <param name="path">要加密的文件路径。</param>
        /// <param name="key">32 字节密钥。</param>
        /// <returns>成功返回 true。</returns>
        bool EncryptFileInPlace(const std::wstring& path, const std::string& key);

        /// <summary>
        /// 原地解密文件，语义与 <see cref="EncryptFileInPlace"/> 对称。
        /// </summary>
        /// <param name="path">要解密的文件路径。</param>
        /// <param name="key">32 字节密钥。</param>
        /// <returns>成功返回 true。</returns>
        bool DecryptFileInPlace(const std::wstring& path, const std::string& key);
    };

    /// <summary>
    /// 字符串加密门面：内部把字符串按 UTF-8 转成字节后调用 <see cref="AES"/>，
    /// 结果以 Base64 返回，便于写入配置、日志或网络传输。
    /// </summary>
    namespace String
    {
        /// <summary>
        /// 加密 UTF-8 字节串。
        /// </summary>
        /// <param name="plain">明文（按原始字节处理）。</param>
        /// <param name="key">32 字节密钥。</param>
        /// <returns>Base64 编码的密文；失败返回空串。</returns>
        std::string Encrypt(const std::string& plain, const std::string& key);

        /// <summary>
        /// 解密 <see cref="Encrypt(const std::string&amp;, const std::string&amp;)"/> 的输出。
        /// </summary>
        /// <param name="cipherBase64">Base64 编码的密文。</param>
        /// <param name="key">与加密时一致的 32 字节密钥。</param>
        /// <returns>明文；Base64 非法或认证失败时返回空串。</returns>
        std::string Decrypt(const std::string& cipherBase64, const std::string& key);

        /// <summary>
        /// 加密宽字符串（内部转为 UTF-8 后再加密）。
        /// </summary>
        /// <param name="plain">宽字符明文。</param>
        /// <param name="key">32 字节密钥。</param>
        /// <returns>Base64 编码的密文（窄字符，可直接持久化）；失败返回空串。</returns>
        std::string Encrypt(const std::wstring& plain, const std::string& key);

        /// <summary>
        /// 解密宽字符串。
        /// </summary>
        /// <param name="cipherBase64">Base64 编码的密文。</param>
        /// <param name="key">与加密时一致的 32 字节密钥。</param>
        /// <returns>宽字符明文；失败返回空串。</returns>
        std::wstring DecryptW(const std::string& cipherBase64, const std::string& key);

        /// <summary>
        /// 判断给定密钥长度是否满足 AES-256 要求（32 字节）。
        /// </summary>
        /// <param name="key">待检查的密钥。</param>
        /// <returns>长度等于 32 返回 true。</returns>
        bool IsValidKey(const std::string& key);

        /// <summary>
        /// 由任意长度的口令派生 32 字节密钥：内部使用 SHA-256 摘要，便于用户直接传密码。
        /// </summary>
        /// <param name="password">用户口令。</param>
        /// <returns>32 字节派生密钥；口令为空时仍会返回基于空串的摘要。</returns>
        std::string DeriveKey(const std::string& password);

        /// <summary>
        /// 由宽字符串口令派生 32 字节密钥。
        /// </summary>
        /// <param name="password">用户口令。</param>
        /// <returns>32 字节派生密钥。</returns>
        std::string DeriveKey(const std::wstring& password);
    };

    /// <summary>
    /// SHA-256 哈希与十六进制编码工具。
    /// </summary>
    namespace Hash
    {
        /// <summary>
        /// 计算 SHA-256 原始摘要。
        /// </summary>
        /// <param name="data">输入字节流。</param>
        /// <returns>32 字节摘要；失败返回空 vector。</returns>
        std::vector<uint8_t> SHA256(const std::vector<uint8_t>& data);

        /// <summary>
        /// 对 UTF-8 字符串计算 SHA-256，并输出小写十六进制文本。
        /// </summary>
        /// <param name="text">输入文本（按原始字节处理）。</param>
        /// <returns>64 字符小写十六进制摘要；失败返回空串。</returns>
        std::string SHA256Hex(const std::string& text);

        /// <summary>
        /// 对宽字符串计算 SHA-256（内部转为 UTF-8），并输出小写十六进制文本。
        /// </summary>
        /// <param name="text">输入文本。</param>
        /// <returns>64 字符小写十六进制摘要；失败返回空串。</returns>
        std::string SHA256Hex(const std::wstring& text);

        /// <summary>
        /// 计算文件的 SHA-256，适合做完整性校验。
        /// </summary>
        /// <param name="path">文件路径。</param>
        /// <returns>64 字符小写十六进制摘要；文件打不开时返回空串。</returns>
        std::string SHA256File(const std::wstring& path);

        /// <summary>
        /// 把任意字节流编码为 Base64 文本。
        /// </summary>
        /// <param name="data">待编码的字节流。</param>
        /// <returns>Base64 字符串；输入为空时返回空串。</returns>
        std::string Base64Encode(const std::vector<uint8_t>& data);

        /// <summary>
        /// 把 Base64 文本解码为字节流。
        /// </summary>
        /// <param name="base64">Base64 字符串。</param>
        /// <returns>解码后的字节流；输入非法时返回空 vector。</returns>
        std::vector<uint8_t> Base64Decode(const std::string& base64);

        /// <summary>
        /// 把字节流编码为小写十六进制文本。
        /// </summary>
        /// <param name="data">待编码的字节流。</param>
        /// <returns>十六进制字符串。</returns>
        std::string ToHex(const std::vector<uint8_t>& data);

        /// <summary>
        /// 生成密码学安全的随机字节序列，可用于生成密钥或盐。
        /// </summary>
        /// <param name="length">需要的字节数。</param>
        /// <returns>随机字节；失败时返回空 vector。</returns>
        std::vector<uint8_t> RandomBytes(size_t length);
    };
}
