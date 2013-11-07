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
}