/********************************************************************
	created:	4:7:2013   20:14
	filename	GeometryDef.h
	author:		maval

	purpose:	渲染几何数据结构定义
*********************************************************************/
#ifndef GeometryDef_h__
#define GeometryDef_h__

#include "Prerequiestity.h"
#include "Utility.h"
#include "MathDef.h"
#include "PixelBox.h"

namespace SR
{
#ifdef USE_32BIT_INDEX
	typedef DWORD	Index;
#else
	typedef WORD	Index;
#endif

	enum eTriangleShape
	{
		eTriangleShape_Top,		//平顶
		eTriangleShape_Bottom,	//平底
		eTriangleShape_General	//普通
	};

	///////////////////////////////////////////////////
	struct SColor 
	{
		SColor():color(0) {}
		explicit SColor(int c):color(c) {}
		SColor(BYTE _a, BYTE _r, BYTE _g, BYTE _b):a(_a),r(_r),g(_g),b(_b) {}

		SColor operator* (float k) const
		{ 
			SColor ret;
			ret.a = Ext::Ftoi32_Fast(a * k); 
			ret.r = Ext::Ftoi32_Fast(r * k);
			ret.g = Ext::Ftoi32_Fast(g * k);
			ret.b = Ext::Ftoi32_Fast(b * k);

			return ret;
		}

		SColor operator*= (float k)
		{
			a = Ext::Ftoi32_Fast(a * k); 
			r = Ext::Ftoi32_Fast(r * k); 
			g = Ext::Ftoi32_Fast(g * k); 
			b = Ext::Ftoi32_Fast(b * k);
			return *this;
		}

		SColor operator* (const VEC3& v) const
		{
			SColor ret;

			ret.r = Ext::Ftoi32_Fast(r * v.x); 
			ret.g = Ext::Ftoi32_Fast(g * v.y); 
			ret.b = Ext::Ftoi32_Fast(b * v.z);

			return ret;
		}

		SColor operator+= (const SColor& rhs)
		{
			//NB: 小心溢出！
			int tmp_a = a + rhs.a;
			int tmp_r = r + rhs.r;
			int tmp_g = g + rhs.g;
			int tmp_b = b + rhs.b;

			a = tmp_a > 255 ? 255 : tmp_a; 
			r = tmp_r > 255 ? 255 : tmp_r;  
			g = tmp_g > 255 ? 255 : tmp_g;  
			b = tmp_b > 255 ? 255 : tmp_b;  

			return *this;
		}

		union
		{
			struct
			{
				BYTE b,g,r,a;
			};
			unsigned int color;
		};

		static SColor WHITE;
		static SColor BLACK;
		static SColor BLUE;
	};

	///////////////////////////////////////////////////
	struct SVertex 
	{
		SVertex():normal(VEC3::ZERO),worldNormal(VEC3::ZERO),bActive(false),color(0),uv(-1,-1) {}

		VEC4	worldPos;	
		VEC4	pos;
		VEC3	worldNormal;
		VEC3	normal;
		VEC2	uv;
		bool	bActive;
		SColor	color;
	};
	typedef std::vector<SVertex>	VertexBuffer;
	typedef std::vector<Index>		IndexBuffer;

	///////////////////////////////////////////////////
	struct SFace
	{
		SFace():index1(-1),index2(-1),index3(-1),color(0),IsBackface(false),bCulled(false) {}
		SFace(Index idx1, Index idx2, Index idx3):index1(idx1),index2(idx2),index3(idx3),
			faceNormal(VEC3::ZERO),color(0),IsBackface(false),bCulled(false) {}

		Index	index1, index2, index3;
		VEC3	faceNormal;				//面法线,用于背面拣选和Flat着色
		SColor	color;					//用于Flat着色
		bool	IsBackface;
		bool	bCulled;
	};
	typedef std::vector<SFace>		FaceList;

	///////////////////////////////////////////////////
	struct STexture
	{
		STexture():texName(""),pData(nullptr) {}
		~STexture() { SAFE_DELETE(pData); }

		STexture(const STexture& rhs);
		STexture& operator= (const STexture& rhs);

		void		LoadTexture(const STRING& filename);
		//临近点采样
		void		Tex2D_Point(VEC2& uv, SColor& ret) const;
		//双线性插值采样
		void		Tex2D_Bilinear(VEC2& uv, SColor& ret) const;

		STRING		texName;		
		Common::PixelBox*	pData;
	};

	///////////////////////////////////////////////////
	struct SMaterial 
	{
		SMaterial():ambient(1,1,1),diffuse(1,1,1),specular(1,1,1),pDiffuseMap(nullptr),pNormalMap(nullptr)
			,shiness(20),bUseHalfLambert(false),bUseBilinearSampler(false) {}
		~SMaterial() { SAFE_DELETE(pDiffuseMap); SAFE_DELETE(pNormalMap); }

		VEC3		ambient, diffuse, specular;
		float		shiness;
		STexture*	pDiffuseMap;
		STexture*	pNormalMap;
		bool		bUseHalfLambert;		//See: https://developer.valvesoftware.com/wiki/Half_Lambert
		bool		bUseBilinearSampler;	//使用纹理双线性插值
	};

	///////////////////////////////////////////////////
	struct SDirectionLight
	{
		SDirectionLight():dir(VEC3::ZERO),color(SColor::BLACK) {}

		VEC3	dir;
		SColor	color;
	};
}

namespace Ext
{
	//特化SColor
	template<> inline void LinearLerp(SR::SColor& result, const SR::SColor& s, const SR::SColor& e, float t)
	{
		float a,r,g,b;
		LinearLerp(a, (float)s.a, (float)e.a, t);
		LinearLerp(r, (float)s.r, (float)e.r, t);
		LinearLerp(g, (float)s.g, (float)e.g, t);
		LinearLerp(b, (float)s.b, (float)e.b, t);

		result.a = Ext::Ftoi32_Fast(a);
		result.r = Ext::Ftoi32_Fast(r);
		result.g = Ext::Ftoi32_Fast(g);
		result.b = Ext::Ftoi32_Fast(b);
	}
}

#endif // GeometryDef_h__