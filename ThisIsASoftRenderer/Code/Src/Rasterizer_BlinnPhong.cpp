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
	void RasBlinnPhong::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, context);
	}

	void RasBlinnPhong::DoPerPixelLighting(SColor& result, void* pLightingContext, const SMaterial* pMaterial)
	{
		SLightingContext_Phong* plc = (SLightingContext_Phong*)pLightingContext;

		VEC3 N = *plc->worldNormal;
		N.Normalize();

		RenderUtil::DoLambertLighting(result, N, g_env.renderer->m_testLight.neg_dir, pMaterial);

		//高光
		const VEC3& camPos = g_env.renderer->m_camera.GetPos().GetVec3();
		VEC3 V = Common::Sub_Vec3_By_Vec3(camPos, *plc->worldPos);
		V.Normalize();
		VEC3 H = Common::Add_Vec3_By_Vec3(V, g_env.renderer->m_testLight.neg_dir);
		H.Normalize();

		float spec = pow(max(Common::DotProduct_Vec3_By_Vec3(N, H), 0), pMaterial->shiness);
		result += pMaterial->specular * spec;
	}

	void RasBlinnPhong::LerpVertexAttributes( SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type )
	{
		if (type == eLerpType_Linear)
		{
			Ext::LinearLerp(dest->uv			, src1->uv			, src2->uv			, t);
			Ext::LinearLerp(dest->worldPos		, src1->worldPos	, src2->worldPos	, t);
			Ext::LinearLerp(dest->worldNormal	, src1->worldNormal	, src2->worldNormal	, t);
		} 
		else
		{
			float w0 = src1->pos.w;
			float w1 = src2->pos.w;

			Ext::HyperLerp(dest->uv				, src1->uv			, src2->uv			, t, w0, w1);
			Ext::HyperLerp(dest->worldPos		, src1->worldPos	, src2->worldPos	, t, w0, w1);
			Ext::HyperLerp(dest->worldNormal	, src1->worldNormal	, src2->worldNormal	, t, w0, w1);
			//双曲插值最后一步
			float inv_w = 1 / dest->pos.w;
			Common::Multiply_Vec2_By_K(dest->uv			, dest->uv			, inv_w);
			Common::Multiply_Vec4_By_K(dest->worldPos	, dest->worldPos	, inv_w);
			Common::Multiply_Vec3_By_K(dest->worldNormal, dest->worldNormal	, inv_w);
		}
	}

	void RasBlinnPhong::RasTriangleSetup( SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type )
	{
		Rasterizer::RasTriangleSetup(rasData, v0, v1, v2, type);

		const VEC4& p0 = v0->pos;
		const VEC4& p1 = v1->pos;
		const VEC4& p2 = v2->pos;

#if USE_PERSPEC_CORRECT == 1
		const VEC2 uv0(v0->uv.x*p0.w, v0->uv.y*p0.w);
		const VEC2 uv1(v1->uv.x*p1.w, v1->uv.y*p1.w);
		const VEC2 uv2(v2->uv.x*p2.w, v2->uv.y*p2.w);

		const VEC3 wp0(v0->worldPos.x*p0.w, v0->worldPos.y*p0.w, v0->worldPos.z*p0.w);
		const VEC3 wp1(v1->worldPos.x*p1.w, v1->worldPos.y*p1.w, v1->worldPos.z*p1.w);
		const VEC3 wp2(v2->worldPos.x*p2.w, v2->worldPos.y*p2.w, v2->worldPos.z*p2.w);

		const VEC3 wn0(v0->worldNormal.x*p0.w, v0->worldNormal.y*p0.w, v0->worldNormal.z*p0.w);
		const VEC3 wn1(v1->worldNormal.x*p1.w, v1->worldNormal.y*p1.w, v1->worldNormal.z*p1.w);
		const VEC3 wn2(v2->worldNormal.x*p2.w, v2->worldNormal.y*p2.w, v2->worldNormal.z*p2.w);
#else
		const VEC2 uv0(v0->uv.x, v0->uv.y);
		const VEC2 uv1(v1->uv.x, v1->uv.y);
		const VEC2 uv2(v2->uv.x, v2->uv.y);

		const VEC3& wp0 = v0->worldPos.GetVec3();
		const VEC3& wp1 = v1->worldPos.GetVec3();
		const VEC3& wp2 = v2->worldPos.GetVec3();

		const VEC3& wn0 = v0->worldNormal;
		const VEC3& wn1 = v1->worldNormal;
		const VEC3& wn2 = v2->worldNormal;
#endif

		if (type == eTriangleShape_Bottom)
		{
			//当前两端点uv分量及增量
			rasData.curUV_L = uv0;
			rasData.curUV_R = uv0;
			rasData.duv_L.Set((uv1.x-uv0.x)*rasData.inv_dy_L, (uv1.y-uv0.y)*rasData.inv_dy_L);
			rasData.duv_R.Set((uv2.x-uv0.x)*rasData.inv_dy_R, (uv2.y-uv0.y)*rasData.inv_dy_R);
			//世界坐标及增量
			rasData.curPW_L = wp0;
			rasData.curPW_R = wp0;
			rasData.dpw_L.Set((wp1.x-wp0.x)*rasData.inv_dy_L, (wp1.y-wp0.y)*rasData.inv_dy_L, (wp1.z-wp0.z)*rasData.inv_dy_L);
			rasData.dpw_R.Set((wp2.x-wp0.x)*rasData.inv_dy_R, (wp2.y-wp0.y)*rasData.inv_dy_R, (wp2.z-wp0.z)*rasData.inv_dy_R);
			//世界法线及增量
			rasData.curN_L = wn0;
			rasData.curN_R = wn0;
			rasData.dn_L.Set((wn1.x-wn0.x)*rasData.inv_dy_L, (wn1.y-wn0.y)*rasData.inv_dy_L, (wn1.z-wn0.z)*rasData.inv_dy_L);
			rasData.dn_R.Set((wn2.x-wn0.x)*rasData.inv_dy_R, (wn2.y-wn0.y)*rasData.inv_dy_R, (wn2.z-wn0.z)*rasData.inv_dy_R);
		}
		else
		{
			//当前两端点uv分量及增量
			rasData.curUV_L = uv0;
			rasData.curUV_R = uv2;
			rasData.duv_L.Set((uv1.x-uv0.x)*rasData.inv_dy_L, (uv1.y-uv0.y)*rasData.inv_dy_L);
			rasData.duv_R.Set((uv1.x-uv2.x)*rasData.inv_dy_R, (uv1.y-uv2.y)*rasData.inv_dy_R);
			//世界坐标及增量
			rasData.curPW_L = wp0;
			rasData.curPW_R = wp2;
			rasData.dpw_L.Set((wp1.x-wp0.x)*rasData.inv_dy_L, (wp1.y-wp0.y)*rasData.inv_dy_L, (wp1.z-wp0.z)*rasData.inv_dy_L);
			rasData.dpw_R.Set((wp1.x-wp2.x)*rasData.inv_dy_R, (wp1.y-wp2.y)*rasData.inv_dy_R, (wp1.z-wp2.z)*rasData.inv_dy_R);
			//世界法线及增量
			rasData.curN_L = wn0;
			rasData.curN_R = wn2;
			rasData.dn_L.Set((wn1.x-wn0.x)*rasData.inv_dy_L, (wn1.y-wn0.y)*rasData.inv_dy_L, (wn1.z-wn0.z)*rasData.inv_dy_L);
			rasData.dn_R.Set((wn1.x-wn2.x)*rasData.inv_dy_R, (wn1.y-wn2.y)*rasData.inv_dy_R, (wn1.z-wn2.z)*rasData.inv_dy_R);
		}

		//裁剪区域裁剪y
		if(rasData.bClipY)
		{
			Common::Add_Vec2_By_Vec2(rasData.curUV_L, rasData.curUV_L, Common::Multiply_Vec2_By_K(rasData.duv_L, rasData.clip_dy));
			Common::Add_Vec2_By_Vec2(rasData.curUV_R, rasData.curUV_R, Common::Multiply_Vec2_By_K(rasData.duv_R, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curPW_L, rasData.curPW_L, Common::Multiply_Vec3_By_K(rasData.dpw_L, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curPW_R, rasData.curPW_R, Common::Multiply_Vec3_By_K(rasData.dpw_R, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curN_L, rasData.curN_L, Common::Multiply_Vec3_By_K(rasData.dn_L, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curN_R, rasData.curN_R, Common::Multiply_Vec3_By_K(rasData.dn_R, rasData.clip_dy));
		}
	}

	void RasBlinnPhong::RasLineSetup( SScanLine& scanLine, const SScanLinesData& rasData )
	{
		Rasterizer::RasLineSetup(scanLine, rasData);

		scanLine.deltaUV.Set((rasData.curUV_R.x-rasData.curUV_L.x)*scanLine.inv_dx, (rasData.curUV_R.y-rasData.curUV_L.y)*scanLine.inv_dx);
		scanLine.deltaPW.Set((rasData.curPW_R.x-rasData.curPW_L.x)*scanLine.inv_dx, (rasData.curPW_R.y-rasData.curPW_L.y)*scanLine.inv_dx, (rasData.curPW_R.z-rasData.curPW_L.z)*scanLine.inv_dx);
		scanLine.deltaN.Set((rasData.curN_R.x-rasData.curN_L.x)*scanLine.inv_dx, (rasData.curN_R.y-rasData.curN_L.y)*scanLine.inv_dx, (rasData.curN_R.z-rasData.curN_L.z)*scanLine.inv_dx);

		scanLine.curUV = rasData.curUV_L;
		scanLine.curPW = rasData.curPW_L;
		scanLine.curN = rasData.curN_L;

		//裁剪区域裁剪x
		if(scanLine.bClipX)
		{
			Common::Add_Vec2_By_Vec2(scanLine.curUV, scanLine.curUV, Common::Multiply_Vec2_By_K(scanLine.deltaUV, scanLine.clip_dx));
			Common::Add_Vec3_By_Vec3(scanLine.curPW, scanLine.curPW, Common::Multiply_Vec3_By_K(scanLine.deltaPW, scanLine.clip_dx));
			Common::Add_Vec3_By_Vec3(scanLine.curN, scanLine.curN, Common::Multiply_Vec3_By_K(scanLine.deltaN, scanLine.clip_dx));
		}
	}

	void RasBlinnPhong::RaterizeAdvancePixel( SScanLine& scanLine )
	{
		Common::Add_Vec2_By_Vec2(scanLine.curUV, scanLine.curUV, scanLine.deltaUV);
		Common::Add_Vec3_By_Vec3(scanLine.curPW, scanLine.curPW, scanLine.deltaPW);
		Common::Add_Vec3_By_Vec3(scanLine.curN, scanLine.curN, scanLine.deltaN);
	}

	void RasBlinnPhong::RaterizeAdvanceLine( SScanLinesData& scanLineData )
	{
		Common::Add_Vec2_By_Vec2(scanLineData.curUV_L, scanLineData.curUV_L, scanLineData.duv_L);
		Common::Add_Vec2_By_Vec2(scanLineData.curUV_R, scanLineData.curUV_R, scanLineData.duv_R);
		Common::Add_Vec3_By_Vec3(scanLineData.curPW_L, scanLineData.curPW_L, scanLineData.dpw_L);
		Common::Add_Vec3_By_Vec3(scanLineData.curPW_R, scanLineData.curPW_R, scanLineData.dpw_R);
		Common::Add_Vec3_By_Vec3(scanLineData.curN_L, scanLineData.curN_L, scanLineData.dn_L);
		Common::Add_Vec3_By_Vec3(scanLineData.curN_R, scanLineData.curN_R, scanLineData.dn_R);
	}

	void RasBlinnPhong::RasterizePixel( SScanLine& scanLine, const SScanLinesData& rasData )
	{
#if USE_PERSPEC_CORRECT == 1
		//双曲插值最后一步
		float inv_w = 1 / scanLine.zw.y;
		scanLine.finalUV.Set(scanLine.curUV.x*inv_w, scanLine.curUV.y*inv_w);
		scanLine.finalPW.Set(scanLine.curPW.x*inv_w, scanLine.curPW.y*inv_w, scanLine.curPW.z*inv_w);
		scanLine.finalN.Set(scanLine.curN.x*inv_w, scanLine.curN.y*inv_w, scanLine.curN.z*inv_w);
#else
		scanLine.finalUV = scanLine.curUV;
		scanLine.finalPW = scanLine.curPW;
		scanLine.finalN = scanLine.curN;
#endif			

		scanLine.pFragmeng->texLod = rasData.texLod;
		scanLine.pFragmeng->uv = scanLine.finalUV;
		scanLine.pFragmeng->worldPos = scanLine.finalPW;
		scanLine.pFragmeng->normal = scanLine.finalN;

#if USE_MULTI_THREAD == 1
		scanLine.pFragmeng->bActive = true;
#else
		FragmentPS(*scanLine.pFragmeng);
#endif
	}

	void RasBlinnPhong::FragmentPS( SFragment& frag )
	{
		SColor texColor(SColor::WHITE), lightColor(SColor::WHITE);
		SMaterial* pMaterial = frag.pMaterial;

		if(pMaterial->pDiffuseMap && pMaterial->bUseBilinearSampler)
		{
			pMaterial->pDiffuseMap->Tex2D_Bilinear(frag.uv, texColor, frag.texLod);
		}
		else if(pMaterial->pDiffuseMap)
		{
			pMaterial->pDiffuseMap->Tex2D_Point(frag.uv, texColor, frag.texLod);
		}

		SLightingContext_Phong lc;
		lc.uv = &frag.uv;
		lc.worldNormal = &frag.normal;
		lc.worldPos = &frag.worldPos;

		g_env.renderer->GetCurRas()->DoPerPixelLighting(lightColor, &lc, pMaterial);
		texColor *= lightColor;
		texColor.Saturate();
		*frag.finalColor = texColor.GetAsInt();

#if USE_PROFILER == 1
		g_env.profiler->AddRenderedPixel();
#endif
	}
}