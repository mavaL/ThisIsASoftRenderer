#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"
#include "RenderUtil.h"
#include "RenderObject.h"
#include "Profiler.h"

namespace SR
{
	void Rasterizer::RasterizeTriangleList( SRenderContext& context )
	{
		FaceList& faces = context.faces;
		VertexBuffer& verts = context.verts;

		//each triangle
		size_t nFace = faces.size();

		for (size_t iFace=0; iFace<nFace; ++iFace)
		{
			const SFace& face = faces[iFace];
			if(face.IsBackface || face.bCulled)
				continue;

			const Index idx1 = faces[iFace].index1;
			const Index idx2 = faces[iFace].index2;
			const Index idx3 = faces[iFace].index3;

			const SVertex& vert0 = verts[idx1];
			const SVertex& vert1 = verts[idx2];
			const SVertex& vert2 = verts[idx3];

			assert(vert0.bActive && vert1.bActive && vert2.bActive && "Shit, this can't be true!");

			// Determine mip level of this triangle
			const SMaterial* pMat = context.pMaterial;
			if (pMat->pDiffuseMap && pMat->pDiffuseMap->bMipMap)
			{
				const float areaScreen = Ext::CalcAreaOfTriangle(
					VEC2(vert0.pos.x, vert0.pos.y),
					VEC2(vert1.pos.x, vert1.pos.y),
					VEC2(vert2.pos.x, vert2.pos.y));

				float ratio = face.texArea / areaScreen;
				context.texLod = log(ratio) / log(4.0f) + pMat->pDiffuseMap->lodBias;
				//clamp it
				if(context.texLod < 0)
					context.texLod = 0;
				if(context.texLod >= pMat->pDiffuseMap->GetMipCount())
					context.texLod = pMat->pDiffuseMap->GetMipCount() - 1;
			}

			_RasterizeTriangle(vert0, vert1, vert2, face, context);
		}
	}

	void Rasterizer::LerpVertexAttributes(SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type)
	{
		Ext::LinearLerp(dest->pos, src1->pos, src2->pos, t);
	}

	void Rasterizer::RasTriangleSetup(SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type)
	{
		const VEC4& p0 = v0->pos;
		const VEC4& p1 = v1->pos;
		const VEC4& p2 = v2->pos;

		if (type == eTriangleShape_Bottom)
		{
			//左上填充规则:上
			rasData.curY = Ext::Ceil32_Fast(p0.y), rasData.endY = Ext::Ceil32_Fast(p1.y) - 1;
			//位置坐标及增量
			rasData.inv_dy_L = 1.0f / (p1.y - p0.y);
			rasData.inv_dy_R = 1.0f / (p2.y - p0.y);
			rasData.curP_L.Set(p0.x, p0.z, p0.w);
			rasData.curP_R.Set(p0.x, p0.z, p0.w);
			rasData.dp_L.Set((p1.x-p0.x)*rasData.inv_dy_L, (p1.z-p0.z)*rasData.inv_dy_L, (p1.w-p0.w)*rasData.inv_dy_L);
			rasData.dp_R.Set((p2.x-p0.x)*rasData.inv_dy_R, (p2.z-p0.z)*rasData.inv_dy_R, (p2.w-p0.w)*rasData.inv_dy_R);
		}
		else
		{
			//左上填充规则:上
			rasData.curY = Ext::Ceil32_Fast(p0.y), rasData.endY = Ext::Ceil32_Fast(p1.y) - 1;
			//屏幕坐标及增量
			rasData.inv_dy_L = 1.0f / (p1.y - p0.y);
			rasData.inv_dy_R = 1.0f / (p1.y - p2.y);
			rasData.curP_L.Set(p0.x, p0.z, p0.w);
			rasData.curP_R.Set(p2.x, p2.z, p2.w);
			rasData.dp_L.Set((p1.x-p0.x)*rasData.inv_dy_L, (p1.z-p0.z)*rasData.inv_dy_L, (p1.w-p0.w)*rasData.inv_dy_L);
			rasData.dp_R.Set((p1.x-p2.x)*rasData.inv_dy_R, (p1.z-p2.z)*rasData.inv_dy_R, (p1.w-p2.w)*rasData.inv_dy_R);
		}

		//裁剪区域裁剪y
		if(rasData.curY < min_clip_y)
		{
			float dy = min_clip_y - rasData.curY;
			rasData.curY = min_clip_y;
			Common::Add_Vec3_By_Vec3(rasData.curP_L, rasData.curP_L, Common::Multiply_Vec3_By_K(rasData.dp_L, dy));
			Common::Add_Vec3_By_Vec3(rasData.curP_R, rasData.curP_R, Common::Multiply_Vec3_By_K(rasData.dp_R, dy));
		}
		if(rasData.endY > max_clip_y)
		{
			rasData.endY = max_clip_y;
		}
	}

