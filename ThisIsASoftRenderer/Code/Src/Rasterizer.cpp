#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"

namespace SR
{
	void RasWireFrame::RasterizeTriangleList( SRenderContext& context )
	{
		FaceList& faces = *context.faces;
		VertexBuffer& verts = *context.verts;

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

			const VEC4& p0 = verts[idx1].pos;
			const VEC4& p1 = verts[idx2].pos;
			const VEC4& p2 = verts[idx3].pos;

			assert(verts[idx1].bActive && verts[idx2].bActive && verts[idx3].bActive && "Shit, this can't be true!");

			//each line
			RenderUtil::DrawLine_Bresenahams((int)p0.x, (int)p0.y, (int)p1.x, (int)p1.y, SColor::WHITE, true);
			RenderUtil::DrawLine_Bresenahams((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, SColor::WHITE, true);
			RenderUtil::DrawLine_Bresenahams((int)p0.x, (int)p0.y, (int)p2.x, (int)p2.y, SColor::WHITE, true);

			++g_env.renderer->m_frameStatics.nRenderedFace;
		}
	}

	void RasFlat::RasterizeTriangleList( SRenderContext& context )
	{
		FaceList& faces = *context.faces;
		VertexBuffer& verts = *context.verts;

		RenderUtil::SortTris_PainterAlgorithm(verts, faces);

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

			RenderUtil::DrawTriangle_Scanline(&vert0, &vert1, &vert2, faces[iFace].color);

			++g_env.renderer->m_frameStatics.nRenderedFace;
		}
	}

	void RasFlat::DoLighting( VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj, const SDirectionLight& light )
	{
		///Flat shade基于面法线
		VEC3 lightDir = light.dir;
		lightDir.Neg();
		for (size_t iFace=0; iFace<workingFaces.size(); ++iFace)
		{
			SFace& face = workingFaces[iFace];

			if(face.IsBackface || face.bCulled)
				continue;

			//在世界空间进行光照
			VEC3 worldNormal = Common::Transform_Vec3_By_Mat44(face.faceNormal, obj.m_matWorldIT, false).GetVec3();
			worldNormal.Normalize();
			float nl = Common::DotProduct_Vec3_By_Vec3(worldNormal, lightDir);

			face.color = SColor::BLACK;
			if(nl > 0)
			{
				face.color = light.color * nl ;
			}
		}
	}

	void RasGouraud::RasterizeTriangleList( SRenderContext& context )
	{
		FaceList& faces = *context.faces;
		VertexBuffer& verts = *context.verts;

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

			RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, false, context, m_scanLineData);

			++g_env.renderer->m_frameStatics.nRenderedFace;
		}
	}

	void RasGouraud::DoLighting( VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj, const SDirectionLight& light )
	{
		///Gouraud shade基于逐顶点法线
		VEC3 lightDir = light.dir;
		lightDir.Neg();

		for (size_t iVert=0; iVert<workingVB.size(); ++iVert)
		{
			SVertex& vert = workingVB[iVert];

			if(!vert.bActive)
				continue;

			//在世界空间进行光照
			VEC3 worldNormal = Common::Transform_Vec3_By_Mat44(vert.normal, obj.m_matWorldIT, false).GetVec3();
			worldNormal.Normalize();
			float nl = Common::DotProduct_Vec3_By_Vec3(worldNormal, lightDir);

			vert.color = SColor::BLACK;
			if(nl > 0)
			{
				vert.color = light.color * nl;
			}
		}
	}


	void RasTexturedGouraud::RasterizeTriangleList( SRenderContext& context )
	{
		FaceList& faces = *context.faces;
		VertexBuffer& verts = *context.verts;

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

			RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, true, context, m_scanLineData);

			++g_env.renderer->m_frameStatics.nRenderedFace;
		}
	}

}