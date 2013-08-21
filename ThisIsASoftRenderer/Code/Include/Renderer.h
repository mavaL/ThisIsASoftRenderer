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

namespace SR
{
	/////////////////////////////////////////////////////////////
	struct SRenderContext 
	{
		SRenderContext():pMaterial(nullptr) {}

		VertexBuffer	verts;
		FaceList		faces;
		SMaterial*		pMaterial;
	};

	/////////////////////////////////////////////////////////////
	class Renderer
	{
		friend class RenderUtil;
	public:
		Renderer();
		~Renderer();

		Camera			m_camera;
		SDirectionLight	m_testLight;		//测试方向光
		SColor			m_ambientColor;
		SFragment*		m_fragmentBuffer;	//这可是个内存大户

	public:
		void	Init();
		void	SetRasterizeType(eRasterizeType type);
		void	OnFrameMove();
		Rasterizer*	GetCurRas() { return m_curRas; }
		bool	IsCurSponzaScene();
		//切换渲染模式(wireframe, flat, gouraud, phong)
		void	ToggleShadingMode();
		//获取当前渲染模式名字
		const char*	GetCurShadingModeName() const;
		//切换测试场景
		void	ToggleScene();
		//渲染管线
		void	RenderOneFrame();
		//交换前后缓冲
		void	Present();
		//添加材质
		void	AddMaterial(const STRING& name, SMaterial* mat);
		//获取材质
		SMaterial*	GetMaterial(const STRING& name);

	private:
		void	_InitAllScene();
		//清除帧缓存,z-buffer
		void	_Clear(const SColor& color, float depth);

	private:
		std::unique_ptr<Gdiplus::Bitmap>	m_bmBackBuffer;
		std::unique_ptr<SR::PixelBox>	m_backBuffer;
		std::unique_ptr<SR::PixelBox>	m_zBuffer;

		std::unordered_map<eRasterizeType, Rasterizer*>	m_rasLib;		//所有可用shader
		Rasterizer*							m_curRas;					//当前使用shader

		std::unordered_map<STRING, SMaterial*>	m_matLib;				//材质库

		std::vector<Scene*>					m_scenes;					//所有测试场景
		size_t								m_curScene;					//当前场景索引
	};
}

#endif // Renderer_h__