#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"

namespace SR
{
	void RasWireFrame::RasterizeTriangleList( const VertexBuffer& workingVB, SRenderObj& obj )
	{
		//each triangle
		FaceList& faces = obj.faces;
		size_t nFace = faces.size();

		for (size_t iFace=0; iFace<nFace; ++iFace)
		{
			if(faces[iFace].IsBackface)
				continue;

			const Index idx1 = faces[iFace].index1;
			const Index idx2 = faces[iFace].index2;
			const Index idx3 = faces[iFace].index3;

			const VEC4& p0 = workingVB[idx1].pos;
			const VEC4& p1 = workingVB[idx2].pos;
			const VEC4& p2 = workingVB[idx3].pos;

			assert(workingVB[idx1].bActive && workingVB[idx2].bActive && workingVB[idx3].bActive && "Shit, this can't be true!");

			//each line
			RenderUtil::DrawLine_Bresenahams((int)p0.x, (int)p0.y, (int)p1.x, (int)p1.y, SColor::WHITE, true);
			RenderUtil::DrawLine_Bresenahams((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, SColor::WHITE, true);
			RenderUtil::DrawLine_Bresenahams((int)p0.x, (int)p0.y, (int)p2.x, (int)p2.y, SColor::WHITE, true);
		}
	}

	void RasFlat::RasterizeTriangleList( const VertexBuffer& workingVB, SRenderObj& obj )
	{
		FaceList& faces = obj.faces;
		RenderUtil::SortTris_PainterAlgorithm(workingVB, faces);

		//each triangle
		size_t nFace = faces.size();

		for (size_t iFace=0; iFace<nFace; ++iFace)
		{
			if(faces[iFace].IsBackface)
				continue;

			const Index idx1 = faces[iFace].index1;
			const Index idx2 = faces[iFace].index2;
			const Index idx3 = faces[iFace].index3;

			const SVertex& vert0 = workingVB[idx1];
			const SVertex& vert1 = workingVB[idx2];
			const SVertex& vert2 = workingVB[idx3];

			assert(vert0.bActive && vert1.bActive && vert2.bActive && "Shit, this can't be true!");

			RenderUtil::DrawTriangle_Scanline(&vert0, &vert1, &vert2, faces[iFace].color);			
		}
	}

	void RasFlat::DoLighting( VertexBuffer& workingVB, SRenderObj& obj, const SDirectionLight& light )
	{
		///Flat shade基于面法线
		VEC3 lightDir = light.dir;
		lightDir.Neg();
		for (size_t iFace=0; iFace<obj.faces.size(); ++iFace)
		{
			SFace& face = obj.faces[iFace];

			if(face.IsBackface)
				continue;

			//在世界空间进行光照
			VEC3 worldNormal = Common::Transform_Vec3_By_Mat44(face.faceNormal, obj.matWorldIT, false).GetVec3();
			worldNormal.Normalize();
			float nl = Common::DotProduct_Vec3_By_Vec3(worldNormal, lightDir);

			face.color = SColor::BLACK;
			if(nl > 0)
			{
				face.color = light.color * nl ;
			}
		}
	}


	void RasGouraud::RasterizeTriangleList( const VertexBuffer& workingVB, SRenderObj& obj )
	{
		FaceList& faces = obj.faces;

		//each triangle
		size_t nFace = faces.size();

		for (size_t iFace=0; iFace<nFace; ++iFace)
		{
			if(faces[iFace].IsBackface)
				continue;

			const Index idx1 = faces[iFace].index1;
			const Index idx2 = faces[iFace].index2;
			const Index idx3 = faces[iFace].index3;

			const SVertex& vert0 = workingVB[idx1];
			const SVertex& vert1 = workingVB[idx2];
			const SVertex& vert2 = workingVB[idx3];

			assert(vert0.bActive && vert1.bActive && vert2.bActive && "Shit, this can't be true!");

			RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, false, nullptr);			
		}
	}

	void RasGouraud::DoLighting( VertexBuffer& workingVB, SRenderObj& obj, const SDirectionLight& light )
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
			VEC3 worldNormal = Common::Transform_Vec3_By_Mat44(vert.normal, obj.matWorldIT, false).GetVec3();
			worldNormal.Normalize();
			float nl = Common::DotProduct_Vec3_By_Vec3(worldNormal, lightDir);

			vert.color = SColor::BLACK;
			if(nl > 0)
			{
				vert.color = light.color * nl;
			}
		}
	}


	void RasTextured::RasterizeTriangleList( const VertexBuffer& workingVB, SRenderObj& obj )
	{
		FaceList& faces = obj.faces;

		//each triangle
		size_t nFace = faces.size();

		for (size_t iFace=0; iFace<nFace; ++iFace)
		{
			if(faces[iFace].IsBackface)
				continue;

			const Index idx1 = faces[iFace].index1;
			const Index idx2 = faces[iFace].index2;
			const Index idx3 = faces[iFace].index3;

			const SVertex& vert0 = workingVB[idx1];
			const SVertex& vert1 = workingVB[idx2];
			const SVertex& vert2 = workingVB[idx3];

			assert(vert0.bActive && vert1.bActive && vert2.bActive && "Shit, this can't be true!");

			RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, true, &obj.texture);			
		}
	}

}