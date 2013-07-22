#include "stdafx.h"
#include "MeshLoaderBase.h"
#include "Renderer.h"

namespace Ext
{
	bool MeshLoader::LoadMeshFile( const STRING& filename )
	{
		if(!LoadImpl(filename))
			return false;

		//each object
		for (size_t iObj=0; iObj<m_objs.size(); ++iObj)
		{
			SR::SRenderObj& obj = m_objs[iObj];

			//计算包围球
			obj.boundingRadius = SR::RenderUtil::ComputeBoundingRadius(obj.VB);

			//计算面法线
			for (size_t i=0; i<obj.faces.size(); ++i)
			{
				//fetch vertexs
				const SR::Index idx1 = obj.faces[i].index1;
				const SR::Index idx2 = obj.faces[i].index2;
				const SR::Index idx3 = obj.faces[i].index3;

				const VEC4& p0 = obj.VB[idx1].pos;
				const VEC4& p1 = obj.VB[idx2].pos;
				const VEC4& p2 = obj.VB[idx3].pos;

				//NB: 顶点必须是逆时针顺序
				const VEC3 u = Common::Sub_Vec4_By_Vec4(p1, p0).GetVec3();
				const VEC3 v = Common::Sub_Vec4_By_Vec4(p2, p0).GetVec3();
				obj.faces[i].faceNormal = Common::CrossProduct_Vec3_By_Vec3(u, v);
				obj.faces[i].faceNormal.Normalize();
			}
		}

		return true;
	}

}