	void Rasterizer::RasLineClipX(SScanLine& scanLine, const SScanLinesData& rasData)
	{
		float invdx = 1.0f / (scanLine.lineX1 - scanLine.lineX0);
		scanLine.deltaClr.Set((rasData.clr_R.x-rasData.clr_L.x)*invdx, (rasData.clr_R.y-rasData.clr_L.y)*invdx, (rasData.clr_R.z-rasData.clr_L.z)*invdx);
		scanLine.deltaUV.Set((rasData.curUV_R.x-rasData.curUV_L.x)*invdx, (rasData.curUV_R.y-rasData.curUV_L.y)*invdx);
		scanLine.deltaPW.Set((rasData.curPW_R.x-rasData.curPW_L.x)*invdx, (rasData.curPW_R.y-rasData.curPW_L.y)*invdx, (rasData.curPW_R.z-rasData.curPW_L.z)*invdx);
		scanLine.deltaN.Set((rasData.curN_R.x-rasData.curN_L.x)*invdx, (rasData.curN_R.y-rasData.curN_L.y)*invdx, (rasData.curN_R.z-rasData.curN_L.z)*invdx);
		scanLine.dz = (rasData.curP_R.y - rasData.curP_L.y) * invdx;
		scanLine.dw = (rasData.curP_R.z - rasData.curP_L.z) * invdx;

		scanLine.curClr = rasData.clr_L;
		scanLine.curUV = rasData.curUV_L;
		scanLine.curPW = rasData.curPW_L;
		scanLine.curN = rasData.curN_L;
		scanLine.z = rasData.curP_L.y;
		scanLine.w = rasData.curP_L.z;

		//裁剪区域裁剪x
		if(scanLine.lineX0 < min_clip_x)
		{
			const float dx = min_clip_x-scanLine.lineX0;
			Common::Add_Vec3_By_Vec3(scanLine.curClr, scanLine.curClr, Common::Multiply_Vec3_By_K(scanLine.deltaClr, dx));
			Common::Add_Vec2_By_Vec2(scanLine.curUV, scanLine.curUV, Common::Multiply_Vec2_By_K(scanLine.deltaUV, dx));
			Common::Add_Vec3_By_Vec3(scanLine.curPW, scanLine.curPW, Common::Multiply_Vec3_By_K(scanLine.deltaPW, dx));
			Common::Add_Vec3_By_Vec3(scanLine.curN, scanLine.curN, Common::Multiply_Vec3_By_K(scanLine.deltaN, dx));
			scanLine.lineX0 = min_clip_x;
			scanLine.z += dx*scanLine.dz;
			scanLine.w += dx*scanLine.dw;
		}
	}

	void Rasterizer::RasScanLine(SScanLine& scanLine, const SScanLinesData& rasData, const SRenderContext& context)
	{
		if(bTextured)
		{
#if USE_PERSPEC_CORRECT == 1
			//双曲插值最后一步
			float inv_w = 1 / scanLine.w;
			scanLine.finalUV.Set(scanLine.curUV.x*inv_w, scanLine.curUV.y*inv_w);
			scanLine.finalPW.Set(scanLine.curPW.x*inv_w, scanLine.curPW.y*inv_w, scanLine.curPW.z*inv_w);
			scanLine.finalN.Set(scanLine.curN.x*inv_w, scanLine.curN.y*inv_w, scanLine.curN.z*inv_w);
			scanLine.curPixelClr.Set(scanLine.curClr.x*inv_w, scanLine.curClr.y*inv_w, scanLine.curClr.z*inv_w);
#else
			scanLine.finalUV = scanLine.curUV;
			scanLine.finalPW = scanLine.curPW;
			scanLine.finalN = scanLine.curN;
			scanLine.curPixelClr = scanLine.curClr;
#endif			
			if(context.pMaterial->pDiffuseMap && context.pMaterial->bUseBilinearSampler)
			{
				context.pMaterial->pDiffuseMap->Tex2D_Bilinear(scanLine.finalUV, scanLine.pixelColor, context.texLod);
			}
			else if(context.pMaterial->pDiffuseMap)
			{
				context.pMaterial->pDiffuseMap->Tex2D_Point(scanLine.finalUV, scanLine.pixelColor, context.texLod);
			}
			else
			{
				scanLine.pixelColor = SColor::WHITE;
			}

			if(bPerPixel)
			{
				g_env.renderer->m_curRas->DoPerPixelLighting(
					scanLine.curPixelClr, scanLine.finalPW, scanLine.finalN, scanLine.finalUV, context.pMaterial);
				scanLine.pixelColor *= scanLine.curPixelClr;
			}
			else
			{
				scanLine.pixelColor *= scanLine.curPixelClr;
			}
		}
		else
		{
			scanLine.pixelColor.Set(scanLine.curClr.x, scanLine.curClr.y, scanLine.curClr.z);
		}

		//NB: 这三句在并行中应与if(z < zBuffer[curX])这句达成原子性
		zBuffer[curX] = scanLine.z;
		scanLine.pixelColor.Saturate();
		destBuffer[curX] = scanLine.pixelColor.GetAsInt();

#if USE_PROFILER == 1
		g_env.profiler->AddRenderedPixel();
#endif
	}

