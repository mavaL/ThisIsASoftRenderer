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
	struct SVertex 
	{
		Common::SVector3	pos;
	};
	typedef std::vector<SVertex>	VertexList;

	struct STriangle
	{
		SVertex	vert[3];
	};
	typedef std::vector<STriangle>	TriangleList;
}


#endif // GeometryDef_h__