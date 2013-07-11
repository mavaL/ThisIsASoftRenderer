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

namespace SR
{
#ifdef USE_32BIT_INDEX
	typedef DWORD	Index;
#else
	typedef WORD	Index;
#endif

	///////////////////////////////////////////////////
	struct SVertex 
	{
		SVertex():normal(VEC3::ZERO),bActive(false),color(0) {}

		VEC4	pos;
		VEC3	normal;
		bool	bActive;
		int		color;
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
		int		color;					//用于Flat着色
		bool	IsBackface;
	};
	typedef std::vector<SFace>		FaceList;

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
		MAT44			matWorld;
		float			boundingRadius;
		bool			m_bCull;
	};
	typedef std::vector<SRenderObj>		RenderList;

	///////////////////////////////////////////////////
	struct SDirectionLight
	{
		SDirectionLight():dir(VEC3::ZERO),color(0) {}

		VEC3	dir;
		int		color;
	};
}


#endif // GeometryDef_h__