/********************************************************************
	created:	4:7:2013   20:14
	filename	GeometryDef.h
	author:		maval

	purpose:	渲染几何数据结构定义
*********************************************************************/
#ifndef GeometryDef_h__
#define GeometryDef_h__

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
			ret.a = (BYTE)(a * k); 
			ret.r = (BYTE)(r * k);
			ret.g = (BYTE)(g * k);
			ret.b = (BYTE)(b * k);

			return ret;
		}

		inline SColor operator*= (float k)
		{
			a = (BYTE)(a * k); 
			r = (BYTE)(r * k); 
			g = (BYTE)(g * k); 
			b = (BYTE)(b * k);
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
		SFace():index1(-1),index2(-1),index3(-1),IsBackface(false),color(0) {}
		SFace(Index idx1, Index idx2, Index idx3):index1(idx1),index2(idx2),index3(idx3),
			IsBackface(false),faceNormal(VEC3::ZERO),color(0) {}

		void	ResetState()	{ IsBackface = false; }

		Index	index1, index2, index3;
		VEC3	faceNormal;				//面法线,用于背面拣选和Flat着色
		SColor	color;					//用于Flat着色
		bool	IsBackface;
	};
	typedef std::vector<SFace>		FaceList;

	///////////////////////////////////////////////////
	struct STexture
	{
		STexture():texName(""),pData(nullptr) {}
		~STexture() { SAFE_DELETE(pData); }

		void LoadTexture(const STRING& filename);

		STRING		texName;
		Common::PixelBox*	pData;
	};

	///////////////////////////////////////////////////
	struct SRenderObj 
	{
		SRenderObj():boundingRadius(0),m_bCull(false) {}

		void ResetState()
		{ 
			m_bCull = false; 
			std::for_each(faces.begin(), faces.end(), [&](SFace& face){ face.ResetState(); });
		}

		VertexBuffer	VB;
		FaceList		faces;
		STexture		texture;
		MAT44			matWorld;
		MAT44			matWorldIT;		//世界矩阵的逆转置,用于法线变换
		float			boundingRadius;
		bool			m_bCull;
	};
	typedef std::vector<SRenderObj>		RenderList;

	///////////////////////////////////////////////////
	struct SDirectionLight
	{
		SDirectionLight():dir(VEC3::ZERO),color(SColor::BLACK) {}

		VEC3	dir;
		SColor	color;
	};
}


#endif // GeometryDef_h__