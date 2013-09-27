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
#include "Color.h"

namespace SR
{
#if USE_32BIT_INDEX == 1
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
	struct SVertex 
	{
		SVertex()
		:normal(VEC3::ZERO),worldNormal(VEC3::ZERO),bActive(false),
		color(SColor::BLACK),uv(-1,-1),viewSpaceZ(0) {}

		VEC4	worldPos;
		VEC4	pos;
		VEC3	worldNormal;
		VEC3	normal;
		VEC2	uv;
		bool	bActive;
		SColor	color;
		float	viewSpaceZ;	// Use for mip determine
	};
	typedef std::vector<SVertex>	VertexBuffer;
	typedef std::vector<Index>		IndexBuffer;

	///////////////////////////////////////////////////
	struct SFace
	{
		SFace()
		:index1(-1),index2(-1),index3(-1),color(SColor::BLACK),
		IsBackface(false),bCulled(false),texArea(0) {}

		SFace(Index idx1, Index idx2, Index idx3)
		:index1(idx1),index2(idx2),index3(idx3),faceNormal(VEC3::ZERO),
		color(SColor::BLACK),IsBackface(false),bCulled(false),texArea(0) {}

		Index	index1, index2, index3;
		VEC3	faceNormal;				//面法线,用于背面拣选和Flat着色
		SColor	color;					//用于Flat着色
		bool	IsBackface;
		bool	bCulled;
		float	texArea;				//该三角面在纹理中所占面积
	};
	typedef std::vector<SFace>		FaceList;

	///////////////////////////////////////////////////
	struct STexture
	{
		STexture():texName(""),bMipMap(false),lodBias(0) {}
		~STexture();

		STexture(const STexture& rhs);
		STexture& operator= (const STexture& rhs);

		void		LoadTexture(const STRING& filename, bool bmipmap);
		PixelBox*	GetSurface(int i);
		//临近点采样
		void		Tex2D_Point(VEC2& uv, SColor& ret, int mip = 0) const;
		//双线性插值采样
		void		Tex2D_Bilinear(VEC2& uv, SColor& ret, int mip = 0) const;
		//生成mipmap层
		void		GenMipMaps();
		int			GetMipCount() const { return texData.size(); }

		STRING			texName;
		typedef std::vector<PixelBox*>	MipmapChain;
		MipmapChain		texData;
		bool			bMipMap;
		int				lodBias;
	};

	///////////////////////////////////////////////////
	struct SMaterial 
	{
		SMaterial()
			:ambient(SColor::WHITE),diffuse(SColor::WHITE),specular(SColor::WHITE)
			,pDiffuseMap(nullptr),shiness(20),bUseHalfLambert(false),bUseBilinearSampler(false) {}

		~SMaterial() { SAFE_DELETE(pDiffuseMap); }

		SColor		ambient, diffuse, specular;
		float		shiness;
		STexture*	pDiffuseMap;
		bool		bUseHalfLambert;		//See: https://developer.valvesoftware.com/wiki/Half_Lambert
		bool		bUseBilinearSampler;	//使用纹理双线性插值
	};

	///////////////////////////////////////////////////
	struct SDirectionLight
	{
		SDirectionLight():dir(VEC3::ZERO),neg_dir(VEC3::ZERO),color(SColor::BLACK) {}

		VEC3	dir;
		VEC3	neg_dir;	//反方向
		SColor	color;
	};

	///////////////////////////////////////////////////
	struct SFragment 
	{
		bool		bActive;
		DWORD*		finalColor;	//最终颜色输出
		VEC3		worldPos;	//世界坐标
		VEC2		uv;			//纹理坐标
		VEC3		normal;		//世界法线
		SMaterial*	pMaterial;	//材质
		int			texLod;
	};
}

#endif // GeometryDef_h__