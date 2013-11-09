#include "stdafx.h"
#include "RenderObject.h"
#include "Renderer.h"

namespace SR
{
	RenderObject::RenderObject()
	:m_bStatic(false)
	,m_pMaterial(nullptr)
	,m_pShader(nullptr)
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

	static inline VEC3 GetAngleWeight( const VEC3& v1, const VEC3& v2, const VEC3& v3 )
	{
		// Calculate this triangle's weight for each of its three vertices
		// start by calculating the lengths of its sides
		const float a = Common::Vec3_Distance(v2, v3);
		const float asqrt = sqrtf(a);
		const float b = Common::Vec3_Distance(v1, v3);
		const float bsqrt = sqrtf(b);
		const float c = Common::Vec3_Distance(v1, v2);
		const float csqrt = sqrtf(c);

		// use them to find the angle at each vertex
		return VEC3(
			acosf((b + c - a) / (2.f * bsqrt * csqrt)),
			acosf((-b + c + a) / (2.f * asqrt * csqrt)),
			acosf((b - c + a) / (2.f * bsqrt * asqrt)));
	}

	void RenderObject::BuildTangentVectors()
	{
		/////Ogre,鬼火都是这个算法:根据顶点所在的面来计算切空间,然后加权起来

		//Each vertex gets the sum of the tangents and binormals from the faces around it
		for ( size_t i=0; i<m_verts.size(); ++i )
		{
			m_verts[i].normal = VEC3::ZERO;
			m_verts[i].tangent = VEC3::ZERO;
			m_verts[i].binormal = VEC3::ZERO;
		}

		for (size_t i=0; i<m_faces.size(); ++i)
		{
			SFace& face = m_faces[i];

			//fetch vertexs
			const SR::Index idx1 = face.index1;
			const SR::Index idx2 = face.index2;
			const SR::Index idx3 = face.index3;

			const VEC2& tc1 = m_verts[idx1].uv;
			const VEC2& tc2 = m_verts[idx2].uv;
			const VEC2& tc3 = m_verts[idx3].uv;

			const VEC3& vt1 = m_verts[idx1].pos.GetVec3();
			const VEC3& vt2 = m_verts[idx2].pos.GetVec3();
			const VEC3& vt3 = m_verts[idx3].pos.GetVec3();

			// if this triangle is degenerate, skip it!
			if (vt1 == vt2 ||
				vt2 == vt3 ||
				vt3 == vt1	)
				continue;

			//Angle-weighted normals look better, but are slightly more CPU intensive to calculate
			VEC3 weight = GetAngleWeight(vt1, vt2, vt3);	// writing irr::scene:: necessary for borland

			VEC3 localNormal;
			VEC3 localTangent;
			VEC3 localBinormal;

			_CalcTangentSpace(localNormal,localTangent,localBinormal,
				vt1,vt2,vt3,
				tc1,tc2,tc3);

			Common::Multiply_Vec3_By_K(localNormal, localNormal, weight.x);
			Common::Multiply_Vec3_By_K(localTangent, localTangent, weight.x);
			Common::Multiply_Vec3_By_K(localBinormal, localBinormal, weight.x);
			Common::Add_Vec3_By_Vec3(m_verts[idx1].normal, m_verts[idx1].normal, localNormal);
			Common::Add_Vec3_By_Vec3(m_verts[idx1].tangent, m_verts[idx1].tangent, localTangent);
			Common::Add_Vec3_By_Vec3(m_verts[idx1].binormal, m_verts[idx1].binormal, localBinormal);

			_CalcTangentSpace(localNormal,localTangent,localBinormal,
				vt2,vt3,vt1,
				tc2,tc3,tc1);

			Common::Multiply_Vec3_By_K(localNormal, localNormal, weight.y);
			Common::Multiply_Vec3_By_K(localTangent, localTangent, weight.y);
			Common::Multiply_Vec3_By_K(localBinormal, localBinormal, weight.y);
			Common::Add_Vec3_By_Vec3(m_verts[idx2].normal, m_verts[idx2].normal, localNormal);
			Common::Add_Vec3_By_Vec3(m_verts[idx2].tangent, m_verts[idx2].tangent, localTangent);
			Common::Add_Vec3_By_Vec3(m_verts[idx2].binormal, m_verts[idx2].binormal, localBinormal);

			_CalcTangentSpace(localNormal,localTangent,localBinormal,
				vt3,vt1,vt2,
				tc3,tc1,tc2);

			Common::Multiply_Vec3_By_K(localNormal, localNormal, weight.z);
			Common::Multiply_Vec3_By_K(localTangent, localTangent, weight.z);
			Common::Multiply_Vec3_By_K(localBinormal, localBinormal, weight.z);
			Common::Add_Vec3_By_Vec3(m_verts[idx3].normal, m_verts[idx3].normal, localNormal);
			Common::Add_Vec3_By_Vec3(m_verts[idx3].tangent, m_verts[idx3].tangent, localTangent);
			Common::Add_Vec3_By_Vec3(m_verts[idx3].binormal, m_verts[idx3].binormal, localBinormal);
		}

		for ( size_t i=0; i<m_verts.size(); ++i )
		{
			m_verts[i].tangent.Normalize();
			m_verts[i].binormal.Normalize();
			m_verts[i].normal.Normalize();
		}
	}

	void RenderObject::_CalcTangentSpace( VEC3& oNormal, VEC3& oTangent, VEC3& oBinormal, const VEC3& vt1, const VEC3& vt2, const VEC3& vt3, const VEC2& tc1, const VEC2& tc2, const VEC2& tc3 )
	{
		VEC3 v1 = Common::Sub_Vec3_By_Vec3(vt1, vt2);
		VEC3 v2 = Common::Sub_Vec3_By_Vec3(vt3, vt1);

		oNormal = Common::CrossProduct_Vec3_By_Vec3(v2, v1);
		oNormal.Normalize();

		VEC3 tmp1, tmp2;
		// binormal
		float deltaX1 = tc1.x - tc2.x;
		float deltaX2 = tc3.x - tc1.x;
		Common::Multiply_Vec3_By_K(tmp1, v1, deltaX2);
		Common::Multiply_Vec3_By_K(tmp2, v2, deltaX1);
		oBinormal = Common::Sub_Vec3_By_Vec3(tmp1, tmp2);
		oBinormal.Normalize();

		// tangent
		float deltaY1 = tc1.y - tc2.y;
		float deltaY2 = tc3.y - tc1.y;
		Common::Multiply_Vec3_By_K(tmp1, v1, deltaY2);
		Common::Multiply_Vec3_By_K(tmp2, v2, deltaY1);
		oTangent = Common::Sub_Vec3_By_Vec3(tmp1, tmp2);
		oTangent.Normalize();

		// adjust
		VEC3 txb = Common::CrossProduct_Vec3_By_Vec3(oTangent, oBinormal);
		if (Common::DotProduct_Vec3_By_Vec3(txb, oNormal) < 0.0f)
		{
			Common::Multiply_Vec3_By_K(oTangent, oTangent, -1.0f);
			Common::Multiply_Vec3_By_K(oBinormal, oBinormal, -1.0f);
		}
	}

	void RenderObject::SetShader(eRasterizeType shader)
	{
		m_pShader = g_env.renderer->GetRasterizer(shader);
		assert(m_pShader);
	}

}