/********************************************************************
	created:	30:6:2013   23:46
	filename	Renderer.h
	author:		maval

	purpose:	软渲染器管理类
*********************************************************************/
#ifndef Renderer_h__
#define Renderer_h__

#include "Camera.h"
#include "MathDef.h"
#include "Rasterizer.h"
#include "GeometryDef.h"
#include "Utility.h"

extern const int	SCREEN_WIDTH;
extern const int	SCREEN_HEIGHT;
extern const int	PIXEL_MODE;

namespace Common
{
	class PixelBox;
}

namespace SR
{
	class Renderer
	{
		friend class RenderUtil;
	public:
		Renderer();
		~Renderer();

		HWND		m_hwnd;
		Camera		m_camera;
		SDirectionLight	m_testLight;	//测试方向光

	public:
		//设置渲染模式(wireframe, flat, gouround, phong)
		void	SetRasterizeType(eRasterizeType type);
		//渲染管线
		void	RenderOneFrame();
		//插入渲染图元
		void	AddRenderable(const SRenderObj& obj);
		const RenderList&	GetRenderList() const { return m_renderList; }

	private:
		void	_Clear();
		//背面拣选.同时对被剔除的顶点做上标记,它们是不需要参与接下来的T&L的.
		VertexBuffer	_DoBackfaceCulling(SRenderObj& obj);
		//交换前后缓冲
		void	_Present();

	private:
		std::unique_ptr<Common::PixelBox>	m_backBuffer;
		std::unordered_map<eRasterizeType, Rasterizer*>	m_rasLib;		//所有可用光栅化器
		Rasterizer*							m_curRas;					//当前使用光栅化器
		RenderList							m_renderList;				//渲染列表
	};

	class RenderUtil
	{
	public:
		///////	根据屏幕区域对直线裁剪,取自<<3D编程大师技巧>>
		static int	ClipLine(int& x1, int& y1, int& x2, int& y2);
		/////// GDI绘制字体
		static void	DrawText(int x, int y, const STRING& text, const COLORREF color);
		/////// 根据物体顶点计算包围球球径. NB:注意尽量保持物体中心与原点接近,否则误差较大.
		static float	ComputeBoundingRadius(const VertexBuffer& verts);
		///////	Bresenahams画线算法,取自<<3D编程大师技巧>>
		static void	DrawLine_Bresenahams(int x0, int y0, int x1, int y1, int color, bool bClip);
		///////	最简单的DDA画线算法
		static void	DrawLine_DDA(int x0, int y0, int x1, int y1, int color, bool bClip);
		///////	扫描线算法绘制三角面. NB:顺序应该是逆时针.
		static void	DrawTriangle_Scanline(int x0, int y0, int x1, int y1, int x2, int y2, int color);
		///////	扫描线算法绘制平底三角形
		static void	DrawBottomTri_Scanline(int x0, int y0, int x1, int y1, int x2, int y2, int color);
		///////	扫描线算法绘制平顶三角形
		static void	DrawTopTri_Scanline(int x0, int y0, int x1, int y1, int x2, int y2, int color);
	};
}

extern SR::Renderer		g_renderer;	

#endif // Renderer_h__