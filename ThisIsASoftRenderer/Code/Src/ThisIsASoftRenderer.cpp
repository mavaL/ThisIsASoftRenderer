// ThisIsASoftRenderer.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "../../Resource.h"
#include "Renderer.h"
#include "OgreMeshLoader.h"
#include "ObjMeshLoader.h"
#include "Utility.h"
#include "RenderObject.h"

#define MAX_LOADSTRING 100
const int		SCREEN_WIDTH	=	800;
const int		SCREEN_HEIGHT	=	600;

// 全局变量:
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

ULONG_PTR			g_gdiplusToken;
SR::Renderer		g_renderer;			
Ext::OgreMeshLoader	g_meshLoader;		
Ext::ObjMeshLoader	g_objLoader;
SGlobalEnv			g_env;

// 此代码模块中包含的函数的前向声明:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: 在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_THISISASOFTRENDERER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_THISISASOFTRENDERER));

	// 主消息循环:
	while (1)
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{ 
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} 
		else
		{
			// Render a frame during idle time (no messages are waiting)

			//////////////////////////////////////////////////////////
			/////////////// main game processing goes here
			g_renderer.OnFrameMove();
			g_renderer.RenderOneFrame();

			//显示一些辅助信息
			{
				const VEC4& pos = g_renderer.m_camera.GetPos();
				char szText[128];
				sprintf_s(szText, ARRAYSIZE(szText), "lastFPS : %d, CamPos : (%f, %f, %f)", 
					g_renderer.m_frameStatics.lastFPS, pos.x, pos.y, pos.z);

				SR::RenderUtil::DrawText(10, 10, szText, RGB(0,255,0));
			}

			{
				char szText[128];
				sprintf_s(szText, ARRAYSIZE(szText), "RenderedTris : %d, Culled Object : %d, Backface : %d, Culled Face : %d", 
					g_renderer.m_frameStatics.nRenderedFace, g_renderer.m_frameStatics.nObjCulled, g_renderer.m_frameStatics.nBackFace, g_renderer.m_frameStatics.nFaceCulled);

				SR::RenderUtil::DrawText(10, 35, szText, RGB(0,255,0));
			}

			{
				const float speed = g_renderer.m_camera.GetMoveSpeed();
				char szText[128];
				sprintf_s(szText, ARRAYSIZE(szText), 
					"Camera Speed: %f . Press \"+/-\" to increase/decrease camera speed. Press R to toggle shade mode!", speed);

				SR::RenderUtil::DrawText(10, 60, szText, RGB(0,255,0));
			}

			//swap!!
			g_renderer.Present();
		}
	}

	//GdiplusShutdown(g_gdiplusToken);

	return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
