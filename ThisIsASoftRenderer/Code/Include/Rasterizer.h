/********************************************************************
	created:	3:7:2013   0:00
	filename	Rasterizer.h
	author:		maval

	purpose:	一系列光栅化器
*********************************************************************/
#ifndef Rasterizer_h__
#define Rasterizer_h__

#include "GeometryDef.h"

namespace SR
{
	enum eRasterizeType
	{
		eRasterizeType_Wireframe,
		eRasterizeType_Flat,
		eRasterizeType_Gouraud
	};

	/////////////////////////////////////////////////////////////
	//////// Rasterizer基类
	class Rasterizer
	{
	public:
		virtual ~Rasterizer() {}

	public:
		virtual void	RasterizeTriangleList(const VertexBuffer& vb, FaceList& faces) = 0;
		virtual eRasterizeType	GetType() = 0;
		virtual void	DoLighting(VertexBuffer& workingVB, SRenderObj& obj, const SDirectionLight& light) = 0;
	};

	/////////////////////////////////////////////////////////////
	//////// 线框
	class RasWireFrame : public Rasterizer
	{
	public:
		virtual void	RasterizeTriangleList(const VertexBuffer& vb, FaceList& faces);
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Wireframe; }
		virtual void	DoLighting(VertexBuffer&, SRenderObj&, const SDirectionLight&) {}
	};

	/////////////////////////////////////////////////////////////
	//////// Flat 
	class RasFlat : public Rasterizer
	{
	public:
		virtual void	RasterizeTriangleList(const VertexBuffer& vb, FaceList& faces);
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Flat; }
		virtual void	DoLighting(VertexBuffer& workingVB, SRenderObj& obj, const SDirectionLight& light);
	};

	/////////////////////////////////////////////////////////////
	//////// Gouraud
	class RasGouraud : public Rasterizer
	{
	public:
		virtual void	RasterizeTriangleList(const VertexBuffer& vb, FaceList& faces);
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Gouraud; }
		virtual void	DoLighting(VertexBuffer& workingVB, SRenderObj& obj, const SDirectionLight& light);
	};
}


#endif // Rasterizer_h__