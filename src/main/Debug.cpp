#include "Debug.h"

namespace Debug
{
	bool Debug;
	NOXS;
	void DrawDebugRect(RenderTarget& target, int x, int y, int w, int h)
	{
		if (!Debug) return;

		XGraph::SetColor(Color(255, 255, 100));
		static int lw = 1;
		XGraph::LineShape::SetLineWidth(lw);
		XGraph::RectangleShape::Rect(x + lw, y + lw, w - lw * 2, h - lw * 2, target);
	}
}