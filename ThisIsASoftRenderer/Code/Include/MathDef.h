/********************************************************************
	created:	2:7:2013   22:38
	filename	MathDef.h
	author:		maval

	purpose:	手写数学库.
				本项目采用右手坐标系与列主序矩阵.
				数学运算一律采用C函数,避免C++中运算符重载的隐晦
*********************************************************************/
#ifndef MathDef_h__
#define MathDef_h__

const float		PI			=	3.14159f;
const float		HALF_PI		=	1.57079f;
const float		TWO_PI		=	6.28318f;

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

		inline void	Normalize()
		{
			float mod = std::sqrt(x * x + y * y + z * z);
			float invMode = 1 / mod;
			x *= invMode;
			y *= invMode;
			z *= invMode;
		}
		//求负
		inline void	Neg() { x = -x; y = -y; z = -z; }

		float x, y, z;

		static SVector3		ZERO;
		static SVector3		UNIT_X;
		static SVector3		UNIT_Y;
		static SVector3		UNIT_Z;
		static SVector3		NEG_UNIT_X;
		static SVector3		NEG_UNIT_Y;
		static SVector3		NEG_UNIT_Z;
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
		inline void	Neg() { x = -x; y = -y; z = -z; w = -w; }

		float x, y, z, w;

		static SVector4		ZERO;
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

		void		SetRow(int row, const SVector4 vec);
		//单位矩阵化
		void		MakeIdentity();
		//零矩阵化
		void		MakeZero();
		//求逆
		void		Inverse()	{ /*TODO..*/ assert(0); }
		//转置
		SMatrix44	Transpose();
		//清除平移部分
		void		ClearTranslation();
		//设置平移部分
		void		SetTranslation(const SVector4& t);
		//获取平移部分
		SVector4	GetTranslation() const;
		//通过轴角对构建旋转矩阵,平移部分设为0
		void		FromAxisAngle(const SVector3& axis, float angle);
		//通过轴构建矩阵
		void		FromAxises(const SVector3& v1, const SVector3& v2, const SVector3& v3);

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

	/////////////////////////////////////////////////////////////
	//////// 4d向量乘以常数
	SVector4	Multiply_Vec4_By_K(const SVector4& v, float k);

	/////////////////////////////////////////////////////////////
	//////// 4d向量相减
	SVector4	Sub_Vec4_By_Vec4(const SVector4& v1, const SVector4& v2);

	/////////////////////////////////////////////////////////////
	//////// 3d向量叉乘
	SVector3	CrossProduct_Vec3_By_Vec3(const SVector3& v1, const SVector3& v2);

	/////////////////////////////////////////////////////////////
	//////// 角度转弧度
	inline float Angle_To_Radian(float angle)
	{
		return angle * PI / 180;
	}
}

typedef Common::SVector2	VEC2;
typedef Common::SVector3	VEC3;
typedef Common::SVector4	VEC4;
typedef Common::SMatrix44	MAT44;

#endif // MathDef_h__