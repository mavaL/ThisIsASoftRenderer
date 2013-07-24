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
		eRasterizeType_Gouraud,
		eRasterizeType_TexturedGouraud
	};

	/////////////////////////////////////////////////////////////
	//////// Rasterizer基类
	class Rasterizer
	{
	public:
		virtual ~Rasterizer() {}

		struct SRenderContext 
		{
			SRenderContext():verts(nullptr),faces(nullptr),texture(nullptr) {}

			VertexBuffer*	verts;
			FaceList*		faces;
			STexture*		texture;
		};

	public:
		virtual void	RasterizeTriangleList(SRenderContext& context) = 0;
		virtual eRasterizeType	GetType() = 0;
		virtual void	DoLighting(VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj, const SDirectionLight& light) = 0;
	};

	/////////////////////////////////////////////////////////////
	//////// 线框
	class RasWireFrame : public Rasterizer
	{
	public:
		virtual void	RasterizeTriangleList(SRenderContext& context);
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Wireframe; }
		virtual void	DoLighting(VertexBuffer&, FaceList&, RenderObject&, const SDirectionLight&) {}
	};

	/////////////////////////////////////////////////////////////
	//////// Flat 
	class RasFlat : public Rasterizer
	{
	public:
		virtual void	RasterizeTriangleList(SRenderContext& context);
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Flat; }
		virtual void	DoLighting(VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj, const SDirectionLight& light);
	};

	/////////////////////////////////////////////////////////////
	//////// Gouraud
	class RasGouraud : public Rasterizer
	{
	public:
		virtual void	RasterizeTriangleList(SRenderContext& context);
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Gouraud; }
		virtual void	DoLighting(VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj, const SDirectionLight& light);
	};

	/////////////////////////////////////////////////////////////
	//////// 纹理+gouraud
	class RasTexturedGouraud : public RasGouraud
	{
	public:
		virtual void	RasterizeTriangleList(SRenderContext& context);
		virtual eRasterizeType	GetType()	{ return eRasterizeType_TexturedGouraud; }
	};
}


#endif // Rasterizer_h__