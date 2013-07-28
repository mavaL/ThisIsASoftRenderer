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

		inline SColor operator* (float k) const
		{ 
			SColor ret;
			ret.a = Ext::Ftoi32_Fast(a * k); 
			ret.r = Ext::Ftoi32_Fast(r * k);
			ret.g = Ext::Ftoi32_Fast(g * k);
			ret.b = Ext::Ftoi32_Fast(b * k);

			return ret;
		}

		inline SColor operator*= (float k)
		{
			a = Ext::Ftoi32_Fast(a * k); 
			r = Ext::Ftoi32_Fast(r * k); 
			g = Ext::Ftoi32_Fast(g * k); 
			b = Ext::Ftoi32_Fast(b * k);
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
		SVertex():normal(VEC3::ZERO),bActive(false),color(0),uv(-1,-1) {}

		VEC4	pos;
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
		//点采样
		SColor		Tex2D_Point(VEC2& uv) const;

		STRING		texName;		
		Common::PixelBox*	pData;
	};

	///////////////////////////////////////////////////
	struct SMaterial 
	{
		SMaterial():ambient(VEC3::ZERO),diffuse(VEC3::ZERO),specular(VEC3::ZERO),pTexture(nullptr) {}
		~SMaterial() { SAFE_DELETE(pTexture); }

		VEC3		ambient, diffuse, specular;
		STexture*	pTexture;
	};

	///////////////////////////////////////////////////
	struct SDirectionLight
	{
		SDirectionLight():dir(VEC3::ZERO),color(SColor::BLACK) {}

		VEC3	dir;
		SColor	color;
	};
}


#endif // GeometryDef_h__