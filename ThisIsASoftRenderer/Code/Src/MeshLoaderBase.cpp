#include "stdafx.h"
#include "MeshLoaderBase.h"
#include "Renderer.h"

namespace Ext
{
	bool MeshLoader::LoadMeshFile( const STRING& filename, bool bStatic, bool bFlipUV )
	{
		if(!LoadImpl(filename, bFlipUV))
			return false;

		//each object
		for (size_t iObj=0; iObj<m_objs.size(); ++iObj)
		{
			SR::RenderObject& obj = *m_objs[iObj];

			obj.m_bStatic = bStatic;

			//计算面法线
			for (size_t i=0; i<obj.m_faces.size(); ++i)
			{
				//fetch vertexs
				const SR::Index idx1 = obj.m_faces[i].index1;
				const SR::Index idx2 = obj.m_faces[i].index2;
				const SR::Index idx3 = obj.m_faces[i].index3;

				const VEC4& p0 = obj.m_verts[idx1].pos;
				const VEC4& p1 = obj.m_verts[idx2].pos;
				const VEC4& p2 = obj.m_verts[idx3].pos;

				//NB: 顶点必须是逆时针顺序
				const VEC3 u = Common::Sub_Vec4_By_Vec4(p1, p0).GetVec3();
				const VEC3 v = Common::Sub_Vec4_By_Vec4(p2, p0).GetVec3();
				obj.m_faces[i].faceNormal = Common::CrossProduct_Vec3_By_Vec3(u, v);
				obj.m_faces[i].faceNormal.Normalize();
			}
		}

		return true;
	}

}