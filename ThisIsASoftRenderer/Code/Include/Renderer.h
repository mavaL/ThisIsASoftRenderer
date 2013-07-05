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

typedef std::string STRING;

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

		HWND	m_hwnd;

	public:
		void	SetRasterizeType(eRasterizeType type);
		//渲染管线
		void	RenderOneFrame();
		//GDI绘制字体
		void	DrawText(int x, int y, const STRING& text);

	private:
		void	_Clear();
		//交换前后缓冲
		void	_Present();

	private:
		STriangle							m_testTri;
		std::unique_ptr<Common::PixelBox>	m_backBuffer;
		std::unordered_map<eRasterizeType, Rasterizer*>	m_rasLib;		//所有可用光栅化器
		Rasterizer*							m_curRas;		//当前使用光栅化器
	};

	class RenderUtil
	{
	public:
		///////	根据屏幕区域对直线裁剪,取自<<3D编程大师技巧>>
		static int	ClipLine(int& x1, int& y1, int& x2, int& y2);
		///////	Bresenahams画线算法,取自<<3D编程大师技巧>>
		static void	DrawClipLine_Bresenahams(int x0, int y0, int x1, int y1, int color);
		///////	最简单的DDA画线算法
		static void	DrawClipLine_DDA(int x0, int y0, int x1, int y1, int color);
	};
}

extern SR::Renderer		g_renderer;	
extern SR::Camera	g_camera;

#endif // Renderer_h__