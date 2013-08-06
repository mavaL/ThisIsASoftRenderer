/********************************************************************
	created:	3:7:2013   0:00
	filename	Rasterizer.h
	author:		maval

	purpose:	一系列光栅化器
*********************************************************************/
#ifndef Rasterizer_h__
#define Rasterizer_h__

#include "Prerequiestity.h"
#include "GeometryDef.h"

namespace SR
{
	enum eRasterizeType
	{
		eRasterizeType_Wireframe,
		eRasterizeType_Flat,
		eRasterizeType_Gouraud,
		eRasterizeType_TexturedGouraud,
		eRasterizeType_BlinnPhong,
		eRasterizeType_NormalMap
	};

	/////////////////////////////////////////////////////////////
	//////// Rasterizer基类
	class Rasterizer
	{
	public:
		virtual ~Rasterizer() {}

	public:
		virtual void	RasterizeTriangleList(SRenderContext& context);
		virtual eRasterizeType	GetType() = 0;
		//逐顶点光照
		virtual void	DoPerVertexLighting(VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj) {}
		//逐像素光照
		virtual void	DoPerPixelLighting(SColor& result, const VEC3& worldPos, const VEC3& worldNormal, const SMaterial* pMaterial) {}

	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context) = 0;
	};

	/////////////////////////////////////////////////////////////
	//////// 线框
	class RasWireFrame : public Rasterizer
	{
	public:
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Wireframe; }

	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context);
	};

	/////////////////////////////////////////////////////////////
	//////// Flat 
	class RasFlat : public Rasterizer
	{
	public:
		virtual void	RasterizeTriangleList(SRenderContext& context);
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Flat; }
		virtual void	DoPerVertexLighting(VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj);

	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context);
	};

	/////////////////////////////////////////////////////////////
	//////// Gouraud
	class RasGouraud : public Rasterizer
	{
	public:
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Gouraud; }
		virtual void	DoPerVertexLighting(VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj);

		struct SScanLineData 
		{
			VEC3	curP_L, curP_R;		//屏幕坐标的x,z,w分量
			VEC3	dp_L, dp_R;			//屏幕坐标的x,z,w增量
			VEC3	curPW_L, curPW_R;	//世界坐标
			VEC3	dpw_L, dpw_R;		//世界坐标增量
			VEC3	curN_L, curN_R;		//世界法线
			VEC3	dn_L, dn_R;			//世界法线增量
			int		curY, endY;
			VEC3	clr_L, clr_R;
			VEC3	dclr_L, dclr_R;
			VEC2	curUV_L, curUV_R;
			VEC2	duv_L, duv_R;
		};

	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context);
	};

	/////////////////////////////////////////////////////////////
	//////// 纹理+gouraud
	class RasTexturedGouraud : public RasGouraud
	{
	public:
		virtual eRasterizeType	GetType()	{ return eRasterizeType_TexturedGouraud; }

	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context);
	};

	/////////////////////////////////////////////////////////////
	//////// Blinn-Phong光照,Phong模型是逐像素光照
	class RasBlinnPhong : public Rasterizer
	{
	public:
		virtual eRasterizeType	GetType()	{ return eRasterizeType_BlinnPhong; }
		virtual void	DoPerPixelLighting(SColor& result, const VEC3& worldPos, const VEC3& worldNormal, const SMaterial* pMaterial);
	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context);
	};

	/////////////////////////////////////////////////////////////
	//////// Phong + 法线贴图
	class RasNormalMap : public RasBlinnPhong
	{
	public:
		virtual eRasterizeType	GetType()	{ return eRasterizeType_NormalMap; }
		virtual void	DoPerPixelLighting(SColor& result, const VEC3& worldPos, const VEC3& worldNormal, const SMaterial* pMaterial) {}
	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context) {}
	};
}


#endif // Rasterizer_h__