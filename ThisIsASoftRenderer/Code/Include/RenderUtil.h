/********************************************************************
	created:	2013/08/12
	created:	12:8:2013   20:33
	filename: 	RenderUtil_h
	author:		maval
	
	purpose:	所有渲染算法所在
*********************************************************************/
#ifndef RenderUtil_h__
#define RenderUtil_h__

#include "Prerequiestity.h"
#include "GeometryDef.h"

namespace SR
{
	class RenderUtil
	{
	public:
		///////	根据屏幕区域对直线裁剪. Sutherland-hogdman算法
		static bool	ClipLine(int& x1, int& y1, int& x2, int& y2);

		/////// GDI绘制字体
		static void	DrawText(float x, float y, const STRING& text, const Gdiplus::Color& color);

		/////// 遍历所有顶点计算物体包围盒
		static void	ComputeAABB(RenderObject& obj);

		///////	DDA画线算法
		static void	DrawLine_DDA(int x0, int y0, int x1, int y1, const SColor& color);

		///////	物体T&L阶段
		static void	ObjectTnL(RenderObject& obj, SRenderContext& context);

		/////// 背面拣选.同时对被剔除的顶点做上标记,它们是不需要参与接下来的T&L的.
		static void	DoBackfaceCulling(VertexBuffer& vb, FaceList& workingFaces, RenderObject& obj);

		/////// 3D裁减.对近裁面会进行裁剪.
		static void	Do3DClipping(VertexBuffer& VB, FaceList& faces);

		//////	三角面光栅化前预处理
		static bool	PreDrawTriangle(const SVertex*& vert0, const SVertex*& vert1, const SVertex*& vert2, eTriangleShape& retType);

		///////	扫描线算法绘制三角面
		static void	DrawTriangle_Scanline(const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, SColor color);

		///////	扫描线算法绘制平底三角形
		static void	DrawBottomTri_Scanline(float x0, float y0, float x1, float y1, float x2, float y2, SColor color);

		///////	扫描线算法绘制平顶三角形
		static void	DrawTopTri_Scanline(float x0, float y0, float x1, float y1, float x2, float y2, SColor color);

		///////	根据画家算法对三角面列表进行排序
		static void	SortTris_PainterAlgorithm(const VertexBuffer& verts, FaceList& faces);

		///////	绘制三角形的第二个版本.线性插值所需的attribute
		static void	DrawTriangle_Scanline_V2(const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, const SRenderContext& context);
		static void	DrawBottomTri_Scanline_V2(const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, const SRenderContext& context);
		static void	DrawTopTri_Scanline_V2(const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, const SRenderContext& context);
		static void RasterizeScanLines(SScanLinesData& scanLineData);

		///////	进行Lambert光照,假定法线和光源向量都已归一化
		static void DoLambertLighting(SColor& result, const VEC3& normal, const VEC3& lightDir, const SMaterial* pMaterial);
	};
}

#endif // RenderUtil_h__