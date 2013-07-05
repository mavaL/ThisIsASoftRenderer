#include "stdafx.h"
#include "Renderer.h"
#include "PixelBox.h"

namespace SR
{
	Renderer::Renderer()
		:m_hwnd(nullptr)
		,m_curRas(nullptr)
	{
		//这是世界坐标
		m_testTri.vert[0].pos = Common::SVector3(0, 0, -50);
		m_testTri.vert[1].pos = Common::SVector3(40, 0, -50);
		m_testTri.vert[2].pos = Common::SVector3(20, 30, -50);

		m_rasLib.insert(std::make_pair(eRasterizeType_FlatWire, new RasWireFrame));

		//创建后备缓冲
		m_backBuffer.reset(new Common::PixelBox(SCREEN_WIDTH, SCREEN_HEIGHT, 2));
	}

	Renderer::~Renderer()
	{
		for(auto iter=m_rasLib.begin(); iter!=m_rasLib.end(); ++iter)
			delete iter->second;
		m_rasLib.clear();
	}

	void Renderer::SetRasterizeType( eRasterizeType type )
	{
		auto iter = m_rasLib.find(type);
		assert(iter != m_rasLib.end());

		m_curRas = iter->second;
	}

	void Renderer::RenderOneFrame()
	{
		/////////////////////////////////////////////////
		///////// 刷新后备缓冲
		_Clear();

		//TODO:根据物体包围球在相机空间进行culling

		//TODO:世界空间进行背面剔除
		//		g_camera.GetViewPt();

		Common::SVector4 transPt[3];
		/////////////////////////////////////////////////
		///////// 相机变换
		g_camera.Update();
		auto matView = g_camera.GetViewMatrix();

		for (int i=0; i<3; ++i)
		{
			transPt[i] = Common::Transform_Vec3_By_Mat44(m_testTri.vert[i].pos, matView, true);
		}

		/////////////////////////////////////////////////
		///////// 透视投影变换
		float d = g_camera.GetNearClip();
		float ratio = SCREEN_WIDTH / (float)SCREEN_HEIGHT;
		for (int i=0; i<3; ++i)
		{
			transPt[i].x = transPt[i].x * d / transPt[i].z;
			transPt[i].y = transPt[i].y * d * ratio / transPt[i].z;
		}

		/////////////////////////////////////////////////
		///////// 齐次除法
		for (int i=0; i<3; ++i)
		{
			transPt[i].x /= transPt[i].w;
			transPt[i].y /= transPt[i].w;
			transPt[i].w = 1;
		}

		/////////////////////////////////////////////////
		///////// 视口映射
		float a = 0.5f * SCREEN_WIDTH - 0.5f;
		float b = 0.5f *SCREEN_HEIGHT - 0.5f;
		for (int i=0; i<3; ++i)
		{
			transPt[i].x = a + a * transPt[i].x;
			transPt[i].y = b - b * transPt[i].y;
		}

		/////////////////////////////////////////////////
		///////// 光栅化
		TriangleList tris;
		STriangle tri;
		tri.vert[0].pos = transPt[0].GetVec3(); 
		tri.vert[1].pos = transPt[1].GetVec3(); 
		tri.vert[2].pos = transPt[2].GetVec3();
		tris.push_back(tri);
		m_curRas->RasterizeTriangleList(tris);

		/////////////////////////////////////////////////
		///////// 最后进行swap
		_Present();
	}

	void Renderer::DrawText( int x, int y, const STRING& text )
	{
		//TODO..
	}

	void Renderer::_Present()
	{
		HDC dc = ::GetDC(m_hwnd);
		assert(dc);

		RECT rect;
		GetClientRect(m_hwnd, &rect);
		const void* memory = m_backBuffer->GetDataPointer();

		BITMAPV4HEADER bi;
		ZeroMemory (&bi, sizeof(bi));
		bi.bV4Size = sizeof(BITMAPINFOHEADER);
		bi.bV4BitCount = m_backBuffer->GetBitsPerPixel();
		bi.bV4Planes = 1;
		bi.bV4Width = m_backBuffer->GetWidth();
		bi.bV4Height = -m_backBuffer->GetHeight();		//负的表示Y轴向下,见MSDN
		bi.bV4V4Compression = BI_RGB;
		// 			bi.bV4AlphaMask = 0x1 << 15;
		// 			bi.bV4RedMask = 0x1f << 10;
		// 			bi.bV4GreenMask = 0x1f << 5;
		// 			bi.bV4BlueMask = 0x1f;

		StretchDIBits(dc, 0,0, rect.right, rect.bottom,
			0, 0, m_backBuffer->GetWidth(), m_backBuffer->GetHeight(),
			memory, (const BITMAPINFO*)(&bi), DIB_RGB_COLORS, SRCCOPY);

		ReleaseDC(m_hwnd, dc);
	}

	void Renderer::_Clear()
	{
		int color = 0;
		int bufBytes = m_backBuffer->GetWidth() * m_backBuffer->GetHeight() * m_backBuffer->GetBytesPerPixel();
		memset(m_backBuffer->GetDataPointer(), color, bufBytes);
	}
}