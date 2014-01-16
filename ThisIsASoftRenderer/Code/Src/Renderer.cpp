#include "stdafx.h"
#include "Renderer.h"
#include "PixelBox.h"
#include "Scene.h"
#include "Profiler.h"
#include "RenderUtil.h"
#include "ThreadPool/MyJob.h"

namespace SR
{
	Renderer::Renderer()
	:m_curRas(nullptr)
	,m_ambientColor(SColor::WHITE)
	,m_curScene(-1)
	{
		
	}

	void Renderer::Init()
	{
		//初始化所有光栅器
		m_rasLib.insert(std::make_pair(eRasterizeType_Wireframe, new RasWireFrame));
		m_rasLib.insert(std::make_pair(eRasterizeType_Flat, new RasFlat));
		m_rasLib.insert(std::make_pair(eRasterizeType_Gouraud, new RasGouraud));
		m_rasLib.insert(std::make_pair(eRasterizeType_TexturedGouraud, new RasTexturedGouraud));
		m_rasLib.insert(std::make_pair(eRasterizeType_BlinnPhong, new RasBlinnPhong));
		m_rasLib.insert(std::make_pair(eRasterizeType_NormalMap, new RasNormalMap));

		//创建后备缓冲
		m_backBuffer.reset(new SR::PixelBox(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_MODE));

		int bmWidth = m_backBuffer->GetWidth();
		int bmHeight = m_backBuffer->GetHeight();
		int bmPitch = m_backBuffer->GetPitch();
		BYTE* data = (BYTE*)m_backBuffer->GetDataPointer();

		m_bmBackBuffer.reset(new Gdiplus::Bitmap(bmWidth, bmHeight, bmPitch, PixelFormat32bppARGB, data));

		//创建z-buffer
		m_zBuffer.reset(new SR::PixelBox(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_MODE));

		//创建fragment buffer
		m_fragmentBuffer = new SFragment[SCREEN_WIDTH * SCREEN_HEIGHT];

		//测试方向光
		m_testLight.dir = VEC3(-0.3f,-1,-1);
		m_testLight.dir.Normalize();
		m_testLight.neg_dir = m_testLight.dir;
		m_testLight.neg_dir.Neg();
		m_testLight.color.Set(0.7f, 0.7f, 0.7f);

		m_ambientColor.Set(0.4f, 0.4f, 0.4f);

		_InitAllScene();

		ToggleScene();
	}

	Renderer::~Renderer()
	{
		SAFE_DELETE_ARRAY(m_fragmentBuffer);

		std::for_each(m_scenes.begin(), m_scenes.end(), std::default_delete<Scene>());
		m_scenes.clear();

		for(auto iter=m_rasLib.begin(); iter!=m_rasLib.end(); ++iter)
			delete iter->second;
		m_rasLib.clear();

		for(auto iter=m_matLib.begin(); iter!=m_matLib.end(); ++iter)
			delete iter->second;
		m_matLib.clear();
	}

	Rasterizer* Renderer::GetRasterizer( eRasterizeType type )
	{
		auto iter = m_rasLib.find(type);
		assert(iter != m_rasLib.end());

		return iter->second;
	}

	void Renderer::OnFrameMove()
	{
#if USE_PROFILER == 1
		//update FPS
		DWORD curTime = GetTickCount();
		static DWORD lastTime = curTime;
		static DWORD passedFrameCnt = 0;

		DWORD passedTime = curTime - lastTime;
		++passedFrameCnt;

		if(passedTime >= 1000)
		{
			g_env.profiler->m_frameStatics.lastFPS = (DWORD)(passedFrameCnt / (float)passedTime * 1000);
			lastTime = curTime;
			passedFrameCnt = 0;
		}
#endif

		for (size_t iObj=0; iObj<m_scenes[m_curScene]->m_renderList_solid.size(); ++iObj)
		{
			RenderObject* obj = m_scenes[m_curScene]->m_renderList_solid[iObj];
			obj->OnFrameMove();
		}

		for (size_t iObj=0; iObj<m_scenes[m_curScene]->m_renderList_trans.size(); ++iObj)
		{
			RenderObject* obj = m_scenes[m_curScene]->m_renderList_trans[iObj];
			obj->OnFrameMove();
		}

		g_env.profiler->m_frameStatics.Reset();
		m_camera.Update();
	}

	void Renderer::RenderOneFrame()
	{
		//刷新后备缓冲
		_Clear(SColor::NICE_BLUE, 1.0f);

		// Render solids
		_FlushRenderList(m_scenes[m_curScene]->m_renderList_solid);

		// Render transparency
		_FlushRenderList(m_scenes[m_curScene]->m_renderList_trans);
	}

