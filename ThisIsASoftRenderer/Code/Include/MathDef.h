/********************************************************************
	created:	2:7:2013   22:38
	filename	MathDef.h
	author:		maval

	purpose:	简易数学定义.
				本项目采用右手坐标系与列主序矩阵.
				数学运算一律采用C函数,避免C++类运算符重载的隐晦
*********************************************************************/
#ifndef MathDef_h__
#define MathDef_h__

const float		PI	=	3.14159f;

namespace Common
{
	/////////////////////////////////////////////////////////////
	//////// 2D Vector
	struct SVector2
	{
		SVector2() {}
		SVector2(float _x, float _y):x(_x),y(_y) {}

		float x, y;
	};

	/////////////////////////////////////////////////////////////
	//////// 3D Vector
	struct SVector3
	{
		SVector3() {}
		SVector3(float _x, float _y, float _z):x(_x),y(_y),z(_z) {}
		SVector3(const SVector3& rhs):x(rhs.x),y(rhs.y),z(rhs.z) {}

		float x, y, z;
	};

	/////////////////////////////////////////////////////////////
	//////// 4D Vector
	struct SVector4
	{
		SVector4() {}
		SVector4(SVector3 pt, float _w):x(pt.x),y(pt.y),z(pt.z),w(_w) {}
		SVector4(float _x, float _y, float _z, float _w):x(_x),y(_y),z(_z),w(_w) {}

		SVector3	GetVec3()	{ return std::move(SVector3(x,y,z)); }
		//求负
		void		Neg();

		float x, y, z, w;
	};

	/////////////////////////////////////////////////////////////
	//////// 4x4 Matrix
	struct SMatrix44
	{
		SMatrix44() { MakeIdentity(); }
		SMatrix44(	float _m00, float _m01, float _m02, float _m03,
					float _m10, float _m11, float _m12, float _m13,
					float _m20, float _m21, float _m22, float _m23,
					float _m30, float _m31, float _m32, float _m33)
					:m00(_m00),m01(_m01),m02(_m02),m03(_m03)
					,m10(_m10),m11(_m11),m12(_m12),m13(_m13)
					,m20(_m20),m21(_m21),m22(_m22),m23(_m23)
					,m30(_m30),m31(_m31),m32(_m32),m33(_m33) {}

		void		MakeIdentity();
		void		Inverse()	{ /*TODO..*/ assert(0); }
		SMatrix44	Transpose();
		//清除平移部分
		void		ClearTranslation();
		//设置平移部分
		void		SetTranslation(const SVector4& t);
		//通过轴角对构建旋转矩阵,平移部分设为0
		void		FromAxisAngle(const SVector3& axis, float angle);

		union
		{
			struct  
			{
				float	m00, m01, m02, m03,
						m10, m11, m12, m13,
						m20, m21, m22, m23,
						m30, m31, m32, m33;
			};

			float m_arr[4][4];
		};
	};

	/////////////////////////////////////////////////////////////
	//////// 以4x4矩阵变换3d坐标,bPosOrDir为true表示变换的是点,否则是方向
	SVector4	Transform_Vec3_By_Mat44(const SVector3& pt, const SMatrix44& mat, bool bPosOrDir);

	/////////////////////////////////////////////////////////////
	//////// 以4x4矩阵变换4d坐标
	SVector4	Transform_Vec4_By_Mat44(const SVector4& pt, const SMatrix44& mat);

	/////////////////////////////////////////////////////////////
	//////// 4x4矩阵相乘
	SMatrix44	Multiply_Mat44_By_Mat44(const SMatrix44& mat1, const SMatrix44& mat2);

	/////////////////////////////////////////////////////////////
	//////// 4d向量相加
	SVector4	Add_Vec4_By_Vec4(const SVector4& v1, const SVector4& v2);
}

#endif // MathDef_h__