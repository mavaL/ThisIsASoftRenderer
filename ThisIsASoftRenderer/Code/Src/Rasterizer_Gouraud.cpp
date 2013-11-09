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
	void RasGouraud::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, context);
	}

	void RasGouraud::DoPerVertexLighting( VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj )
	{
		///Gouraud shade基于逐顶点法线
		for (size_t iVert=0; iVert<workingVB.size(); ++iVert)
		{
			SVertex& vert = workingVB[iVert];

			if(!vert.bActive)
				continue;

			//在世界空间进行光照
			VEC3 worldNormal = Common::Transform_Vec3_By_Mat44(vert.normal, obj.m_matWorldIT, false).GetVec3();
			worldNormal.Normalize();

			SColor tmp;
			RenderUtil::DoLambertLighting(tmp, worldNormal, g_env.renderer->m_testLight.neg_dir, obj.m_pMaterial);
			vert.color *= tmp;
		}
	}

	void RasGouraud::LerpVertexAttributes( SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type )
	{
		if (type == eLerpType_Linear)
		{
			Ext::LinearLerp(dest->color, src1->color, src2->color, t);			
		} 
		else
		{
			float w0 = src1->pos.w;
			float w1 = src2->pos.w;

			Ext::HyperLerp(dest->color, src1->color, src2->color, t, w0, w1);
			//双曲插值最后一步
			float inv_w = 1 / dest->pos.w;
			dest->color *= inv_w;
		}
	}

	void RasGouraud::RasTriangleSetup( SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type )
	{
		Rasterizer::RasTriangleSetup(rasData, v0, v1, v2, type);

		const VEC4& p0 = v0->pos;
		const VEC4& p1 = v1->pos;
		const VEC4& p2 = v2->pos;

#if USE_PERSPEC_CORRECT == 1
		const SColor c0(v0->color.r*p0.w, v0->color.g*p0.w, v0->color.b*p0.w);
		const SColor c1(v1->color.r*p1.w, v1->color.g*p1.w, v1->color.b*p1.w);
		const SColor c2(v2->color.r*p2.w, v2->color.g*p2.w, v2->color.b*p2.w);
#else
		const SColor& c0 = v0->color;
		const SColor& c1 = v1->color;
		const SColor& c2 = v2->color;
#endif

		if (type == eTriangleShape_Bottom)
		{
			//当前两端点颜色分量及增量
			rasData.clr_L.Set(c0.r, c0.g, c0.b);
			rasData.clr_R.Set(c0.r, c0.g, c0.b);
			rasData.dclr_L.Set((c1.r-c0.r)*rasData.inv_dy_L, (c1.g-c0.g)*rasData.inv_dy_L, (c1.b-c0.b)*rasData.inv_dy_L);
			rasData.dclr_R.Set((c2.r-c0.r)*rasData.inv_dy_R, (c2.g-c0.g)*rasData.inv_dy_R, (c2.b-c0.b)*rasData.inv_dy_R);
		}
		else
		{
			//当前两端点颜色分量及增量
			rasData.clr_L.Set(c0.r, c0.g, c0.b);
			rasData.clr_R.Set(c2.r, c2.g, c2.b);
			rasData.dclr_L.Set((c1.r-c0.r)*rasData.inv_dy_L, (c1.g-c0.g)*rasData.inv_dy_L, (c1.b-c0.b)*rasData.inv_dy_L);
			rasData.dclr_R.Set((c1.r-c2.r)*rasData.inv_dy_R, (c1.g-c2.g)*rasData.inv_dy_R, (c1.b-c2.b)*rasData.inv_dy_R);
		}

		//裁剪区域裁剪y
		if(rasData.bClipY)
		{
			Common::Add_Vec3_By_Vec3(rasData.clr_L, rasData.clr_L, Common::Multiply_Vec3_By_K(rasData.dclr_L, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.clr_R, rasData.clr_R, Common::Multiply_Vec3_By_K(rasData.dclr_R, rasData.clip_dy));
		}
	}

	void RasGouraud::RasLineSetup( SScanLine& scanLine, const SScanLinesData& rasData )
	{
		Rasterizer::RasLineSetup(scanLine, rasData);

		scanLine.deltaClr.Set((rasData.clr_R.x-rasData.clr_L.x)*scanLine.inv_dx, (rasData.clr_R.y-rasData.clr_L.y)*scanLine.inv_dx, (rasData.clr_R.z-rasData.clr_L.z)*scanLine.inv_dx);
		scanLine.curClr = rasData.clr_L;

		//裁剪区域裁剪x
		if(scanLine.bClipX)
		{
			Common::Add_Vec3_By_Vec3(scanLine.curClr, scanLine.curClr, Common::Multiply_Vec3_By_K(scanLine.deltaClr, scanLine.clip_dx));
		}
	}

	void RasGouraud::RasterizePixel( SScanLine& scanLine, const SScanLinesData& rasData )
	{
#if USE_PERSPEC_CORRECT == 1
		//双曲插值最后一步
		float inv_w = 1 / scanLine.w;
		scanLine.pixelColor.Set(scanLine.curClr.x*inv_w, scanLine.curClr.y*inv_w, scanLine.curClr.z*inv_w);
#else
		scanLine.pixelColor = scanLine.curClr;
#endif

		scanLine.pixelColor.Saturate();
		*scanLine.pFragmeng->finalColor = scanLine.pixelColor.GetAsInt();

#if USE_PROFILER == 1
		g_env.profiler->AddRenderedPixel();
#endif
	}
}