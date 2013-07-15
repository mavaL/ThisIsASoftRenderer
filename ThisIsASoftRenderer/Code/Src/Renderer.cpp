#include "stdafx.h"
#include "Renderer.h"
#include "PixelBox.h"

//像素模式(字节数). NB:只支持32位模式,不要更改
const int PIXEL_MODE	=	4;

namespace SR
{
	Renderer::Renderer()
		:m_hwnd(nullptr)
		,m_curRas(nullptr)
	{
		//初始化所有光栅器
		m_rasLib.insert(std::make_pair(eRasterizeType_Wireframe, new RasWireFrame));
		m_rasLib.insert(std::make_pair(eRasterizeType_Flat, new RasFlat));
		m_rasLib.insert(std::make_pair(eRasterizeType_Gouraud, new RasGouraud));

		//创建后备缓冲
		m_backBuffer.reset(new Common::PixelBox(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_MODE));

		m_testLight.dir = VEC3(0.3f,-1,-1);
		m_testLight.dir.Normalize();
		m_testLight.color = SColor::WHITE;
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
		m_camera.Update();

		/////////////////////////////////////////////////
		///////// 刷新后备缓冲
		_Clear();

		//for each object
		for (size_t iObj=0; iObj<m_renderList.size(); ++iObj)
		{
			SRenderObj& obj = m_renderList[iObj];
			obj.ResetState();

			/////////////////////////////////////////////////
			///////// 视锥裁减
			if(m_camera.ObjectFrustumCulling(obj))
			{
				obj.m_bCull = true;
				continue;
			}

			/////////////////////////////////////////////////
			///////// 世界空间进行背面剔除
			VertexBuffer workingVB = _DoBackfaceCulling(obj);

			/////////////////////////////////////////////////
			///////// 世界空间进行光照
			m_curRas->DoLighting(workingVB, obj, m_testLight);

			//transform each vertex
			for (size_t iVert=0; iVert<workingVB.size(); ++iVert)
			{
				if(!workingVB[iVert].bActive)
					continue;

				VEC4& vertPos = workingVB[iVert].pos;

				/////////////////////////////////////////////////
				///////// 相机变换
				auto matView = m_camera.GetViewMatrix();
				vertPos = Common::Transform_Vec4_By_Mat44(vertPos, matView);

				/////////////////////////////////////////////////
				///////// 透视投影变换
				auto matProj = m_camera.GetProjMatrix();
				vertPos = Common::Transform_Vec4_By_Mat44(vertPos, matProj);

				/////////////////////////////////////////////////
				///////// 齐次除法
				float inv_w = 1 / vertPos.w;
				vertPos.x *= inv_w;
				vertPos.y *= inv_w;
				vertPos.z *= inv_w;

				/////////////////////////////////////////////////
				///////// 视口映射 [-1,1] -> [0, Viewport W/H]
				float a = 0.5f * SCREEN_WIDTH;
				float b = 0.5f *SCREEN_HEIGHT;

				vertPos.x = a + a * vertPos.x;
				vertPos.y = b - b * vertPos.y;
			}

			/////////////////////////////////////////////////
			///////// 光栅化物体
			m_curRas->RasterizeTriangleList(workingVB, obj.faces);
		}

		/////////////////////////////////////////////////
		///////// 最后进行swap
		_Present();
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
		int bufBytes = m_backBuffer->GetWidth() * m_backBuffer->GetHeight() * m_backBuffer->GetBytesPerPixel();
		memset(m_backBuffer->GetDataPointer(), 0, bufBytes);
	}

	void Renderer::AddRenderable(const SRenderObj& obj)
	{
		m_renderList.push_back(obj);
	}

	VertexBuffer Renderer::_DoBackfaceCulling( SRenderObj& obj )
	{
		VertexBuffer vb;
		vb.assign(obj.VB.begin(), obj.VB.end());

		const VEC4& camPos = m_camera.GetPos();

		for (size_t i=0; i<obj.faces.size(); ++i)
		{
			SFace& face = obj.faces[i];
			
			//fetch vertexs
			const SR::Index idx1 = face.index1;
			const SR::Index idx2 = face.index2;
			const SR::Index idx3 = face.index3;

			const VEC4& pos1 = obj.VB[idx1].pos;
			const VEC4& pos2 = obj.VB[idx2].pos;
			const VEC4& pos3 = obj.VB[idx3].pos;

			VEC4 faceToCam = Common::Add_Vec4_By_Vec4(Common::Add_Vec4_By_Vec4(pos1, pos2), pos3);
			faceToCam = Common::Multiply_Vec4_By_K(faceToCam, 0.33333f);
			faceToCam.w = 1;
			faceToCam = Common::Transform_Vec4_By_Mat44(faceToCam, obj.matWorld);
			faceToCam = Common::Sub_Vec4_By_Vec4(camPos, faceToCam);

			VEC4 faceWorldNormal = Common::Transform_Vec3_By_Mat44(face.faceNormal, obj.matWorld, false);

			if(Common::DotProduct_Vec3_By_Vec3(faceToCam.GetVec3(), faceWorldNormal.GetVec3()) <= 0.0f)
			{
				face.IsBackface = true;
			}
			else
			{
				vb[idx1].bActive = true;
				vb[idx2].bActive = true;
				vb[idx3].bActive = true;
			}
		}

		return std::move(vb);
	}
}