	void Renderer::Present()
	{
		HWND hwnd = g_env.hwnd;
		HDC dc = ::GetDC(hwnd);
		assert(dc);

		RECT rect;
		GetClientRect(hwnd, &rect);
		const void* memory = m_backBuffer->GetDataPointer();

		BITMAPV4HEADER bi;
		ZeroMemory (&bi, sizeof(bi));
		bi.bV4Size = sizeof(BITMAPINFOHEADER);
		bi.bV4BitCount = m_backBuffer->GetBitsPerPixel();
		bi.bV4Planes = 1;
		bi.bV4Width = m_backBuffer->GetWidth();
		bi.bV4Height = -m_backBuffer->GetHeight();		//负的表示Y轴向下,见MSDN
		bi.bV4V4Compression = BI_RGB;

		StretchDIBits(dc, 0,0, rect.right, rect.bottom,
			0, 0, m_backBuffer->GetWidth(), m_backBuffer->GetHeight(),
			memory, (const BITMAPINFO*)(&bi), DIB_RGB_COLORS, SRCCOPY);

		ReleaseDC(hwnd, dc);
	}

	void Renderer::_Clear(const SColor& color, float depth)
	{
		//clear backbuffer
		{
			DWORD nBuffer = m_backBuffer->GetWidth() * m_backBuffer->GetHeight();
			void* dst = m_backBuffer->GetDataPointer();
			DWORD clr = color.GetAsInt();

			_asm
			{
				mov edi, dst
				mov ecx, nBuffer
				mov eax, clr
				rep stosd 
			}
		}

		//clear z-buffer
		{
			DWORD nBuffer = m_zBuffer->GetWidth() * m_zBuffer->GetHeight();
			void* dst = m_zBuffer->GetDataPointer();
			int d = *(int*)&depth;

			_asm
			{
				mov edi, dst
				mov ecx, nBuffer
				mov eax, d
				rep stosd 
			}
		}

		//clear fragment buffer
		{
			static DWORD nBuffer = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(SFragment) / sizeof(int);
			void* dst = &m_fragmentBuffer[0];

			_asm
			{
				mov edi, dst
				mov ecx, nBuffer
				mov eax, 0
				rep stosd 
			}
		}
	}

	void Renderer::AddMaterial( const STRING& name, SMaterial* mat )
	{
		auto iter = m_matLib.find(name);
		if(iter != m_matLib.end())
		{
			STRING errMsg("Error, ");
			errMsg += name;
			errMsg += " already exist in AddMaterial()!";
			throw std::logic_error(errMsg);
			return;
		}

		//预计算加速
		mat->ambient *= m_ambientColor;
		mat->diffuse *= m_testLight.color;
		mat->specular *= m_testLight.color;

		m_matLib.insert(std::make_pair(name, const_cast<SMaterial*>(mat)));
	}

	SMaterial* Renderer::GetMaterial( const STRING& name )
	{
		auto iter = m_matLib.find(name);
		if(iter == m_matLib.end())
		{
			STRING errMsg("Error, ");
			errMsg += name;
			errMsg += " doesn't exist in GetMaterial()!";
			throw std::logic_error(errMsg);
			return nullptr;
		}

		return iter->second;
	}

	void Renderer::ToggleScene()
	{
		++m_curScene;
		if(m_curScene == m_scenes.size())
			m_curScene = 0;

		m_scenes[m_curScene]->Enter();
	}

	void Renderer::_FlushRenderList( RenderList& renderList )
	{
		int nObj = (int)renderList.size();

		//for each object
		for (int iObj=0; iObj<nObj; ++iObj)
		{
			RenderObject& obj = *renderList[iObj];
			m_curRas = obj.m_pShader;

			//T&L
#if USE_MULTI_THREAD == 1
			JobParamVS* param = new JobParamVS;
			param->object = &obj;

			JobVS* job = new JobVS(param);
			g_env.jobMgr->SubmitJob(job, nullptr);
#else			
			SRenderContext context;
			context.pMaterial = obj.m_pMaterial;

			RenderUtil::ObjectTnL(obj, context);

			//光栅化物体
			m_curRas->RasterizeTriangleList(context);
#endif
		}
#if USE_MULTI_THREAD == 1
		g_env.jobMgr->Flush();

		//到这个阶段VS,RS已执行完,且fragment buffer保存了ps需要执行的像素
		int nPixel = SCREEN_WIDTH * SCREEN_HEIGHT;
		SFragment* curFrag = &m_fragmentBuffer[0];

		for (int i=0; i<nPixel; ++i)
		{
			if(curFrag->bActive)
			{
				JobParamPS* param = new JobParamPS;
				param->frag = curFrag;

				JobPS* job = new JobPS(param);
				g_env.jobMgr->SubmitJob(job, nullptr);
			}

			++curFrag;
		}

		g_env.jobMgr->Flush();
#endif
	}
}