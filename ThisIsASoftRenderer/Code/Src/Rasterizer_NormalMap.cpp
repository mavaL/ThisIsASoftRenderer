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
	void RasNormalMap::DoPerVertexLighting( VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj )
	{
		const VEC3& camPos = g_env.renderer->m_camera.GetPos().GetVec3();
		MAT44 matInvWorld = obj.m_matWorld.Inverse();

		for (size_t iVert=0; iVert<obj.m_verts.size(); ++iVert)
		{
			SVertex& vert = obj.m_verts[iVert];

			// Get the matrix which transforms vector from object space to tangent space
			MAT44 matTBN;
			matTBN.SetRow(0, VEC4(vert.tangent, 0));
			matTBN.SetRow(1, VEC4(vert.binormal, 0));
			matTBN.SetRow(2, VEC4(vert.normal, 0));

			// Calc light dir in object space
			const VEC3& lightDir = g_env.renderer->m_testLight.neg_dir;
			vert.lightDirTS = Common::Transform_Vec3_By_Mat44(lightDir, matInvWorld, false).GetVec3();
			// Transform!
			vert.lightDirTS = Common::Transform_Vec3_By_Mat44(vert.lightDirTS, matTBN, false).GetVec3();

			// Half-angle vector
			VEC3 eyeDir = Common::Sub_Vec3_By_Vec3(camPos, vert.pos.GetVec3());
			eyeDir.Normalize();
			Common::Add_Vec3_By_Vec3(vert.halfAngleTS, eyeDir, lightDir);
			vert.halfAngleTS.Normalize();
			vert.halfAngleTS = Common::Transform_Vec3_By_Mat44(vert.halfAngleTS, matTBN, false).GetVec3();
		}
	}

	void RasNormalMap::DoPerPixelLighting( SColor& result, void* pLightingContext, const SMaterial* pMaterial )
	{
		SLightingContext_NormalMap* plc = (SLightingContext_NormalMap*)pLightingContext;

		pMaterial->pNormalMap->Tex2D_Point(*plc->uv, result);

		VEC3 N(result.r, result.g, result.b);
		// Expand
		Common::Add_Vec3_By_Vec3(N, N, VEC3(-0.5f,-0.5f,-0.5f));
		Common::Multiply_Vec3_By_K(N, N, 2);
		N.Normalize();

		RenderUtil::DoLambertLighting(result, N, *plc->lightDirTS, pMaterial);

		//高光
		float spec = pow(max(Common::DotProduct_Vec3_By_Vec3(N, *plc->hVectorTS), 0), pMaterial->shiness);
		result += pMaterial->specular * spec;
	}

	void RasNormalMap::LerpVertexAttributes( SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type )
	{
		if (type == eLerpType_Linear)
		{			
			Ext::LinearLerp(dest->uv			, src1->uv			, src2->uv			, t);
			Ext::LinearLerp(dest->lightDirTS	, src1->lightDirTS	, src2->lightDirTS	, t);			
			Ext::LinearLerp(dest->halfAngleTS	, src1->halfAngleTS	, src2->halfAngleTS	, t);
		} 
		else
		{
			float w0 = src1->pos.w;
			float w1 = src2->pos.w;

			Ext::HyperLerp(dest->uv				, src1->uv			, src2->uv			, t, w0, w1);
			Ext::HyperLerp(dest->lightDirTS		, src1->lightDirTS	, src2->lightDirTS	, t, w0, w1);
			Ext::HyperLerp(dest->halfAngleTS	, src1->halfAngleTS	, src2->halfAngleTS	, t, w0, w1);
			//双曲插值最后一步
			float inv_w = 1 / dest->pos.w;
			Common::Multiply_Vec2_By_K(dest->uv			, dest->uv			, inv_w);
			Common::Multiply_Vec3_By_K(dest->lightDirTS	, dest->lightDirTS	, inv_w);
			Common::Multiply_Vec3_By_K(dest->halfAngleTS, dest->halfAngleTS	, inv_w);
		}
	}

	void RasNormalMap::RasTriangleSetup( SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type )
	{
		Rasterizer::RasTriangleSetup(rasData, v0, v1, v2, type);

		const VEC4& p0 = v0->pos;
		const VEC4& p1 = v1->pos;
		const VEC4& p2 = v2->pos;

#if USE_PERSPEC_CORRECT == 1
		VEC2 uv0 = v0->uv;
		VEC2 uv1 = v1->uv;
		VEC2 uv2 = v2->uv;

		Common::Multiply_Vec2_By_K(uv0, uv0, p0.w);
		Common::Multiply_Vec2_By_K(uv1, uv1, p1.w);
		Common::Multiply_Vec2_By_K(uv2, uv2, p2.w);

		VEC3 lightDir0 = v0->lightDirTS;
		VEC3 lightDir1 = v1->lightDirTS;
		VEC3 lightDir2 = v2->lightDirTS;

		Common::Multiply_Vec3_By_K(lightDir0, lightDir0, p0.w);
		Common::Multiply_Vec3_By_K(lightDir1, lightDir1, p1.w);
		Common::Multiply_Vec3_By_K(lightDir2, lightDir2, p2.w);

		VEC3 hVector0 = v0->halfAngleTS;
		VEC3 hVector1 = v1->halfAngleTS;
		VEC3 hVector2 = v2->halfAngleTS;

		Common::Multiply_Vec3_By_K(hVector0, hVector0, p0.w);
		Common::Multiply_Vec3_By_K(hVector1, hVector1, p1.w);
		Common::Multiply_Vec3_By_K(hVector2, hVector2, p2.w);
#else
		const VEC2 uv0(v0->uv.x, v0->uv.y);
		const VEC2 uv1(v1->uv.x, v1->uv.y);
		const VEC2 uv2(v2->uv.x, v2->uv.y);

		const VEC3 lightDir0 = v0->lightDirTS;
		const VEC3 lightDir1 = v1->lightDirTS;
		const VEC3 lightDir2 = v2->lightDirTS;

		const VEC3 hVector0 = v0->halfAngleTS;
		const VEC3 hVector1 = v1->halfAngleTS;
		const VEC3 hVector2 = v2->halfAngleTS;
#endif

		if (type == eTriangleShape_Bottom)
		{
			//当前两端点uv分量及增量
			rasData.curUV_L = uv0;
			rasData.curUV_R = uv0;
			rasData.duv_L = Common::Sub_Vec2_By_Vec2(uv1, uv0);
			rasData.duv_R = Common::Sub_Vec2_By_Vec2(uv2, uv0);
			//光源方向(切空间)及其增量
			rasData.curLightDir_L = lightDir0;
			rasData.curLightDir_R = lightDir0;
			rasData.dLightDir_L = Common::Sub_Vec3_By_Vec3(lightDir1, lightDir0);
			rasData.dLightDir_R = Common::Sub_Vec3_By_Vec3(lightDir2, lightDir0);
			// Half angle vector(切空间)及其增量
			rasData.curHVector_L = hVector0;
			rasData.curHVector_R = hVector0;
			rasData.dHVector_L = Common::Sub_Vec3_By_Vec3(hVector1, hVector0);
			rasData.dHVector_R = Common::Sub_Vec3_By_Vec3(hVector2, hVector0);
		}
		else
		{
			//当前两端点uv分量及增量
			rasData.curUV_L = uv0;
			rasData.curUV_R = uv2;
			rasData.duv_L = Common::Sub_Vec2_By_Vec2(uv1, uv0);
			rasData.duv_R = Common::Sub_Vec2_By_Vec2(uv1, uv2);
			//光源方向(切空间)及其增量
			rasData.curLightDir_L = lightDir0;
			rasData.curLightDir_R = lightDir2;
			rasData.dLightDir_L = Common::Sub_Vec3_By_Vec3(lightDir1, lightDir0);
			rasData.dLightDir_R = Common::Sub_Vec3_By_Vec3(lightDir1, lightDir2);
			// Half angle vector(切空间)及其增量
			rasData.curHVector_L = hVector0;
			rasData.curHVector_R = hVector2;
			rasData.dHVector_L = Common::Sub_Vec3_By_Vec3(hVector1, hVector0);
			rasData.dHVector_R = Common::Sub_Vec3_By_Vec3(hVector1, hVector2);
		}
		Common::Multiply_Vec2_By_K(rasData.duv_L, rasData.duv_L, rasData.inv_dy_L);
		Common::Multiply_Vec2_By_K(rasData.duv_R, rasData.duv_R, rasData.inv_dy_R);
		Common::Multiply_Vec3_By_K(rasData.dLightDir_L, rasData.dLightDir_L, rasData.inv_dy_L);
		Common::Multiply_Vec3_By_K(rasData.dLightDir_R, rasData.dLightDir_R, rasData.inv_dy_R);
		Common::Multiply_Vec3_By_K(rasData.dHVector_L, rasData.dHVector_L, rasData.inv_dy_L);
		Common::Multiply_Vec3_By_K(rasData.dHVector_R, rasData.dHVector_R, rasData.inv_dy_R);

		//裁剪区域裁剪y
		if(rasData.bClipY)
		{
			Common::Add_Vec2_By_Vec2(rasData.curUV_L, rasData.curUV_L, Common::Multiply_Vec2_By_K(rasData.duv_L, rasData.clip_dy));
			Common::Add_Vec2_By_Vec2(rasData.curUV_R, rasData.curUV_R, Common::Multiply_Vec2_By_K(rasData.duv_R, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curLightDir_L, rasData.curLightDir_L, Common::Multiply_Vec3_By_K(rasData.dLightDir_L, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curLightDir_R, rasData.curLightDir_R, Common::Multiply_Vec3_By_K(rasData.dLightDir_R, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curHVector_L, rasData.curHVector_L, Common::Multiply_Vec3_By_K(rasData.dHVector_L, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curHVector_R, rasData.curHVector_R, Common::Multiply_Vec3_By_K(rasData.dHVector_R, rasData.clip_dy));
		}
	}

	void RasNormalMap::RasLineSetup( SScanLine& scanLine, const SScanLinesData& rasData )
	{
		Rasterizer::RasLineSetup(scanLine, rasData);

		scanLine.deltaUV		= Common::Sub_Vec2_By_Vec2(rasData.curUV_R, rasData.curUV_L);
		scanLine.deltaLightDir	= Common::Sub_Vec3_By_Vec3(rasData.curLightDir_R, rasData.curLightDir_L);
		scanLine.deltaHVector	= Common::Sub_Vec3_By_Vec3(rasData.curHVector_R, rasData.curHVector_L);
		Common::Multiply_Vec2_By_K(scanLine.deltaUV, scanLine.deltaUV, scanLine.inv_dx);
		Common::Multiply_Vec3_By_K(scanLine.deltaLightDir, scanLine.deltaLightDir, scanLine.inv_dx);
		Common::Multiply_Vec3_By_K(scanLine.deltaHVector, scanLine.deltaHVector, scanLine.inv_dx);

		scanLine.curUV		= rasData.curUV_L;
		scanLine.curLightDir = rasData.curLightDir_L;
		scanLine.curHVector = rasData.curHVector_L;

		//裁剪区域裁剪x
		if(scanLine.bClipX)
		{
			Common::Add_Vec2_By_Vec2(scanLine.curUV, scanLine.curUV, Common::Multiply_Vec2_By_K(scanLine.deltaUV, scanLine.clip_dx));
			Common::Add_Vec3_By_Vec3(scanLine.curLightDir, scanLine.curLightDir, Common::Multiply_Vec3_By_K(scanLine.deltaLightDir, scanLine.clip_dx));
			Common::Add_Vec3_By_Vec3(scanLine.curHVector, scanLine.curHVector, Common::Multiply_Vec3_By_K(scanLine.deltaHVector, scanLine.clip_dx));
		}
	}

	void RasNormalMap::RaterizeAdvancePixel( SScanLine& scanLine )
	{
		Common::Add_Vec2_By_Vec2(scanLine.curUV, scanLine.curUV, scanLine.deltaUV);
		Common::Add_Vec3_By_Vec3(scanLine.curLightDir, scanLine.curLightDir, scanLine.deltaLightDir);
		Common::Add_Vec3_By_Vec3(scanLine.curHVector, scanLine.curHVector, scanLine.deltaHVector);
	}

	void RasNormalMap::RaterizeAdvanceLine( SScanLinesData& scanLineData )
	{
		Common::Add_Vec2_By_Vec2(scanLineData.curUV_L, scanLineData.curUV_L, scanLineData.duv_L);
		Common::Add_Vec2_By_Vec2(scanLineData.curUV_R, scanLineData.curUV_R, scanLineData.duv_R);
		Common::Add_Vec3_By_Vec3(scanLineData.curLightDir_L, scanLineData.curLightDir_L, scanLineData.dLightDir_L);
		Common::Add_Vec3_By_Vec3(scanLineData.curLightDir_R, scanLineData.curLightDir_R, scanLineData.dLightDir_R);
		Common::Add_Vec3_By_Vec3(scanLineData.curHVector_L, scanLineData.curHVector_L, scanLineData.dHVector_L);
		Common::Add_Vec3_By_Vec3(scanLineData.curHVector_R, scanLineData.curHVector_R, scanLineData.dHVector_R);
	}

	void RasNormalMap::RasterizePixel( SScanLine& scanLine, const SScanLinesData& rasData )
	{
#if USE_PERSPEC_CORRECT == 1
		//双曲插值最后一步
		float inv_w = 1 / scanLine.zw.y;
		scanLine.finalUV		= scanLine.curUV;
		scanLine.finalLightDir	= scanLine.curLightDir;
		scanLine.finalHVector	= scanLine.curHVector;
		Common::Multiply_Vec2_By_K(scanLine.finalUV, scanLine.finalUV, inv_w);
		Common::Multiply_Vec3_By_K(scanLine.finalLightDir, scanLine.finalLightDir, inv_w);
		Common::Multiply_Vec3_By_K(scanLine.finalHVector, scanLine.finalHVector, inv_w);
#else
		scanLine.finalUV = scanLine.curUV;
		scanLine.finalLightDir = scanLine.curLightDir;
		scanLine.finalHVector = scanLine.curHVector;
#endif

		// Don't forget to normalize!
		scanLine.finalLightDir.Normalize();
		scanLine.finalHVector.Normalize();

		scanLine.pFragmeng->texLod = rasData.texLod;
		scanLine.pFragmeng->uv = scanLine.finalUV;
		scanLine.pFragmeng->lightDirTS = scanLine.finalLightDir;
		scanLine.pFragmeng->hVectorTS = scanLine.finalHVector;

#if USE_MULTI_THREAD == 1
		scanLine.pFragmeng->bActive = true;
#else
		FragmentPS(*scanLine.pFragmeng);
#endif
	}

	void RasNormalMap::FragmentPS( SFragment& frag )
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

		SLightingContext_NormalMap lc;
		lc.uv = &frag.uv;
		lc.lightDirTS = &frag.lightDirTS;
		lc.hVectorTS = &frag.hVectorTS;

		DoPerPixelLighting(lightColor, &lc, pMaterial);

		texColor *= lightColor;
		texColor.Saturate();

		SColor destPixelColor;
		destPixelColor.SetAsInt(*frag.finalColor);

		DoAlphaBlending(destPixelColor, texColor, destPixelColor, pMaterial);

		*frag.finalColor = destPixelColor.GetAsInt();

#if USE_PROFILER == 1
		g_env.profiler->AddRenderedPixel();
#endif
	}

	void RasNormalMap::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, context);
	}
}