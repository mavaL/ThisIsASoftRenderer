/********************************************************************
	created:	30:6:2013   23:46
	filename	Renderer.h
	author:		maval

	purpose:	软渲染器管理类
*********************************************************************/
#ifndef Renderer_h__
#define Renderer_h__

#include "Prerequiestity.h"
#include "Camera.h"
#include "MathDef.h"
#include "Rasterizer.h"
#include "GeometryDef.h"
#include "Utility.h"
#include "RenderObject.h"

extern const int	SCREEN_WIDTH;
extern const int	SCREEN_HEIGHT;
extern const int	PIXEL_MODE;

namespace SR
{
	/////////////////////////////////////////////////////////////
	struct SRenderContext 
	{
		SRenderContext():verts(nullptr),faces(nullptr),pMaterial(nullptr) {}

		VertexBuffer*	verts;
		FaceList*		faces;
		SMaterial*		pMaterial;
	};

	/////////////////////////////////////////////////////////////
	class Renderer
	{
		friend class RenderUtil;
	public:
		Renderer();
		~Renderer();

		//渲染数据统计
		struct SFrameStatics 
		{
			void Reset()
			{
				nObjCulled = nBackFace = nFaceCulled = nRenderedFace = 0;
			}
			DWORD	nObjCulled;
			DWORD	nBackFace;
			DWORD	nFaceCulled;
			DWORD	nRenderedFace;
			DWORD	lastFPS;
		};

		SFrameStatics	m_frameStatics;
		Camera			m_camera;
		SDirectionLight	m_testLight;	//测试方向光
		SColor			m_ambientColor;	

	public:
		void	Init();
		void	SetRasterizeType(eRasterizeType type);
		void	OnFrameMove();
		//切换渲染模式(wireframe, flat, gouraud, phong)
		void	ToggleShadingMode();
		//渲染管线
		void	RenderOneFrame();
		//交换前后缓冲
		void	Present();
		//添加渲染对象
		void	AddRenderable(const RenderObject& obj);
		void	AddRenderObjs(const RenderList& objs);
		//添加材质
		void	AddMaterial(const STRING& name, const SMaterial* mat);
		//获取材质
		SMaterial*	GetMaterial(const STRING& name);

	private:
		void	_Clear(const SColor& color, float depth);
		//背面拣选.同时对被剔除的顶点做上标记,它们是不需要参与接下来的T&L的.
		VertexBuffer	_DoBackfaceCulling(FaceList& workingFaces, RenderObject& obj);
		//3D裁减.对近裁面会进行裁剪.
		void			_Do3DClipping(VertexBuffer& VB, FaceList& faces);

	private:
		std::unique_ptr<Gdiplus::Bitmap>	m_bmBackBuffer;
		std::unique_ptr<Common::PixelBox>	m_backBuffer;
		std::unique_ptr<Common::PixelBox>	m_zBuffer;

		std::unordered_map<eRasterizeType, Rasterizer*>	m_rasLib;		//所有可用shader
		Rasterizer*							m_curRas;					//当前使用shader

		std::unordered_map<STRING, SMaterial*>	m_matLib;				//材质库

		RenderList							m_renderList;				//渲染列表
	};

	class RenderUtil
	{
	public:
		///////	根据屏幕区域对直线裁剪,取自<<3D编程大师技巧>>
		static int	ClipLine(int& x1, int& y1, int& x2, int& y2);
		/////// GDI绘制字体
		static void	DrawText(float x, float y, const STRING& text, const Gdiplus::Color& color);
		/////// 遍历所有顶点计算物体包围盒
		static void	ComputeAABB(RenderObject& obj);
		///////	Bresenahams画线算法,取自<<3D编程大师技巧>>
		static void	DrawLine_Bresenahams(int x0, int y0, int x1, int y1, SColor color, bool bClip);
		///////	最简单的DDA画线算法
		static void	DrawLine_DDA(int x0, int y0, int x1, int y1, SColor color, bool bClip);
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
		static void	DrawTriangle_Scanline_V2(const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, bool bTextured, bool bPerPixel, const SRenderContext& context);
		static void	DrawBottomTri_Scanline_V2(const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, bool bTextured, bool bPerPixel, const SRenderContext& context, RasGouraud::SScanLineData& scanLineData);
		static void	DrawTopTri_Scanline_V2(const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, bool bTextured, bool bPerPixel, const SRenderContext& context, RasGouraud::SScanLineData& scanLineData);
		static void DrawScanLines(RasGouraud::SScanLineData& scanLineData, bool bTextured, bool bPerPixel, const SRenderContext& context);
	};
}

#endif // Renderer_h__