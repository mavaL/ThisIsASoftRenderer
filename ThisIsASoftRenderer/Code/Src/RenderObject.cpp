#include "stdafx.h"
#include "RenderObject.h"

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

}