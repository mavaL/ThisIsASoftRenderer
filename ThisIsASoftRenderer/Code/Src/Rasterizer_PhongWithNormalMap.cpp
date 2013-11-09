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
	void RasPhongWithNormalMap::DoPerVertexLighting( VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj )
	{
		const VEC3& camPos = g_env.renderer->m_camera.GetPos().GetVec3();

		for (size_t iVert=0; iVert<obj.m_verts.size(); ++iVert)
		{
			SVertex& vert = obj.m_verts[iVert];

			// Get the matrix which transforms vector from object space to tangent space
			MAT44 matTBN;
			matTBN.SetRow(0, VEC4(vert.tangent, 0));
			matTBN.SetRow(1, VEC4(vert.binormal, 0));
			matTBN.SetRow(2, VEC4(vert.normal, 0));

			MAT44 matInvWorld = obj.m_matWorld.Inverse();
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

	void RasPhongWithNormalMap::DoPerPixelLighting( SColor& result, void* pLightingContext, const SMaterial* pMaterial )
	{
		SLightingContext_PhongWithNormalMap* plc = (SLightingContext_PhongWithNormalMap*)pLightingContext;

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

	void RasPhongWithNormalMap::LerpVertexAttributes( SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type )
	{
		RasBlinnPhong::LerpVertexAttributes(dest, src1, src2, t, type);

		if (type == eLerpType_Linear)
		{			
			Ext::LinearLerp(dest->lightDirTS	, src1->lightDirTS	, src2->lightDirTS	, t);			
			Ext::LinearLerp(dest->halfAngleTS	, src1->halfAngleTS	, src2->halfAngleTS	, t);
		} 
		else
		{
			float w0 = src1->pos.w;
			float w1 = src2->pos.w;

			Ext::HyperLerp(dest->lightDirTS		, src1->lightDirTS	, src2->lightDirTS	, t, w0, w1);
			Ext::HyperLerp(dest->halfAngleTS	, src1->halfAngleTS	, src2->halfAngleTS	, t, w0, w1);
			//双曲插值最后一步
			float inv_w = 1 / dest->pos.w;
			Common::Multiply_Vec3_By_K(dest->lightDirTS	, dest->lightDirTS	, inv_w);
			Common::Multiply_Vec3_By_K(dest->halfAngleTS, dest->halfAngleTS	, inv_w);
		}
	}

	void RasPhongWithNormalMap::RasTriangleSetup( SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type )
	{
		RasBlinnPhong::RasTriangleSetup(rasData, v0, v1, v2, type);

		const VEC4& p0 = v0->pos;
		const VEC4& p1 = v1->pos;
		const VEC4& p2 = v2->pos;

#if USE_PERSPEC_CORRECT == 1
		const VEC3 lightDir0(v0->lightDirTS.x*p0.w, v0->lightDirTS.y*p0.w, v0->lightDirTS.z*p0.w);
		const VEC3 lightDir1(v1->lightDirTS.x*p1.w, v1->lightDirTS.y*p1.w, v1->lightDirTS.z*p1.w);
		const VEC3 lightDir2(v2->lightDirTS.x*p2.w, v2->lightDirTS.y*p2.w, v2->lightDirTS.z*p2.w);

		const VEC3 hVector0(v0->halfAngleTS.x*p0.w, v0->halfAngleTS.y*p0.w, v0->halfAngleTS.z*p0.w);
		const VEC3 hVector1(v1->halfAngleTS.x*p1.w, v1->halfAngleTS.y*p1.w, v1->halfAngleTS.z*p1.w);
		const VEC3 hVector2(v2->halfAngleTS.x*p2.w, v2->halfAngleTS.y*p2.w, v2->halfAngleTS.z*p2.w);
#else
		const VEC3 lightDir0 = v0->lightDirTS;
		const VEC3 lightDir1 = v1->lightDirTS;
		const VEC3 lightDir2 = v2->lightDirTS;

		const VEC3 hVector0 = v0->halfAngleTS;
		const VEC3 hVector1 = v1->halfAngleTS;
		const VEC3 hVector2 = v2->halfAngleTS;
#endif

		if (type == eTriangleShape_Bottom)
		{
			//光源方向(切空间)及其增量
			rasData.curLightDir_L = lightDir0;
			rasData.curLightDir_R = lightDir0;
			rasData.dLightDir_L.Set((lightDir1.x-lightDir0.x)*rasData.inv_dy_L, (lightDir1.y-lightDir0.y)*rasData.inv_dy_L, (lightDir1.z-lightDir0.z)*rasData.inv_dy_L);
			rasData.dLightDir_R.Set((lightDir2.x-lightDir0.x)*rasData.inv_dy_R, (lightDir2.y-lightDir0.y)*rasData.inv_dy_R, (lightDir2.z-lightDir0.z)*rasData.inv_dy_R);
			// Half angle vector(切空间)及其增量
			rasData.curHVector_L = hVector0;
			rasData.curHVector_R = hVector0;
			rasData.dHVector_L.Set((hVector1.x-hVector0.x)*rasData.inv_dy_L, (hVector1.y-hVector0.y)*rasData.inv_dy_L, (hVector1.z-hVector0.z)*rasData.inv_dy_L);
			rasData.dHVector_R.Set((hVector2.x-hVector0.x)*rasData.inv_dy_R, (hVector2.y-hVector0.y)*rasData.inv_dy_R, (hVector2.z-hVector0.z)*rasData.inv_dy_R);
		}
		else
		{
			//光源方向(切空间)及其增量
			rasData.curLightDir_L = lightDir0;
			rasData.curLightDir_R = lightDir2;
			rasData.dLightDir_L.Set((lightDir1.x-lightDir0.x)*rasData.inv_dy_L, (lightDir1.y-lightDir0.y)*rasData.inv_dy_L, (lightDir1.z-lightDir0.z)*rasData.inv_dy_L);
			rasData.dLightDir_R.Set((lightDir1.x-lightDir2.x)*rasData.inv_dy_R, (lightDir1.y-lightDir2.y)*rasData.inv_dy_R, (lightDir1.z-lightDir2.z)*rasData.inv_dy_R);
			// Half angle vector(切空间)及其增量
			rasData.curHVector_L = hVector0;
			rasData.curHVector_R = hVector2;
			rasData.dHVector_L.Set((hVector1.x-hVector0.x)*rasData.inv_dy_L, (hVector1.y-hVector0.y)*rasData.inv_dy_L, (hVector1.z-hVector0.z)*rasData.inv_dy_L);
			rasData.dHVector_R.Set((hVector1.x-hVector2.x)*rasData.inv_dy_R, (hVector1.y-hVector2.y)*rasData.inv_dy_R, (hVector1.z-hVector2.z)*rasData.inv_dy_R);
		}

		//裁剪区域裁剪y
		if(rasData.bClipY)
		{
			Common::Add_Vec3_By_Vec3(rasData.curLightDir_L, rasData.curLightDir_L, Common::Multiply_Vec3_By_K(rasData.dLightDir_L, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curLightDir_R, rasData.curLightDir_R, Common::Multiply_Vec3_By_K(rasData.dLightDir_R, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curHVector_L, rasData.curHVector_L, Common::Multiply_Vec3_By_K(rasData.dHVector_L, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curHVector_R, rasData.curHVector_R, Common::Multiply_Vec3_By_K(rasData.dHVector_R, rasData.clip_dy));
		}
	}

	void RasPhongWithNormalMap::RasLineSetup( SScanLine& scanLine, const SScanLinesData& rasData )
	{
		RasBlinnPhong::RasLineSetup(scanLine, rasData);

		scanLine.deltaLightDir.Set((rasData.curLightDir_R.x-rasData.curLightDir_L.x)*scanLine.inv_dx, (rasData.curLightDir_R.y-rasData.curLightDir_L.y)*scanLine.inv_dx, (rasData.curLightDir_R.z-rasData.curLightDir_L.z)*scanLine.inv_dx);
		scanLine.deltaHVector.Set((rasData.curHVector_R.x-rasData.curHVector_L.x)*scanLine.inv_dx, (rasData.curHVector_R.y-rasData.curHVector_L.y)*scanLine.inv_dx, (rasData.curHVector_R.z-rasData.curHVector_L.z)*scanLine.inv_dx);

		scanLine.curLightDir = rasData.curLightDir_L;
		scanLine.curHVector = rasData.curHVector_L;

		//裁剪区域裁剪x
		if(scanLine.bClipX)
		{
			Common::Add_Vec3_By_Vec3(scanLine.curLightDir, scanLine.curLightDir, Common::Multiply_Vec3_By_K(scanLine.deltaLightDir, scanLine.clip_dx));
			Common::Add_Vec3_By_Vec3(scanLine.curHVector, scanLine.curHVector, Common::Multiply_Vec3_By_K(scanLine.deltaHVector, scanLine.clip_dx));
		}
	}

	void RasPhongWithNormalMap::RasterizePixel( SScanLine& scanLine, const SScanLinesData& rasData )
	{
#if USE_PERSPEC_CORRECT == 1
		//双曲插值最后一步
		float inv_w = 1 / scanLine.w;
		scanLine.finalLightDir.Set(scanLine.curLightDir.x*inv_w, scanLine.curLightDir.y*inv_w, scanLine.curLightDir.z*inv_w);
		scanLine.finalHVector.Set(scanLine.curHVector.x*inv_w, scanLine.curHVector.y*inv_w, scanLine.curHVector.z*inv_w);
#else
		scanLine.finalLightDir = scanLine.curLightDir;
		scanLine.finalHVector = scanLine.curHVector;
#endif

		// Don't forget to normalize!
		scanLine.finalLightDir.Normalize();
		scanLine.finalHVector.Normalize();

		scanLine.pFragmeng->lightDirTS = scanLine.finalLightDir;
		scanLine.pFragmeng->hVectorTS = scanLine.finalHVector;

		RasBlinnPhong::RasterizePixel(scanLine, rasData);
	}

	void RasPhongWithNormalMap::FragmentPS( SFragment& frag )
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

		SLightingContext_PhongWithNormalMap lc;
		lc.uv = &frag.uv;
		lc.worldNormal = &frag.normal;
		lc.worldPos = &frag.worldPos;
		lc.lightDirTS = &frag.lightDirTS;
		lc.hVectorTS = &frag.hVectorTS;

		g_env.renderer->GetCurRas()->DoPerPixelLighting(lightColor, &lc, pMaterial);
		texColor *= lightColor;
		texColor.Saturate();
		*frag.finalColor = texColor.GetAsInt();

#if USE_PROFILER == 1
		g_env.profiler->AddRenderedPixel();
#endif
	}
}