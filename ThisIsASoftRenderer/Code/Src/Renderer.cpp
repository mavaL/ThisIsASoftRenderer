#include "stdafx.h"
#include "Renderer.h"
#include "PixelBox.h"
#include "Scene.h"
#include "Profiler.h"
#include "RenderUtil.h"
#include "ThreadPool/MyJob.h"
#include "RayTracer.h"

namespace SR
{
	Renderer::Renderer()
	:m_curRas(nullptr)
	,m_ambientColor(SColor::WHITE)
	,m_curScene(-1)
	,m_bZTestEnable(true)
	,m_bZWriteEnable(true)
	,m_frameBuffer(nullptr)
	,m_iCurOutputZBuffer(0)
	,m_rayTracer(nullptr)
	{
		
	}

	void Renderer::Init()
	{
		m_rayTracer = new RayTracer;
		m_rayTracer->RunIntersectUnitTest();

		//初始化所有光栅器
		m_rasLib.insert(std::make_pair(eRasterizeType_Wireframe, new RasWireFrame));
		m_rasLib.insert(std::make_pair(eRasterizeType_Flat, new RasFlat));
		m_rasLib.insert(std::make_pair(eRasterizeType_Gouraud, new RasGouraud));
		m_rasLib.insert(std::make_pair(eRasterizeType_TexturedGouraud, new RasTexturedGouraud));
		m_rasLib.insert(std::make_pair(eRasterizeType_BlinnPhong, new RasBlinnPhong));
		m_rasLib.insert(std::make_pair(eRasterizeType_NormalMap, new RasNormalMap));

		//创建后备缓冲
		m_backBuffer.reset(new SR::PixelBox(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_MODE));

		SetRenderTarget(m_backBuffer.get());

		for(int i=0; i<OIT_LAYER; ++i)
			m_backBuffer_OIT[i].reset(new SR::PixelBox(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_MODE));

		int bmWidth = m_backBuffer->GetWidth();
		int bmHeight = m_backBuffer->GetHeight();
		int bmPitch = m_backBuffer->GetPitch();
		BYTE* data = (BYTE*)m_backBuffer->GetDataPointer();

		m_bmBackBuffer.reset(new Gdiplus::Bitmap(bmWidth, bmHeight, bmPitch, PixelFormat32bppARGB, data));

		//创建z-buffer
		m_zBuffer[0].reset(new SR::PixelBox(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_MODE));
		m_zBuffer[1].reset(new SR::PixelBox(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_MODE));

		m_zFunc[0] = eZFunc_Less;
		m_zFunc[1] = eZFunc_Less;

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
		SAFE_DELETE(m_rayTracer);
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

		Scene* pCurScene = m_scenes[m_curScene];

		// Using ray tracer or not
		if (pCurScene->IsEnableRayTracing())
		{
			m_rayTracer->ProcessScene(pCurScene);
		}
		else
		{
			// Render solids
			_FlushRenderList(pCurScene->m_renderList_solid);

			// Render transparency, using OIT, turn off z-write
			const bool bHasTrans = !pCurScene->m_renderList_trans.empty();
			if(bHasTrans)
			{
#if USE_OIT == 1
				_RenderTransparency_OIT();

				// Recover render states
				SetRenderTarget(nullptr);
				SetZfunc(0, eZFunc_Less);
				SetZfunc(1, eZFunc_Always);
#else
				SetEnableZWrite(false);
				_FlushRenderList(pCurScene->m_renderList_trans);
				SetEnableZWrite(true);
#endif
			}
		}
	}

	void Renderer::_RenderTransparency_OIT()
	{
		// First pass
		SetZfunc(1, eZFunc_Always);
		SetRenderTarget(m_backBuffer_OIT[0].get());
		_FlushRenderList(m_scenes[m_curScene]->m_renderList_trans);

		// Remain passes
		for (int i=1; i<OIT_LAYER; ++i)
		{
			int iOutputZBuffer = i % 2;
			int iReadOnlyZBuffer = (i+1) % 2;

			const float fMaxDepth = 1.0f;
			const DWORD dwMaxDepth = *(int*)&fMaxDepth;
			_ClearBufferImpl(m_zBuffer[iOutputZBuffer].get(), dwMaxDepth);

			SetRenderTarget(m_backBuffer_OIT[i].get(), iOutputZBuffer);
			SetZfunc(iOutputZBuffer, eZFunc_Less);
			SetZfunc(iReadOnlyZBuffer, eZFunc_Greater);
			_FlushRenderList(m_scenes[m_curScene]->m_renderList_trans);
		}

		// Final blending, far layer to near layer
		for (int i=OIT_LAYER-1; i>=0; --i)
		{
			PixelBox* pSrcLayer = m_backBuffer_OIT[i].get();
			PixelBox* pDestLayer = m_backBuffer.get();

			//TODO: How to optimize?
			const DWORD* pSrcData = (DWORD*)pSrcLayer->GetDataPointer();
			DWORD* pDestData = (DWORD*)pDestLayer->GetDataPointer();

			for (int x=0; x<pDestLayer->GetHeight(); ++x)
			{
				for (int y=0; y<pDestLayer->GetWidth(); ++y)
				{
					SColor c1, c2;
					c1.SetAsInt(*pSrcData++);
					c2.SetAsInt(*pDestData);

					Ext::LinearLerp(c1, c2, c1, c1.a);
					c1.Saturate();

					*pDestData++ = c1.GetAsInt();
				}
			}
		}
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
		const DWORD dwColor = color.GetAsInt();
		const DWORD dwDepth = *(int*)&depth;

		//clear backbuffer
		_ClearBufferImpl(m_backBuffer.get(), dwColor);

		//clear z-buffer
		_ClearBufferImpl(m_zBuffer[0].get(), dwDepth);

		//clear OIT buffers
#if USE_OIT == 1
		const bool bHasTrans = !m_scenes[m_curScene]->m_renderList_trans.empty();
		if(bHasTrans)
		{
			const DWORD dwClrNoAlpha = dwColor & 0x00ffffff;

			for (int i=0; i<OIT_LAYER; ++i)
				_ClearBufferImpl(m_backBuffer_OIT[i].get(), dwClrNoAlpha);
		}
#endif

		//clear fragment buffer
		DWORD nBuffer = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(SFragment) / sizeof(int);
		void* dst = &m_fragmentBuffer[0];

		_asm
		{
				mov edi, dst
				mov ecx, nBuffer
				mov eax, 0
				rep stosd 
		}
	}

	void Renderer::_ClearBufferImpl( PixelBox* pBuffer, DWORD val )
	{
		DWORD nBuffer = pBuffer->GetWidth() * pBuffer->GetHeight();
		void* dst = pBuffer->GetDataPointer();

		_asm
		{
				mov edi, dst
				mov ecx, nBuffer
				mov eax, val
				rep stosd 
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

	void Renderer::SetRenderTarget( PixelBox* pRT, int iZBuffer )
	{
		m_frameBuffer = pRT ? pRT : m_backBuffer.get();
		m_iCurOutputZBuffer = iZBuffer;
	}

	void Renderer::SetZfunc( int i, eZFunc func )
	{
		assert(i == 0 || i == 1);
		m_zFunc[i] = func;
	}
}