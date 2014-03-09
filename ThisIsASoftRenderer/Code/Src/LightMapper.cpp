#include "stdafx.h"
#include "LightMapper.h"
#include "Renderer.h"
#include "Intersection.h"


namespace SR
{
	//------------------------------------------------------------------------------------
	LightMapper::LightMapper()
	{
		m_pLight = new SPointLight;
	}
	//------------------------------------------------------------------------------------
	LightMapper::~LightMapper()
	{
		SAFE_DELETE(m_pLight);
	}
	//------------------------------------------------------------------------------------
	STexture* LightMapper::GenerateLightMap( int mapWidth, int mapHeight, RenderObject* pMeshToGen, const RenderList& sceneMeshes )
	{
		STexture* pLightMap = new STexture;
		pLightMap->Create(mapWidth, mapHeight);

		const float fTexelSizeX = 1.0f / (mapWidth - 1);
		const float fTexelSizeY = 1.0f / (mapHeight - 1);
		float u,v;
		const FaceList& faces = pMeshToGen->m_faces;
		const VertexBuffer& verts = pMeshToGen->m_verts;
		DWORD* pDest = (DWORD*)pLightMap->texData[0]->GetDataPointer();

		for (int y=0; y<mapHeight; ++y)
		{
			v = y * fTexelSizeY;

			for (int x=0; x<mapWidth; ++x, ++pDest)
			{
				u = x * fTexelSizeX;
				
				// Step 1 : Find the face which contains current uv
				for (size_t iFace=0; iFace<faces.size(); ++iFace)
				{
					const SVertex& v1 = verts[faces[iFace].index1];
					const SVertex& v2 = verts[faces[iFace].index2];
					const SVertex& v3 = verts[faces[iFace].index3];
					const VEC3& normal = faces[iFace].faceNormal;

					if (IsPointInTriangle(VEC2(u,v), v1.uv, v2.uv, v3.uv))
					{
						// Step 2 : Find the world position corresponding to current uv
						const VEC3 pos = _GetPosFromFace(v1, v2, v3, normal, VEC2(u,v));

						// Step 3 : Calc light map for this texel
						*pDest = _CalcLumel(sceneMeshes, pos, normal).GetAsInt();

						break;
					}
				}
			}
		}

		return pLightMap;
	}

