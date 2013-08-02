#include "stdafx.h"
#include "Renderer.h"
#include "PixelBox.h"

//像素模式(字节数). NB:只支持32位模式,不要更改
const int PIXEL_MODE	=	4;

namespace SR
{
	Renderer::Renderer()
	:m_curRas(nullptr)
	,m_ambientColor(255,40,40,40)
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

		//创建后备缓冲
		m_backBuffer.reset(new Common::PixelBox(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_MODE));

		int bmWidth = m_backBuffer->GetWidth();
		int bmHeight = m_backBuffer->GetHeight();
		int bmPitch = m_backBuffer->GetPitch();
		BYTE* data = (BYTE*)m_backBuffer->GetDataPointer();

		m_bmBackBuffer.reset(new Gdiplus::Bitmap(bmWidth, bmHeight, bmPitch, PixelFormat32bppARGB, data));

		//创建z-buffer
		m_zBuffer.reset(new Common::PixelBox(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_MODE));

		//测试方向光
		m_testLight.dir = VEC3(-0.3f,-1,-1);
		m_testLight.dir.Normalize();
		m_testLight.color = SColor::WHITE;
	}

	Renderer::~Renderer()
	{
		for(auto iter=m_rasLib.begin(); iter!=m_rasLib.end(); ++iter)
			delete iter->second;
		m_rasLib.clear();

		for(auto iter=m_matLib.begin(); iter!=m_matLib.end(); ++iter)
			delete iter->second;
		m_matLib.clear();
	}

	void Renderer::SetRasterizeType( eRasterizeType type )
	{
		auto iter = m_rasLib.find(type);
		assert(iter != m_rasLib.end());

		m_curRas = iter->second;
	}

	void Renderer::OnFrameMove()
	{
		//update FPS
		DWORD curTime = GetTickCount();
		static DWORD lastTime = curTime;
		static DWORD passedFrameCnt = 0;

		DWORD passedTime = curTime - lastTime;
		++passedFrameCnt;

		if(passedTime >= 1000)
		{
			m_frameStatics.lastFPS = (DWORD)(passedFrameCnt / (float)passedTime * 1000);
			lastTime = curTime;
			passedFrameCnt = 0;
		}

		m_frameStatics.Reset();
		m_camera.Update();
	}

	void Renderer::RenderOneFrame()
	{
		/////////////////////////////////////////////////
		///////// 刷新后备缓冲
		_Clear(SColor::BLUE, 1.0f);

		//for each object
		for (size_t iObj=0; iObj<m_renderList.size(); ++iObj)
		{
			RenderObject& obj = m_renderList[iObj];
			obj.OnFrameMove();

			/////////////////////////////////////////////////
			///////// 视锥裁减
			if(m_camera.ObjectFrustumCulling(obj))
			{
				++m_frameStatics.nObjCulled;
				continue;
			}

			FaceList workingFaces(obj.m_faces.begin(), obj.m_faces.end());

			/////////////////////////////////////////////////
			///////// 世界空间进行背面剔除
			VertexBuffer workingVB = _DoBackfaceCulling(workingFaces, obj);

			/////////////////////////////////////////////////
			///////// 世界空间进行光照
			m_curRas->DoLighting(workingVB, workingFaces, obj, m_testLight);

			//transform each vertex
			for (size_t iVert=0; iVert<workingVB.size(); ++iVert)
			{
				if(!workingVB[iVert].bActive)
					continue;

				VEC4& vertPos = workingVB[iVert].pos;

				/////////////////////////////////////////////////
				///////// 世界变换
				Common::Transform_Vec4_By_Mat44(vertPos, vertPos, obj.m_matWorld);

				/////////////////////////////////////////////////
				///////// 相机变换
				auto matView = m_camera.GetViewMatrix();
				Common::Transform_Vec4_By_Mat44(vertPos, vertPos, matView);
			}

			/////////////////////////////////////////////////
			///////// 相机空间进行三角面级别的3D裁减
			///////// 必须在透视变换之前,不然小于近裁面的顶点会被反转
			_Do3DClipping(workingVB, workingFaces);

			//继续顶点变换流水线..
			for (size_t iVert=0; iVert<workingVB.size(); ++iVert)
			{
				if(!workingVB[iVert].bActive)
					continue;

				VEC4& vertPos = workingVB[iVert].pos;

				/////////////////////////////////////////////////
				///////// 透视投影变换
				auto matProj = m_camera.GetProjMatrix();
				Common::Transform_Vec4_By_Mat44(vertPos, vertPos, matProj);

				/////////////////////////////////////////////////
				///////// 齐次除法
				vertPos.w = 1 / vertPos.w;
				vertPos.x *= vertPos.w;
				vertPos.y *= vertPos.w;
				vertPos.z *= vertPos.w;

				/////////////////////////////////////////////////
				///////// 视口映射 [-1,1] -> [0, Viewport W/H]
				float a = 0.5f * SCREEN_WIDTH;
				float b = 0.5f *SCREEN_HEIGHT;

				vertPos.x = a + a * vertPos.x;
				vertPos.y = b - b * vertPos.y;
			}

			/////////////////////////////////////////////////
			///////// 光栅化物体
			SRenderContext context;
			context.verts = &workingVB;
			context.faces = &workingFaces;
			context.pMaterial = obj.m_pMaterial;

			m_curRas->RasterizeTriangleList(context);
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
		//clear backbuffer
		{
			DWORD nBuffer = m_backBuffer->GetWidth() * m_backBuffer->GetHeight();
			void* dst = m_backBuffer->GetDataPointer();
			int clr = color.color;

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
	}

	void Renderer::AddRenderable(const RenderObject& obj)
	{
		m_renderList.push_back(obj);
	}

	void Renderer::AddRenderObjs( const RenderList& objs )
	{
		m_renderList.insert(m_renderList.end(), objs.begin(), objs.end());
	}

	VertexBuffer Renderer::_DoBackfaceCulling( FaceList& workingFaces, RenderObject& obj )
	{
		VertexBuffer vb;
		vb.assign(obj.m_verts.begin(), obj.m_verts.end());

		const VEC4& camPos = m_camera.GetPos();

		for (size_t i=0; i<workingFaces.size(); ++i)
		{
			SFace& face = workingFaces[i];
			
			//fetch vertexs
			const SR::Index idx1 = face.index1;
			const SR::Index idx2 = face.index2;
			const SR::Index idx3 = face.index3;

			const VEC4& pos1 = obj.m_verts[idx1].pos;
			const VEC4& pos2 = obj.m_verts[idx2].pos;
			const VEC4& pos3 = obj.m_verts[idx3].pos;

			VEC4 faceToCam = Common::Add_Vec4_By_Vec4(Common::Add_Vec4_By_Vec4(pos1, pos2), pos3);
			faceToCam = Common::Multiply_Vec4_By_K(faceToCam, 0.33333f);
			faceToCam.w = 1;
			Common::Transform_Vec4_By_Mat44(faceToCam, faceToCam, obj.m_matWorld);
			faceToCam = Common::Sub_Vec4_By_Vec4(camPos, faceToCam);

			VEC4 faceWorldNormal = Common::Transform_Vec3_By_Mat44(face.faceNormal, obj.m_matWorldIT, false);

			if(Common::DotProduct_Vec3_By_Vec3(faceToCam.GetVec3(), faceWorldNormal.GetVec3()) <= 0.0f)
			{
				face.IsBackface = true;
				++m_frameStatics.nBackFace;
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

	void Renderer::_Do3DClipping( VertexBuffer& VB, FaceList& faces )
	{
		const float n = m_camera.GetNearClip();
		const float f = m_camera.GetFarClip();
		const float fov = m_camera.GetFov();
		const float half_w = n * std::tan(fov/2);
		const float half_h = half_w / m_camera.GetAspectRatio();

		size_t nFaces = faces.size();
		for (size_t i=0; i<nFaces; ++i)
		{
			SFace& face = faces[i];

			if(face.IsBackface)
				continue;

			//fetch vertexs
			const SR::Index idx1 = face.index1;
			const SR::Index idx2 = face.index2;
			const SR::Index idx3 = face.index3;

			SVertex& vert1 = VB[idx1];
			SVertex& vert2 = VB[idx2];
			SVertex& vert3 = VB[idx3];

			//左右面
			float x1 = half_w * vert1.pos.z / -n;
			float x2 = half_w * vert2.pos.z / -n;
			float x3 = half_w * vert3.pos.z / -n;

			if(	(vert1.pos.x < -x1 && vert2.pos.x < -x2 && vert3.pos.x < -x3) ||
				(vert1.pos.x > x1 && vert2.pos.x > x2 && vert3.pos.x > x3)	)
			{
				face.bCulled = true;
				++m_frameStatics.nFaceCulled;
				continue;
			}

			//上下面
			float y1 = half_h * vert1.pos.z / -n;
			float y2 = half_h * vert2.pos.z / -n;
			float y3 = half_h * vert3.pos.z / -n;

			if(	(vert1.pos.y < -y1 && vert2.pos.y < -y2 && vert3.pos.y < -y3) ||
				(vert1.pos.y > y1 && vert2.pos.y > y2 && vert3.pos.y > y3)	)
			{
				face.bCulled = true;
				++m_frameStatics.nFaceCulled;
				continue;
			}

			//远裁面
			if(-vert1.pos.z > f && -vert2.pos.z > f && -vert3.pos.z > f)
			{
				face.bCulled = true;
				++m_frameStatics.nFaceCulled;
				continue;
			}

			// 近裁面,需要处理3种情况:
			int nVertOut = 0;
			bool flags[3] = { false };
			if(-vert1.pos.z < n) { flags[0] = true; ++nVertOut; }
			if(-vert2.pos.z < n) { flags[1] = true; ++nVertOut; }
			if(-vert3.pos.z < n) { flags[2] = true; ++nVertOut; }

			/*	1.完全在视锥外,则剔除:

				  ___________ near clip plane
					  p0
					  /\
					 /  \
					/____\
				  p1	  p2					*/

			if(flags[0] && flags[1] && flags[2])
			{
				face.bCulled = true;
				++m_frameStatics.nFaceCulled;
				continue;
			}

			/*	2.2个顶点在视锥内,1个顶点在视锥外,裁剪后需要分割为2个三角面:

				   p0______p2
				     \    /
				 _____\__/_____ near clip plane
				       \/
				       p1							*/

			else if(nVertOut == 1)
			{
				//找出内外的顶点
				SVertex *p0, *p1, *p2; 
				Index idxp0, idxp1, idxp2;
				if(flags[0])		{ p1 = &vert1; p0 = &vert2; p2 = &vert3; idxp1 = idx1; idxp0 = idx2; idxp2 = idx3; }
				else if(flags[1])	{ p1 = &vert2; p0 = &vert1; p2 = &vert3; idxp1 = idx2; idxp0 = idx1; idxp2 = idx3; }
				else				{ p1 = &vert3; p0 = &vert1; p2 = &vert2; idxp1 = idx3; idxp0 = idx1; idxp2 = idx2; }

				//直线参数化方程求t
				const VEC4 line1 = Common::Sub_Vec4_By_Vec4(p1->pos, p0->pos);
				float t1 = (-n - p0->pos.z)/(line1.z);
				//交点1
				float newX1 = p0->pos.x + line1.x * t1;
				float newY1 = p0->pos.y + line1.y * t1;

				//交点2得创建新的顶点和切割新的面
				const VEC4 line2 = Common::Sub_Vec4_By_Vec4(p1->pos, p2->pos);
				float t2 = (-n - p2->pos.z)/(line2.z);
				float newX2 = p2->pos.x + line2.x * t2;
				float newY2 = p2->pos.y + line2.y * t2;
	
				SVertex newVert;
				newVert.bActive = true;
				newVert.normal = p1->normal;
				newVert.pos.Set(newX2, newY2, -n, 1.0f);
				newVert.color = p1->color;

				SFace newFace;
				newFace.faceNormal = face.faceNormal;
				newFace.color = face.color;
				newFace.index1 = idxp2;
				newFace.index2 = idxp1;
				newFace.index3 = VB.size();

				//计算新的uv
				newVert.uv.x = p2->uv.x + (p1->uv.x - p2->uv.x) * t2;
				newVert.uv.y = p2->uv.y + (p1->uv.y - p2->uv.y) * t2;

				p1->uv.x = p0->uv.x + (p1->uv.x - p0->uv.x) * t1;
				p1->uv.y = p0->uv.y + (p1->uv.y - p0->uv.y) * t1;

				//交点1覆盖原来的p1
				p1->pos.x = newX1; p1->pos.y = newY1; p1->pos.z = -n;

				//NB: 最后进行插入操作,不然会使指向元素的指针无效化
				VB.push_back(newVert);
				faces.push_back(newFace);
			}

			/*    3.1个顶点在视锥内,2个顶点在视锥外,裁剪后还是1个三角面:

						p0
						/\
				  _____/__\_____ near clip plane
					  /____\
					 p1	   p2

													*/
			else if(nVertOut == 2)
			{
				//找出内外的顶点
				SVertex *p0, *p1, *p2; 
				if(flags[0] && flags[1])		{ p1 = &vert1; p2 = &vert2; p0 = &vert3; }
				else if(flags[0] && flags[2])	{ p1 = &vert1; p2 = &vert3; p0 = &vert2; }
				else							{ p1 = &vert2; p2 = &vert3; p0 = &vert1; }

				//直线参数化方程求t
				const VEC4 line1 = Common::Sub_Vec4_By_Vec4(p1->pos, p0->pos);
				float t1 = (-n - p0->pos.z)/(line1.z);
				float newX1 = p0->pos.x + line1.x * t1;
				float newY1 = p0->pos.y + line1.y * t1;
				//覆盖原来的p1
				p1->pos.x = newX1; p1->pos.y = newY1; p1->pos.z = -n;

				//另一个交点
				const VEC4 line2 = Common::Sub_Vec4_By_Vec4(p2->pos, p0->pos);
				float t2 = (-n - p0->pos.z)/(line2.z);
				float newX2 = p0->pos.x + line2.x * t2;
				float newY2 = p0->pos.y + line2.y * t2;
				//覆盖原来的p2
				p2->pos.x = newX2; p2->pos.y = newY2; p2->pos.z = -n;

				//计算新的uv
				p1->uv.x = p0->uv.x + (p1->uv.x - p0->uv.x) * t1;
				p1->uv.y = p0->uv.y + (p1->uv.y - p0->uv.y) * t1;

				p2->uv.x = p0->uv.x + (p2->uv.x - p0->uv.x) * t2;
				p2->uv.y = p0->uv.y + (p2->uv.y - p0->uv.y) * t2;
			}
		}
	}

	void Renderer::ToggleShadingMode()
	{
		switch (m_curRas->GetType())
		{
		case SR::eRasterizeType_Wireframe:			SetRasterizeType(SR::eRasterizeType_Flat); break;
		case SR::eRasterizeType_Flat:				SetRasterizeType(SR::eRasterizeType_Gouraud); break;
		case SR::eRasterizeType_Gouraud:			SetRasterizeType(SR::eRasterizeType_TexturedGouraud); break;
		case SR::eRasterizeType_TexturedGouraud:	SetRasterizeType(SR::eRasterizeType_BlinnPhong); break;
		case SR::eRasterizeType_BlinnPhong:			SetRasterizeType(SR::eRasterizeType_Wireframe); break;
		default: assert(0);
		}
	}

	void Renderer::AddMaterial( const STRING& name, const SMaterial* mat )
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

}