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

extern const int	SCREEN_WIDTH;
extern const int	SCREEN_HEIGHT;

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

	public:
		//设置渲染模式(wireframe, flat, gouround, phong)
		void	SetRasterizeType(eRasterizeType type);
		//渲染管线
		void	RenderOneFrame();
		//插入渲染图元
		void	AddRenderable(const VertexBuffer& vb, const IndexBuffer& ib);

	private:
		void	_Clear();
		//交换前后缓冲
		void	_Present();

	private:
		std::unique_ptr<Common::PixelBox>	m_backBuffer;
		std::unordered_map<eRasterizeType, Rasterizer*>	m_rasLib;		//所有可用光栅化器
		Rasterizer*							m_curRas;					//当前使用光栅化器
		SRenderList							m_renderList;				//渲染列表
		VertexBuffer						m_VB;
		IndexBuffer							m_IB;
	};

	class RenderUtil
	{
	public:
		///////	根据屏幕区域对直线裁剪,取自<<3D编程大师技巧>>
		static int	ClipLine(int& x1, int& y1, int& x2, int& y2);
		///////	Bresenahams画线算法,取自<<3D编程大师技巧>>
		static void	DrawLine_Bresenahams(int x0, int y0, int x1, int y1, int color, bool bClip);
		///////	最简单的DDA画线算法
		static void	DrawLine_DDA(int x0, int y0, int x1, int y1, int color, bool bClip);
		/////// GDI绘制字体
		void	DrawText(int x, int y, const STRING& text);
	};
}

extern SR::Renderer		g_renderer;	

#endif // Renderer_h__