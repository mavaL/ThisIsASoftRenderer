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
	void RasTexturedGouraud::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, context);
	}

	void RasTexturedGouraud::LerpVertexAttributes( SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type )
	{
		RasGouraud::LerpVertexAttributes(dest, src1, src2, t, type);

		if (type == eLerpType_Linear)
		{
			Ext::LinearLerp(dest->uv, src1->uv, src2->uv, t);		
		} 
		else
		{
			float w0 = src1->pos.w;
			float w1 = src2->pos.w;

			Ext::HyperLerp(dest->uv, src1->uv, src2->uv, t, w0, w1);
			//双曲插值最后一步
			float inv_w = 1 / dest->pos.w;
			Common::Multiply_Vec2_By_K(dest->uv, dest->uv, inv_w);
		}
	}

	void RasTexturedGouraud::RasTriangleSetup( SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type )
	{
		RasGouraud::RasTriangleSetup(rasData, v0, v1, v2, type);

		const VEC4& p0 = v0->pos;
		const VEC4& p1 = v1->pos;
		const VEC4& p2 = v2->pos;

#if USE_PERSPEC_CORRECT == 1
		const VEC2 uv0(v0->uv.x*p0.w, v0->uv.y*p0.w);
		const VEC2 uv1(v1->uv.x*p1.w, v1->uv.y*p1.w);
		const VEC2 uv2(v2->uv.x*p2.w, v2->uv.y*p2.w);
#else
		const VEC2 uv0(v0->uv.x, v0->uv.y);
		const VEC2 uv1(v1->uv.x, v1->uv.y);
		const VEC2 uv2(v2->uv.x, v2->uv.y);
#endif

		if (type == eTriangleShape_Bottom)
		{
			//当前两端点uv分量及增量
			rasData.curUV_L = uv0;
			rasData.curUV_R = uv0;
			rasData.duv_L.Set((uv1.x-uv0.x)*rasData.inv_dy_L, (uv1.y-uv0.y)*rasData.inv_dy_L);
			rasData.duv_R.Set((uv2.x-uv0.x)*rasData.inv_dy_R, (uv2.y-uv0.y)*rasData.inv_dy_R);
		}
		else
		{
			//当前两端点uv分量及增量
			rasData.curUV_L = uv0;
			rasData.curUV_R = uv2;
			rasData.duv_L.Set((uv1.x-uv0.x)*rasData.inv_dy_L, (uv1.y-uv0.y)*rasData.inv_dy_L);
			rasData.duv_R.Set((uv1.x-uv2.x)*rasData.inv_dy_R, (uv1.y-uv2.y)*rasData.inv_dy_R);
		}

		//裁剪区域裁剪y
		if(rasData.bClipY)
		{
			Common::Add_Vec2_By_Vec2(rasData.curUV_L, rasData.curUV_L, Common::Multiply_Vec2_By_K(rasData.duv_L, rasData.clip_dy));
			Common::Add_Vec2_By_Vec2(rasData.curUV_R, rasData.curUV_R, Common::Multiply_Vec2_By_K(rasData.duv_R, rasData.clip_dy));
		}
	}

	void RasTexturedGouraud::RasLineSetup( SScanLine& scanLine, const SScanLinesData& rasData )
	{
		RasGouraud::RasLineSetup(scanLine, rasData);

		scanLine.deltaUV.Set((rasData.curUV_R.x-rasData.curUV_L.x)*scanLine.inv_dx, (rasData.curUV_R.y-rasData.curUV_L.y)*scanLine.inv_dx);
		scanLine.curUV = rasData.curUV_L;

		//裁剪区域裁剪x
		if(scanLine.bClipX)
		{
			Common::Add_Vec2_By_Vec2(scanLine.curUV, scanLine.curUV, Common::Multiply_Vec2_By_K(scanLine.deltaUV, scanLine.clip_dx));
		}
	}

	void RasTexturedGouraud::RaterizeAdvancePixel( SScanLine& scanLine )
	{
		RasGouraud::RaterizeAdvancePixel(scanLine);
		Common::Add_Vec2_By_Vec2(scanLine.curUV, scanLine.curUV, scanLine.deltaUV);
	}

	void RasTexturedGouraud::RaterizeAdvanceLine( SScanLinesData& scanLineData )
	{
		RasGouraud::RaterizeAdvanceLine(scanLineData);
		Common::Add_Vec2_By_Vec2(scanLineData.curUV_L, scanLineData.curUV_L, scanLineData.duv_L);
		Common::Add_Vec2_By_Vec2(scanLineData.curUV_R, scanLineData.curUV_R, scanLineData.duv_R);
	}

	void RasTexturedGouraud::RasterizePixel( SScanLine& scanLine, const SScanLinesData& rasData )
	{
#if USE_PERSPEC_CORRECT == 1
		//双曲插值最后一步
		float inv_w = 1 / scanLine.zw.y;
		scanLine.finalUV.Set(scanLine.curUV.x*inv_w, scanLine.curUV.y*inv_w);
		scanLine.curPixelClr.Set(scanLine.curClr.x*inv_w, scanLine.curClr.y*inv_w, scanLine.curClr.z*inv_w);
#else
		scanLine.finalUV = scanLine.curUV;
		scanLine.curPixelClr = scanLine.curClr;
#endif			

		if(rasData.pMaterial->bUseBilinearSampler)
		{
			rasData.pMaterial->pDiffuseMap->Tex2D_Bilinear(scanLine.finalUV, scanLine.pixelColor, rasData.texLod);
		}
		else
		{
			rasData.pMaterial->pDiffuseMap->Tex2D_Point(scanLine.finalUV, scanLine.pixelColor, rasData.texLod);
		}
		
		// Vertex color stores lighting diffuse
		// Don't modify alpha [1/19/2014 mavaL]
		float alpha = scanLine.pixelColor.a;

		scanLine.pixelColor *= scanLine.curPixelClr;
		scanLine.pixelColor.a = alpha;
		scanLine.pixelColor.Saturate();

		DWORD& dwDestColor = *(scanLine.pFragmeng->finalColor);

#if USE_OIT == 0
		SColor destPixelColor;
		destPixelColor.SetAsInt(dwDestColor);

		DoAlphaBlending(destPixelColor, scanLine.pixelColor, destPixelColor, rasData.pMaterial);

		dwDestColor = destPixelColor.GetAsInt();
#else
		scanLine.pixelColor.a *= rasData.pMaterial->transparency;
		dwDestColor = scanLine.pixelColor.GetAsInt();
#endif

#if USE_PROFILER == 1
		g_env.profiler->AddRenderedPixel();
#endif
	}
}