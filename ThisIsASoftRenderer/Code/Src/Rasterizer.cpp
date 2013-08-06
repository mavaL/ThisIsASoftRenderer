#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"

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
		VEC3 lightDir = g_env.renderer->m_testLight.dir;
		lightDir.Neg();

		const SColor ambientColor = g_env.renderer->m_ambientColor * obj.m_pMaterial->ambient;
		const SColor tmpDiffuse = g_env.renderer->m_testLight.color * obj.m_pMaterial->diffuse;

		for (size_t iFace=0; iFace<workingFaces.size(); ++iFace)
		{
			SFace& face = workingFaces[iFace];

			if(face.IsBackface || face.bCulled)
				continue;

			//在世界空间进行光照
			VEC3 worldNormal = Common::Transform_Vec3_By_Mat44(face.faceNormal, obj.m_matWorldIT, false).GetVec3();
			worldNormal.Normalize();
			float nl = Common::DotProduct_Vec3_By_Vec3(worldNormal, lightDir);

			//use half-lambert?
			if(obj.m_pMaterial->bUseHalfLambert)
			{
				nl = pow(nl * 0.5f + 0.5f, 2);
				face.color = tmpDiffuse * nl;
			}
			else
			{
				face.color = SColor::BLACK;
				if(nl > 0)
				{
					face.color = tmpDiffuse * nl;
				}
			}

			//环境光
			face.color += ambientColor;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	void RasGouraud::_RasterizeTriangle( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context )
	{
		RenderUtil::DrawTriangle_Scanline_V2(&vert0, &vert1, &vert2, false, true, context);
	}

	void RasGouraud::DoPerVertexLighting( VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj )
	{
		///Gouraud shade基于逐顶点法线
		VEC3 lightDir = g_env.renderer->m_testLight.dir;
		lightDir.Neg();

		const SColor ambientColor = g_env.renderer->m_ambientColor * obj.m_pMaterial->ambient;
		const SColor tmpDiffuse = g_env.renderer->m_testLight.color * obj.m_pMaterial->diffuse;

		for (size_t iVert=0; iVert<workingVB.size(); ++iVert)
		{
			SVertex& vert = workingVB[iVert];

			if(!vert.bActive)
				continue;

			//在世界空间进行光照
			VEC3 worldNormal = Common::Transform_Vec3_By_Mat44(vert.normal, obj.m_matWorldIT, false).GetVec3();
			worldNormal.Normalize();
			float nl = Common::DotProduct_Vec3_By_Vec3(worldNormal, lightDir);

			//use half-lambert?
			if(obj.m_pMaterial->bUseHalfLambert)
			{
				nl = pow(nl * 0.5f + 0.5f, 2);
				vert.color = tmpDiffuse * nl;
			}
			else
			{
				vert.color = SColor::BLACK;
				if(nl > 0)
				{
					vert.color = tmpDiffuse * nl;
				}
			}

			//环境光
			vert.color += ambientColor;
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
		VEC3 lightDir = g_env.renderer->m_testLight.dir;
		lightDir.Neg();

		const SColor ambientColor = g_env.renderer->m_ambientColor * pMaterial->ambient;
		const SColor tmpDiffuse = g_env.renderer->m_testLight.color * pMaterial->diffuse;
		const SColor tmpSpec = g_env.renderer->m_testLight.color * pMaterial->specular;

		//在世界空间进行光照
		VEC3 N = worldNormal;
		N.Normalize();
		float nl = Common::DotProduct_Vec3_By_Vec3(N, lightDir);

		//use half-lambert?
		if(pMaterial->bUseHalfLambert)
		{
			nl = pow(nl * 0.5f + 0.5f, 2);
			result = tmpDiffuse * nl;
		}
		else
		{
			result = SColor::BLACK;
			if(nl > 0)
			{
				result = tmpDiffuse * nl;
			}
		}

		//环境光
		result += ambientColor;

		//高光
		const VEC3& camPos = g_env.renderer->m_camera.GetPos().GetVec3();
		VEC3 V = Common::Sub_Vec3_By_Vec3(camPos, worldPos);
		V.Normalize();
		VEC3 H = Common::Add_Vec3_By_Vec3(V, lightDir);
		H.Normalize();

		float spec = pow(max(Common::DotProduct_Vec3_By_Vec3(N, H), 0), pMaterial->shiness);
		result += tmpSpec * spec;
	}

	///////////////////////////////////////////////////////////////////////////////////////////

}