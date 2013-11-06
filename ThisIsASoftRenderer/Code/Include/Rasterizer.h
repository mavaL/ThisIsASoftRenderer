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
		eRasterizeType_PhongWithNormalMap
	};

	enum eLerpType
	{
		eLerpType_Linear,		//线性插值
		eLerpType_Hyper			//双曲线插值
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
		virtual void	DoPerPixelLighting(SColor& result, const VEC3& worldPos, const VEC3& worldNormal, const VEC2& uv, const SMaterial* pMaterial) {}
		//以下两个光栅化虚函数,目的是不同shading model自行选择需要插值的顶点属性
		virtual void	LerpVertexAttributes(SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type);
		virtual void	RasTriangleSetup(SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type);
		//扫描线光栅化
		virtual void	RasLineClipX(SScanLine& scanLine, const SScanLinesData& rasData);
		virtual void	RasScanLine(SScanLine& scanLine, const SScanLinesData& rasData, const SRenderContext& context);
		virtual void	RasAdvanceToNextPixel();
		virtual void	RasAdvanceToNextScanLine();

	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context) = 0;
	};

	/////////////////////////////////////////////////////////////
	//////// 线框
	class RasWireFrame : public Rasterizer
	{
	public:
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Wireframe; }
		virtual void	LerpVertexAttributes(SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type) {}
		virtual void	RasTriangleSetup(SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type) {}

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
		virtual void	LerpVertexAttributes(SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type) {}
		virtual void	RasTriangleSetup(SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type) {}

	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context);
	};

	/////////////////////////////////////////////////////////////
	//////// Gouraud
	struct SScanLinesData 
	{
		VEC3	curP_L, curP_R;		//屏幕坐标的x,z,w分量
		VEC3	dp_L, dp_R;			//屏幕坐标的x,z,w增量
		float	inv_dy_L, inv_dy_R;
		VEC3	curPW_L, curPW_R;	//世界坐标
		VEC3	dpw_L, dpw_R;		//世界坐标增量
		VEC3	curN_L, curN_R;		//世界法线
		VEC3	dn_L, dn_R;			//世界法线增量
		int		curY, endY;
		VEC3	clr_L, clr_R;		//顶点color
		VEC3	dclr_L, dclr_R;		//顶点color增量
		VEC2	curUV_L, curUV_R;	//UV
		VEC2	duv_L, duv_R;		//UV增量
		VEC3	curLightDir_L, curLightDir_R;	// Light dir in tangent space
		VEC3	dLightDir_L, dLightDir_R;
		VEC3	curHVector_L, curHVector_R;		// H-vector in tangent space
		VEC3	dHVector_L, dHVector_R;
		int		texLod;
	};

	struct SScanLine 
	{
		VEC3	curClr, curPW, curN, finalPW, finalN;
		VEC3	deltaClr, deltaPW, deltaN;
		SColor	pixelColor, curPixelClr;
		VEC2	curUV, finalUV;
		VEC2	deltaUV;
		int		lineX0, lineX1;
		float	z, dz, w, dw;
	};
	

	class RasGouraud : public Rasterizer
	{
	public:
		virtual eRasterizeType	GetType()	{ return eRasterizeType_Gouraud; }
		virtual void	DoPerVertexLighting(VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj);
		virtual void	LerpVertexAttributes(SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type);
		virtual void	RasTriangleSetup(SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type);

	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context);
	};

	/////////////////////////////////////////////////////////////
	//////// 纹理+gouraud
	class RasTexturedGouraud : public RasGouraud
	{
	public:
		virtual eRasterizeType	GetType()	{ return eRasterizeType_TexturedGouraud; }
		virtual void	LerpVertexAttributes(SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type);
		virtual void	RasTriangleSetup(SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type);

	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context);
	};

	/////////////////////////////////////////////////////////////
	//////// Blinn-Phong光照,Phong模型是逐像素光照
	class RasBlinnPhong : public Rasterizer
	{
	public:
		virtual eRasterizeType	GetType()	{ return eRasterizeType_BlinnPhong; }
		virtual void	DoPerPixelLighting(SColor& result, const VEC3& worldPos, const VEC3& worldNormal, const VEC2& uv, const SMaterial* pMaterial);
		virtual void	LerpVertexAttributes(SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type);
		virtual void	RasTriangleSetup(SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type);

	protected:
		virtual void	_RasterizeTriangle(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, const SFace& face, const SRenderContext& context);
	};

	/////////////////////////////////////////////////////////////
	//////// Phong + 法线贴图
	class RasPhongWithNormalMap : public RasBlinnPhong
	{
	public:
		virtual eRasterizeType	GetType()	{ return eRasterizeType_PhongWithNormalMap; }
		// Use TBN matrix in VS
		virtual void	DoPerVertexLighting(VertexBuffer& workingVB, FaceList& workingFaces, RenderObject& obj);
		// Apply normal mapping in PS
		virtual void	DoPerPixelLighting(SColor& result, const VEC3& worldPos, const VEC3& worldNormal, const VEC2& uv, const SMaterial* pMaterial);

		virtual void	LerpVertexAttributes(SVertex* dest, const SVertex* src1, const SVertex* src2, float t, eLerpType type);
		virtual void	RasTriangleSetup(SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type);
	};
}


#endif // Rasterizer_h__