#include "stdafx.h"
#include "RenderObject.h"
#include "Renderer.h"

namespace SR
{
	RenderObject::RenderObject()
	:m_bStatic(false)
	,m_pMaterial(nullptr)
	{

	}

	void RenderObject::OnFrameMove()
	{
		if (!m_bStatic)
		{
			//更新变换法线的逆转置矩阵
			m_matWorldIT = m_matWorld.Inverse();
			m_matWorldIT = m_matWorldIT.Transpose();

			//更新世界包围盒
			m_worldAABB = m_localAABB;
			m_worldAABB.Transform(m_matWorld);
		}
	}

	void RenderObject::CalcAllFaceTexArea()
	{
		if(!m_pMaterial->pDiffuseMap)
			return;

		const PixelBox* pSurface = m_pMaterial->pDiffuseMap->GetSurface(0);
		const VEC2 texDim(pSurface->GetWidth(), pSurface->GetHeight());

		for (size_t i=0; i<m_faces.size(); ++i)
		{
			SFace& face = m_faces[i];

			//fetch vertexs
			const SR::Index idx1 = face.index1;
			const SR::Index idx2 = face.index2;
			const SR::Index idx3 = face.index3;

			const VEC2 uv1 = Common::Multiply_Vec2_By_Vec2(m_verts[idx1].uv, texDim);
			const VEC2 uv2 = Common::Multiply_Vec2_By_Vec2(m_verts[idx2].uv, texDim);
			const VEC2 uv3 = Common::Multiply_Vec2_By_Vec2(m_verts[idx3].uv, texDim);

			face.texArea = Ext::CalcAreaOfTriangle(uv1, uv2, uv3);
		}
	}

	void RenderObject::BuildTangentVectors()
	{

	}
}