	void Rasterizer::RasAdvanceToNextPixel()
	{
		Common::Add_Vec3_By_Vec3(scanLine.curClr, scanLine.curClr, scanLine.deltaClr);
		Common::Add_Vec2_By_Vec2(scanLine.curUV, scanLine.curUV, scanLine.deltaUV);
		Common::Add_Vec3_By_Vec3(scanLine.curPW, scanLine.curPW, scanLine.deltaPW);
		Common::Add_Vec3_By_Vec3(scanLine.curN, scanLine.curN, scanLine.deltaN);
		scanLine.z += scanLine.dz;
		scanLine.w += scanLine.dw;
	}

	void Rasterizer::RasAdvanceToNextScanLine()
	{
		Common::Add_Vec3_By_Vec3(scanLineData.clr_L, scanLineData.clr_L, scanLineData.dclr_L);
		Common::Add_Vec3_By_Vec3(scanLineData.clr_R, scanLineData.clr_R, scanLineData.dclr_R);
		Common::Add_Vec2_By_Vec2(scanLineData.curUV_L, scanLineData.curUV_L, scanLineData.duv_L);
		Common::Add_Vec2_By_Vec2(scanLineData.curUV_R, scanLineData.curUV_R, scanLineData.duv_R);
		Common::Add_Vec3_By_Vec3(scanLineData.curPW_L, scanLineData.curPW_L, scanLineData.dpw_L);
		Common::Add_Vec3_By_Vec3(scanLineData.curPW_R, scanLineData.curPW_R, scanLineData.dpw_R);
		Common::Add_Vec3_By_Vec3(scanLineData.curN_L, scanLineData.curN_L, scanLineData.dn_L);
		Common::Add_Vec3_By_Vec3(scanLineData.curN_R, scanLineData.curN_R, scanLineData.dn_R);
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	void RasWireFrame::_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context)
	{
		const VEC4& p0 = vert0.pos;
		const VEC4& p1 = vert1.pos;
		const VEC4& p2 = vert2.pos;

		//each line
		RenderUtil::DrawLine_Bresenahams(Ext::Floor32_Fast(p0.x), Ext::Floor32_Fast(p0.y), Ext::Floor32_Fast(p1.x), Ext::Floor32_Fast(p1.y), SColor::WHITE, true);
		RenderUtil::DrawLine_Bresenahams(Ext::Floor32_Fast(p1.x), Ext::Floor32_Fast(p1.y), Ext::Floor32_Fast(p2.x), Ext::Floor32_Fast(p2.y), SColor::WHITE, true);
		RenderUtil::DrawLine_Bresenahams(Ext::Floor32_Fast(p0.x), Ext::Floor32_Fast(p0.y), Ext::Floor32_Fast(p2.x), Ext::Floor32_Fast(p2.y), SColor::WHITE, true);

#if USE_PROFILER == 1
		g_env.profiler->AddRenderedFace();
#endif
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	void RasFlat::RasterizeTriangleList( SRenderContext& context )
	{
		FaceList& faces = context.faces;
		VertexBuffer& verts = context.verts;

		RenderUtil::SortTris_PainterAlgorithm(verts, faces);

		__super::RasterizeTriangleList(context);
	}

	void RasFlat::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline(&vert0, &vert1, &vert2, face.color);
	}