	VEC3 LightMapper::_GetPosFromFace( const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const VEC3& normal, const VEC2& uv )
	{
		// TODO: Haven't understand this algorithm yet...
		const float abs_x = fabs(normal.x);
		const float abs_y = fabs(normal.y);
		const float abs_z = fabs(normal.z);
		const float x0 = vert0.pos.x, x1 = vert1.pos.x, x2 = vert2.pos.x;
		const float y0 = vert0.pos.y, y1 = vert1.pos.y, y2 = vert2.pos.y;
		const float z0 = vert0.pos.z, z1 = vert1.pos.z, z2 = vert2.pos.z;
		const float u0 = vert0.uv.x, u1 = vert1.uv.x, u2 = vert2.uv.x;
		const float v0 = vert0.uv.y, v1 = vert1.uv.y, v2 = vert2.uv.y;
		const float du = uv.x - u0;
		const float dv = uv.y - v0;
		const float denominator = (v0 - v2)*(u1 - u2) - (v1 - v2)*(u0 - u2);
		const float inv_denom = 1.0f / denominator;
		const float plane_d = Common::DotProduct_Vec3_By_Vec3(normal, vert0.pos.GetVec3());
		VEC3 oPos;

		if (abs_x >= abs_y && abs_x >= abs_z)
		{
			// Ignore x axis
			float dpdu = ((y1 - y2)*(v0 - v2) - (y0 - y2)*(v1 - v2)) * inv_denom;
			float dpdv = ((y1 - y2)*(u0 - u2) - (y0 - y2)*(u1 - u2)) * -inv_denom;
			float dqdu = ((z1 - z2)*(v0 - v2) - (z0 - z2)*(v1 - v2)) * inv_denom;
			float dqdv = ((z1 - z2)*(u0 - u2) - (z0 - z2)*(u1 - u2)) * -inv_denom;
		
			oPos.y = y0 + dpdu * du + dpdv * dv;
			oPos.z = z0 + dqdu * du + dqdv * dv;
			oPos.x = (plane_d - oPos.y * normal.y - oPos.z * normal.z) / normal.x;
		}
		else if (abs_y >= abs_x && abs_y >= abs_z)
		{
			// Ignore y axis
			float dpdu = ((x1 - x2)*(v0 - v2) - (x0 - x2)*(v1 - v2)) * inv_denom;
			float dpdv = ((x1 - x2)*(u0 - u2) - (x0 - x2)*(u1 - u2)) * -inv_denom;
			float dqdu = ((z1 - z2)*(v0 - v2) - (z0 - z2)*(v1 - v2)) * inv_denom;
			float dqdv = ((z1 - z2)*(u0 - u2) - (z0 - z2)*(u1 - u2)) * -inv_denom;

			oPos.x = x0 + dpdu * du + dpdv * dv;
			oPos.z = z0 + dqdu * du + dqdv * dv;
			oPos.y = (plane_d - oPos.x * normal.x - oPos.z * normal.z) / normal.y;
		}
		else if (abs_z >= abs_x && abs_z >= abs_y)
		{
			// Ignore z axis
			float dpdu = ((x1 - x2)*(v0 - v2) - (x0 - x2)*(v1 - v2)) * inv_denom;
			float dpdv = ((x1 - x2)*(u0 - u2) - (x0 - x2)*(u1 - u2)) * -inv_denom;
			float dqdu = ((y1 - y2)*(v0 - v2) - (y0 - y2)*(v1 - v2)) * inv_denom;
			float dqdv = ((y1 - y2)*(u0 - u2) - (y0 - y2)*(u1 - u2)) * -inv_denom;

			oPos.x = x0 + dpdu * du + dpdv * dv;
			oPos.y = y0 + dqdu * du + dqdv * dv;
			oPos.z = (plane_d - oPos.x * normal.x - oPos.y * normal.y) / normal.z;			
		}
		else
		{
			assert(0);
		}

		return oPos;
	}

	SColor LightMapper::_CalcLumel( const RenderList& sceneMeshes, const VEC3& pos, const VEC3& normal)
	{
		RAY shadowRay;
		shadowRay.m_origin = pos;
		shadowRay.m_dir = Common::Sub_Vec3_By_Vec3(m_pLight->pos, pos);
		shadowRay.m_dir.Normalize();

		// Ray-triangle intersection test for each mesh and each face...
		for (size_t iMesh=0; iMesh<sceneMeshes.size(); ++iMesh)
		{
			const FaceList& faces = sceneMeshes[iMesh]->m_faces;
			const VertexBuffer& verts = sceneMeshes[iMesh]->m_verts;

			for (size_t i=0; i<faces.size(); ++i)
			{
				const VEC3& p1 = verts[faces[i].index1].pos.GetVec3();
				const VEC3& p2 = verts[faces[i].index2].pos.GetVec3();
				const VEC3& p3 = verts[faces[i].index3].pos.GetVec3();

				auto result = shadowRay.Intersect_Triangle(p1, p2, p3);

				// Hit it!
				if (result.first)
				{
					// Is the intersection point near than the light?
					const VEC3 intersecPos = shadowRay.GetPoint(result.second);
					VEC3 testDir = Common::Sub_Vec3_By_Vec3(intersecPos, m_pLight->pos);

					// Avoid wrong self-shadow
					float fDelta = Common::Vec3_DistanceSq(pos, intersecPos);

					if(fDelta > 0.01f && Common::DotProduct_Vec3_By_Vec3(testDir, shadowRay.m_dir) < 0)
					{
						return SColor::BLACK;	// Yes in shadow
					}
				}
			}
		}

		// Not in shadow
		float nl = Common::DotProduct_Vec3_By_Vec3(normal, shadowRay.m_dir);
		SColor result;
		if(nl > 0) 
			result = SColor::WHITE * nl;
		else 
			result = SColor::BLACK;

		return result;
	}
}