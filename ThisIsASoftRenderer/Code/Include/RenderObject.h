/********************************************************************
	created:	2013/07/23
	created:	23:7:2013   9:33
	filename: 	RenderObject.h
	author:		maval
	
	purpose:	渲染对象高层封装
*********************************************************************/
#ifndef RenderObject_h__
#define RenderObject_h__

#include "Prerequiestity.h"
#include "MathDef.h"
#include "GeometryDef.h"
#include "AABB.h"

namespace SR
{
	class RenderObject
	{
	public:
		RenderObject();

		// Mip-map level determination
		void	CalcAllFaceTexArea();
		// Calc tangent space for mesh of this object
		void	BuildTangentVectors();

		void	OnFrameMove();

		VertexBuffer	m_verts;
		FaceList		m_faces;
		SMaterial*		m_pMaterial;
		MAT44			m_matWorld;
		MAT44			m_matWorldIT;		//世界矩阵的逆转置,用于法线变换
		AABB			m_localAABB;		//本地包围盒
		AABB			m_worldAABB;		//世界包围盒
		bool			m_bStatic;			//该物体在场景中完全固定
	};

	typedef std::vector<RenderObject*>		RenderList;
}

#endif // RenderObject_h__