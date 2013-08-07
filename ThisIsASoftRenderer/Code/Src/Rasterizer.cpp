#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"
#include "RenderObject.h"

namespace SR
{
	void Rasterizer::RasterizeTriangleList( SRenderContext& context )
	{
		FaceList& faces = *context.faces;
		VertexBuffer& verts = *context.verts;

		//each triangle
		int nFace = (int)faces.size();

#if USE_OPENMP == 1
#pragma omp parallel for
#endif

		for (int iFace=0; iFace<nFace; ++iFace)
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

			_RasterizeTriangle(vert0, vert1, vert2, face, context);

			++g_env.renderer->m_frameStatics.nRenderedFace;
		}
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
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	void RasFlat::RasterizeTriangleList( SRenderContext& context )
	{
		FaceList& faces = *context.faces;
		VertexBuffer& verts = *context.verts;

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

			RenderUtil::DoLambertLighting(face.color, worldNormal, obj.m_pMaterial);
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

			RenderUtil::DoLambertLighting(vert.color, worldNormal, obj.m_pMaterial);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	void RasTexturedGouraud::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, true, false, context);
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	void RasBlinnPhong::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, true, true, context);
	}

	void RasBlinnPhong::DoPerPixelLighting(SColor& result, const VEC3& worldPos, const VEC3& worldNormal, const SMaterial* pMaterial)
	{
		VEC3 N = worldNormal;
		N.Normalize();

		RenderUtil::DoLambertLighting(result, N, pMaterial);

		//高光
		const VEC3& camPos = g_env.renderer->m_camera.GetPos().GetVec3();
		VEC3 V = Common::Sub_Vec3_By_Vec3(camPos, worldPos);
		V.Normalize();
		VEC3 H = Common::Add_Vec3_By_Vec3(V, g_env.renderer->m_testLight.neg_dir);
		H.Normalize();

		float spec = pow(max(Common::DotProduct_Vec3_By_Vec3(N, H), 0), pMaterial->shiness);
		result += pMaterial->specular * spec;
	}

	///////////////////////////////////////////////////////////////////////////////////////////

}