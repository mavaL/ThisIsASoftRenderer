#include "stdafx.h"
#include "MathDef.h"

namespace Common
{
	Common::SVector3 SVector3::ZERO			=	SVector3(0, 0, 0);
	Common::SVector3 SVector3::UNIT_X		=	SVector3(1, 0, 0);
	Common::SVector3 SVector3::UNIT_Y		=	SVector3(0, 1, 0);
	Common::SVector3 SVector3::UNIT_Z		=	SVector3(0, 0, 1);
	Common::SVector3 SVector3::NEG_UNIT_X	=	SVector3(-1, 0, 0);
	Common::SVector3 SVector3::NEG_UNIT_Y	=	SVector3(0, -1, 0);
	Common::SVector3 SVector3::NEG_UNIT_Z	=	SVector3(0, 0, -1);

	//////////////////////////////////////////////////////////////////////////////////////////////
	SVector4 Transform_Vec3_By_Mat44( const SVector3& pt, const SMatrix44& mat, bool bPosOrDir )
	{
		return Transform_Vec4_By_Mat44(SVector4(pt, bPosOrDir ? 1.0f : 0.0f), mat);
	}

	SVector4 Transform_Vec4_By_Mat44( const SVector4& pt, const SMatrix44& mat )
	{
		return SVector4(
			mat.m_arr[0][0] * pt.x + mat.m_arr[0][1] * pt.y + mat.m_arr[0][2] * pt.z + mat.m_arr[0][3] * pt.w, 
			mat.m_arr[1][0] * pt.x + mat.m_arr[1][1] * pt.y + mat.m_arr[1][2] * pt.z + mat.m_arr[1][3] * pt.w,
			mat.m_arr[2][0] * pt.x + mat.m_arr[2][1] * pt.y + mat.m_arr[2][2] * pt.z + mat.m_arr[2][3] * pt.w,
			mat.m_arr[3][0] * pt.x + mat.m_arr[3][1] * pt.y + mat.m_arr[3][2] * pt.z + mat.m_arr[3][3] * pt.w
			);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	void SMatrix44::MakeIdentity()
	{
		m00 = 1; m01 = 0; m02 = 0; m03 = 0;
		m10 = 0; m11 = 1; m12 = 0; m13 = 0;
		m20 = 0; m21 = 0; m22 = 1; m23 = 0;
		m30 = 0; m31 = 0; m32 = 0; m33 = 1;
	}

	void SMatrix44::SetTranslation( const SVector4& t )
	{
		m_arr[0][3] = t.x;
		m_arr[1][3] = t.y;
		m_arr[2][3] = t.z;
		m_arr[3][3] = t.w;
	}

	Common::SMatrix44 SMatrix44::Transpose()
	{
		return SMatrix44(	m00, m10, m20, m30,
							m01, m11, m21, m31,
							m02, m12, m22, m32,
							m03, m13, m23, m33);
	}

	void SMatrix44::FromAxisAngle( const SVector3& axis, float angle )
	{
		//from ogre
		float radian = Angle_To_Radian(angle);
		float fCos = std::cos(radian);
		float fSin = std::sin(radian);
		float fOneMinusCos = 1.0f-fCos;
		float fX2 = axis.x*axis.x;
		float fY2 = axis.y*axis.y;
		float fZ2 = axis.z*axis.z;
		float fXYM = axis.x*axis.y*fOneMinusCos;
		float fXZM = axis.x*axis.z*fOneMinusCos;
		float fYZM = axis.y*axis.z*fOneMinusCos;
		float fXSin = axis.x*fSin;
		float fYSin = axis.y*fSin;
		float fZSin = axis.z*fSin;

		m_arr[0][0] = fX2*fOneMinusCos+fCos;
		m_arr[0][1] = fXYM-fZSin;
		m_arr[0][2] = fXZM+fYSin;
		m_arr[1][0] = fXYM+fZSin;
		m_arr[1][1] = fY2*fOneMinusCos+fCos;
		m_arr[1][2] = fYZM-fXSin;
		m_arr[2][0] = fXZM-fYSin;
		m_arr[2][1] = fYZM+fXSin;
		m_arr[2][2] = fZ2*fOneMinusCos+fCos;
		m_arr[0][3] = 0;
		m_arr[1][3] = 0;
		m_arr[2][3] = 0;

		ClearTranslation();
	}

	void SMatrix44::ClearTranslation()
	{
		m_arr[0][3] = m_arr[1][3] = m_arr[2][3] = 0;
		m_arr[3][3] = 1;
	}

	void SMatrix44::FromAxises( const SVector3& v1, const SVector3& v2, const SVector3& v3 )
	{
		m00 = v1.x; m01 = v2.x; m02 = v3.x; m03 = 0;
		m10 = v1.y; m11 = v2.y; m12 = v3.y; m13 = 0;
		m20 = v1.z; m21 = v2.z; m22 = v3.z; m23 = 0;
		m30 = 0;	m31 = 0;	m32 = 0;	m33 = 1;
	}

	void SMatrix44::SetRow( int row, const SVector4 vec )
	{
		assert(row >= 0 && row < 4);
		m_arr[row][0] = vec.x;
		m_arr[row][1] = vec.y;
		m_arr[row][2] = vec.z;
		m_arr[row][3] = vec.w;		
	}

	Common::SMatrix44 Multiply_Mat44_By_Mat44( const SMatrix44& mat1, const SMatrix44& mat2 )
	{
		SMatrix44 ret;

		//TODO: SSE
		ret.m00 = mat1.m00 * mat2.m00 + mat1.m01 * mat2.m10 + mat1.m02 * mat2.m20 + mat1.m03 * mat2.m30;
		ret.m01 = mat1.m00 * mat2.m10 + mat1.m01 * mat2.m11 + mat1.m02 * mat2.m21 + mat1.m03 * mat2.m31;
		ret.m02 = mat1.m00 * mat2.m20 + mat1.m01 * mat2.m12 + mat1.m02 * mat2.m22 + mat1.m03 * mat2.m32;
		ret.m03 = mat1.m00 * mat2.m30 + mat1.m01 * mat2.m13 + mat1.m02 * mat2.m23 + mat1.m03 * mat2.m33;

		ret.m10 = mat1.m10 * mat2.m00 + mat1.m11 * mat2.m10 + mat1.m12 * mat2.m20 + mat1.m13 * mat2.m30;
		ret.m11 = mat1.m10 * mat2.m10 + mat1.m11 * mat2.m11 + mat1.m12 * mat2.m21 + mat1.m13 * mat2.m31;
		ret.m12 = mat1.m10 * mat2.m20 + mat1.m11 * mat2.m12 + mat1.m12 * mat2.m22 + mat1.m13 * mat2.m32;
		ret.m13 = mat1.m10 * mat2.m30 + mat1.m11 * mat2.m13 + mat1.m12 * mat2.m23 + mat1.m13 * mat2.m33;

		ret.m20 = mat1.m20 * mat2.m00 + mat1.m21 * mat2.m10 + mat1.m22 * mat2.m20 + mat1.m23 * mat2.m30;
		ret.m21 = mat1.m20 * mat2.m10 + mat1.m21 * mat2.m11 + mat1.m22 * mat2.m21 + mat1.m23 * mat2.m31;
		ret.m22 = mat1.m20 * mat2.m20 + mat1.m21 * mat2.m12 + mat1.m22 * mat2.m22 + mat1.m23 * mat2.m32;
		ret.m23 = mat1.m20 * mat2.m30 + mat1.m21 * mat2.m13 + mat1.m22 * mat2.m23 + mat1.m23 * mat2.m33;

		ret.m30 = mat1.m30 * mat2.m00 + mat1.m31 * mat2.m10 + mat1.m32 * mat2.m20 + mat1.m33 * mat2.m30;
		ret.m31 = mat1.m30 * mat2.m10 + mat1.m31 * mat2.m11 + mat1.m32 * mat2.m21 + mat1.m33 * mat2.m31;
		ret.m32 = mat1.m30 * mat2.m20 + mat1.m31 * mat2.m12 + mat1.m32 * mat2.m22 + mat1.m33 * mat2.m32;
		ret.m33 = mat1.m30 * mat2.m30 + mat1.m31 * mat2.m13 + mat1.m32 * mat2.m23 + mat1.m33 * mat2.m33;

		return std::move(ret);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	Common::SVector4 Add_Vec4_By_Vec4( const SVector4& v1, const SVector4& v2 )
	{
		return std::move(SVector4(v1.x+v2.x, v1.y+v2.y, v1.z+v2.z, v1.w+v2.w));
	}

	Common::SVector4 Sub_Vec4_By_Vec4( const SVector4& v1, const SVector4& v2 )
	{
		return std::move(SVector4(v1.x-v2.x, v1.y-v2.y, v1.z-v2.z, v1.w-v2.w));
	}

	Common::SVector3 CrossProduct_Vec3_By_Vec3( const SVector3& v1, const SVector3& v2 )
	{
		SVector3 ret;

		ret.x = v1.y * v2.z - v1.z * v2.y;
		ret.y = v1.z * v2.x - v1.x * v2.z;
		ret.z = v1.x * v2.y - v1.y * v2.x;

		return std::move(ret);
	}

}