	void RasFlat::DoPerVertexLighting( VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj )
	{
		///Flat shade基于面法线
		for (size_t iFace=0; iFace<workingFaces.size(); ++iFace)
		{
			SFace& face = workingFaces[iFace];

			if(face.IsBackface || face.bCulled)
				continue;

			//在世界空间进行光照
			VEC3 worldNormal = Common::Transform_Vec3_By_Mat44(face.faceNormal, obj.m_matWorldIT, false).GetVec3();
			worldNormal.Normalize();

			RenderUtil::DoLambertLighting(face.color, worldNormal, g_env.renderer->m_testLight.neg_dir, obj.m_pMaterial);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	void RasGouraud::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, false, false, context);
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

			RenderUtil::DoLambertLighting(vert.color, worldNormal, g_env.renderer->m_testLight.neg_dir, obj.m_pMaterial);
		}
	}

	void RasGouraud::LerpVertexAttributes( SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type )
	{
		Rasterizer::LerpVertexAttributes(dest, src1, src2, t, type);

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
		if(rasData.curY < min_clip_y)
		{
			float dy = min_clip_y - rasData.curY;
			Common::Add_Vec3_By_Vec3(rasData.clr_L, rasData.clr_L, Common::Multiply_Vec3_By_K(rasData.dclr_L, dy));
			Common::Add_Vec3_By_Vec3(rasData.clr_R, rasData.clr_R, Common::Multiply_Vec3_By_K(rasData.dclr_R, dy));
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	void RasTexturedGouraud::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, true, false, context);
	}

	void RasTexturedGouraud::LerpVertexAttributes( SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type )
	{
		Rasterizer::LerpVertexAttributes(dest, src1, src2, t, type);

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
		Rasterizer::RasTriangleSetup(rasData, v0, v1, v2, type);

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
		if(rasData.curY < min_clip_y)
		{
			float dy = min_clip_y - rasData.curY;
			Common::Add_Vec2_By_Vec2(rasData.curUV_L, rasData.curUV_L, Common::Multiply_Vec2_By_K(rasData.duv_L, dy));
			Common::Add_Vec2_By_Vec2(rasData.curUV_R, rasData.curUV_R, Common::Multiply_Vec2_By_K(rasData.duv_R, dy));
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	void RasBlinnPhong::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, true, true, context);
	}

	void RasBlinnPhong::DoPerPixelLighting(SColor& result, const VEC3& worldPos, const VEC3& worldNormal, const VEC2& uv, const SMaterial* pMaterial)
	{
		VEC3 N = worldNormal;
		N.Normalize();

		RenderUtil::DoLambertLighting(result, N, g_env.renderer->m_testLight.neg_dir, pMaterial);

		//高光
		const VEC3& camPos = g_env.renderer->m_camera.GetPos().GetVec3();
		VEC3 V = Common::Sub_Vec3_By_Vec3(camPos, worldPos);
		V.Normalize();
		VEC3 H = Common::Add_Vec3_By_Vec3(V, g_env.renderer->m_testLight.neg_dir);
		H.Normalize();

		float spec = pow(max(Common::DotProduct_Vec3_By_Vec3(N, H), 0), pMaterial->shiness);
		result += pMaterial->specular * spec;
	}

	void RasBlinnPhong::LerpVertexAttributes( SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type )
	{
		Rasterizer::LerpVertexAttributes(dest, src1, src2, t, type);

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
		if(rasData.curY < min_clip_y)
		{
			float dy = min_clip_y - rasData.curY;
			Common::Add_Vec2_By_Vec2(rasData.curUV_L, rasData.curUV_L, Common::Multiply_Vec2_By_K(rasData.duv_L, dy));
			Common::Add_Vec2_By_Vec2(rasData.curUV_R, rasData.curUV_R, Common::Multiply_Vec2_By_K(rasData.duv_R, dy));
			Common::Add_Vec3_By_Vec3(rasData.curPW_L, rasData.curPW_L, Common::Multiply_Vec3_By_K(rasData.dpw_L, dy));
			Common::Add_Vec3_By_Vec3(rasData.curPW_R, rasData.curPW_R, Common::Multiply_Vec3_By_K(rasData.dpw_R, dy));
			Common::Add_Vec3_By_Vec3(rasData.curN_L, rasData.curN_L, Common::Multiply_Vec3_By_K(rasData.dn_L, dy));
			Common::Add_Vec3_By_Vec3(rasData.curN_R, rasData.curN_R, Common::Multiply_Vec3_By_K(rasData.dn_R, dy));
		}
	}

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

	void RasPhongWithNormalMap::DoPerPixelLighting( SColor& result, const VEC3& worldPos, const VEC3& worldNormal, const VEC2& uv, const SMaterial* pMaterial )
	{
		pMaterial->pNormalMap->Tex2D_Point(uv, result);

		VEC3 N(result.r, result.g, result.b);
		// Expand
		Common::Add_Vec3_By_Vec3(N, N, VEC3(-0.5f,-0.5f,-0.5f));
		Common::Multiply_Vec3_By_K(N, N, 2);
		N.Normalize();

		RenderUtil::DoLambertLighting(result, N, g_env.renderer->m_testLight.neg_dir, pMaterial);

		//高光
		const VEC3& camPos = g_env.renderer->m_camera.GetPos().GetVec3();
		VEC3 V = Common::Sub_Vec3_By_Vec3(camPos, worldPos);
		V.Normalize();
		VEC3 H = Common::Add_Vec3_By_Vec3(V, g_env.renderer->m_testLight.neg_dir);
		H.Normalize();

		float spec = pow(max(Common::DotProduct_Vec3_By_Vec3(N, H), 0), pMaterial->shiness);
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

			// Half angle vector(切空间)及其增量
		}
		else
		{
			
		}
	}
}