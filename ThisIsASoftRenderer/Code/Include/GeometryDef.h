/********************************************************************
	created:	4:7:2013   20:14
	filename	GeometryDef.h
	author:		maval

	purpose:	渲染几何数据结构定义
*********************************************************************/
#ifndef GeometryDef_h__
#define GeometryDef_h__

#include "MathDef.h"

namespace SR
{
#ifdef USE_32BIT_INDEX
	typedef DWORD	Index;
#else
	typedef WORD	Index;
#endif

	struct SVertex 
	{
		Common::SVector4	pos;
	};
	typedef std::vector<SVertex>	VertexBuffer;
	typedef std::vector<Index>		IndexBuffer;

	struct SFace
	{
		SFace():index1(-1),index2(-1),index3(-1) {}
		SFace(Index idx1, Index idx2, Index idx3):index1(idx1),index2(idx2),index3(idx3) {}

		Index	index1, index2, index3;
	};

	struct SRenderList 
	{
		VertexBuffer	verts;
		IndexBuffer		indexes;
	};
}


#endif // GeometryDef_h__