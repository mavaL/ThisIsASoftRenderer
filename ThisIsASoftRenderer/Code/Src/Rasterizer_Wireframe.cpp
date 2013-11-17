#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"
#include "RenderUtil.h"
#include "RenderObject.h"
#include "Profiler.h"

namespace SR
{
	///////////////////////////////////////////////////////////////////////////////////////////
	void RasWireFrame::_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context)
	{
		const VEC4& p0 = vert0.pos;
		const VEC4& p1 = vert1.pos;
		const VEC4& p2 = vert2.pos;

		//each line
		RenderUtil::DrawLine_DDA(Ext::Floor32_Fast(p0.x), Ext::Floor32_Fast(p0.y), Ext::Floor32_Fast(p1.x), Ext::Floor32_Fast(p1.y), SColor::WHITE);
		RenderUtil::DrawLine_DDA(Ext::Floor32_Fast(p1.x), Ext::Floor32_Fast(p1.y), Ext::Floor32_Fast(p2.x), Ext::Floor32_Fast(p2.y), SColor::WHITE);
		RenderUtil::DrawLine_DDA(Ext::Floor32_Fast(p0.x), Ext::Floor32_Fast(p0.y), Ext::Floor32_Fast(p2.x), Ext::Floor32_Fast(p2.y), SColor::WHITE);

#if USE_PROFILER == 1
		g_env.profiler->AddRenderedFace();
#endif
	}
}