//  注释:
//
//    仅当希望
//    此代码与添加到 Windows 95 中的“RegisterClassEx”
//    函数之前的 Win32 系统兼容时，才需要此函数及其用法。调用此函数十分重要，
//    这样应用程序就可以获得关联的
//    “格式正确的”小图标。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THISISASOFTRENDERER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(GetStockObject(BLACK_BRUSH));
	wcex.lpszMenuName	= /*MAKEINTRESOURCE(IDC_THISISASOFTRENDERER)*/nullptr;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
   hInst = hInstance;

   //调整真正渲染窗口大小
   RECT rcClient;
   rcClient.top = 0;
   rcClient.left = 0;
   rcClient.right = SCREEN_WIDTH;
   rcClient.bottom = SCREEN_HEIGHT;

   DWORD style = WS_SYSMENU | WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

   AdjustWindowRect(&rcClient, style, FALSE);

   const int realWidth = rcClient.right - rcClient.left;
   const int realHeight = rcClient.bottom - rcClient.top;

   int windowLeft = (GetSystemMetrics(SM_CXSCREEN) - realWidth) / 2;
   int windowTop = (GetSystemMetrics(SM_CYSCREEN) - realHeight) / 2;
 
   hWnd = CreateWindow(szWindowClass, szTitle, style,
      windowLeft, windowTop, realWidth, realHeight, NULL, NULL, hInstance, NULL);

   if (!hWnd)
      return FALSE;

   ShowWindow(hWnd, SW_SHOWNORMAL);
   UpdateWindow(hWnd);

   float a = 3.3f;
   BYTE b = static_cast<BYTE>(a);

   /////////////////////////////////////////////////////////
   /////////////// Init here
   Gdiplus::GdiplusStartupInput gdiplusStartupInput;
   GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

   //init globals
   g_env.hwnd = hWnd;
   g_env.renderer = &g_renderer;
   g_env.meshLoader = &g_meshLoader;
   g_env.objLoader = &g_objLoader;

   //init renderer
   g_env.renderer->Init();
   g_env.renderer->SetRasterizeType(SR::eRasterizeType_TexturedGouraud);

   //// Test case 1: Simple one triangle
   {
// 	   SR::RenderObject obj;
// 
// 	   SR::SVertex v1, v2, v3;
// 	   v1.pos = VEC4(-20, -15, 0, 1);
// 	   v2.pos = VEC4(20, -15, 0, 1);
// 	   v3.pos = VEC4(0, 15, 0, 1);
// 
// 	   v1.normal = VEC3::UNIT_Z;
// 	   v2.normal = VEC3::UNIT_Z;
// 	   v3.normal = VEC3::UNIT_Z;
// 
// 	   v1.uv = VEC2(0.0f, 1.0f);
// 	   v2.uv = VEC2(1.0f, 1.0f);
// 	   v3.uv = VEC2(0.5f, 0.0f);
// 
// 	   obj.m_verts.push_back(v1);
// 	   obj.m_verts.push_back(v2);
// 	   obj.m_verts.push_back(v3);
// 
// 	   SR::SFace face(0,1,2);
// 	   face.faceNormal = VEC3::UNIT_Z; 
// 	   obj.m_faces.push_back(face);
// 	  
// 	   SR::SMaterial* mat = new SR::SMaterial;
// 	   mat->pTexture = new SR::STexture;
// 	   mat->pTexture->LoadTexture(GetResPath("marine_diffuse_blood.bmp"));
// 	   g_renderer.AddMaterial("MatMarine", mat);
// 	   obj.m_pMaterial = mat;
// 
// 	   SR::RenderUtil::ComputeAABB(obj);
// 
// 	   g_env.renderer->AddRenderable(obj);
   }

   //// Test case 2: 透视校正纹理映射
   {
	   SR::RenderObject obj;

	   SR::SVertex v1, v2, v3 ,v4;
	   v1.pos = VEC4(-500, 0, -500, 1);
	   v2.pos = VEC4(500, 0, -500, 1);
	   v3.pos = VEC4(-500, 0, 500, 1);
	   v4.pos = VEC4(500, 0, 500, 1);

	   v1.normal = VEC3::UNIT_Y;
	   v2.normal = VEC3::UNIT_Y;
	   v3.normal = VEC3::UNIT_Y;
	   v4.normal = VEC3::UNIT_Y;

	   v1.uv = VEC2(0, 0);
	   v2.uv = VEC2(10, 0);
	   v3.uv = VEC2(0, 10);
	   v4.uv = VEC2(10, 10);

	   obj.m_verts.push_back(v1);
	   obj.m_verts.push_back(v2);
	   obj.m_verts.push_back(v3);
	   obj.m_verts.push_back(v4);

	   SR::SFace face1(0,2,1);
	   SR::SFace face2(1,2,3);
	   face1.faceNormal = VEC3::UNIT_Y; 
	   face2.faceNormal = VEC3::UNIT_Y; 
	   obj.m_faces.push_back(face1);
	   obj.m_faces.push_back(face2);

	   SR::SMaterial* mat = new SR::SMaterial;
	   mat->pTexture = new SR::STexture;
	   mat->pTexture->LoadTexture(GetResPath("ChesePanel.bmp"));
	   g_renderer.AddMaterial("Chese", mat);
	   obj.m_pMaterial = mat;

	   SR::RenderUtil::ComputeAABB(obj);

	   g_renderer.AddRenderable(obj);

	   g_renderer.m_camera.SetFarClip(10000);
	   g_renderer.m_camera.SetPosition(VEC3(-20,706,1706));
   }

   //// Test case 3: marine.mesh
   {
// 	   try
// 	   {
// 		   if(!g_meshLoader.LoadMeshFile(GetResPath("marine.mesh.xml"), true))
// 			   throw std::logic_error("Error, Load .mesh file failed!");
// 
// 		   SR::SMaterial* mat = new SR::SMaterial;
// 		   mat->pTexture = new SR::STexture;
// 		   mat->pTexture->LoadTexture(GetResPath("marine_diffuse_blood.bmp"));
// 		   g_renderer.AddMaterial("MatMarine", mat);
// 		   g_meshLoader.m_objs[0].m_pMaterial = mat;
// 	   }
// 	   catch (std::exception& e)
// 	   {
// 		   MessageBoxA(hWnd, e.what(), "Error", MB_ICONERROR);
// 		   return FALSE;
// 	   }
// 
// 	   g_env.renderer->AddRenderObjs(g_meshLoader.m_objs);
   }
  
   //// Test case 4: sponza.obj
   {
// 	   try
// 	   {
// 		   if(!g_objLoader.LoadMeshFile(GetResPath("Sponza\\sponza.obj"), true))
// 			   throw std::logic_error("Error, Load .obj file failed!");
// 	   }
// 	   catch (std::exception& e)
// 	   {
// 		   MessageBoxA(hWnd, e.what(), "Error", MB_ICONERROR);
// 		   return FALSE;
// 	   }
// 
// 	   g_env.renderer->AddRenderObjs(g_objLoader.m_objs);
// 	   g_env.renderer->m_camera.SetPosition(VEC3(-1.8f, 6.6f, -4.7f));
   }

   

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
#include <WinUser.h>
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_PAINT:
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_ERASEBKGND:
		return 0;

	case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_ADD:
				{
					g_env.renderer->m_camera.AddMoveSpeed(1.0f);
					return 0;
				}
				break;
			case VK_SUBTRACT:
				{
					g_env.renderer->m_camera.AddMoveSpeed(-0.2f);
					return 0;
				}
				break;
			case VK_ESCAPE:
				{
					DestroyWindow(hWnd);	
					return 0;
				}
			default: return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;

	case WM_CHAR:
		{
			if(wParam == 'r')
			{
				g_env.renderer->ToggleShadingMode();
				return 0;
			}
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
