// ThisIsASoftRenderer.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "../../Resource.h"
#include "Renderer.h"
#include "OgreMeshLoader.h"
#include "ObjMeshLoader.h"
#include "Utility.h"

#define MAX_LOADSTRING 100
const int		SCREEN_WIDTH	=	800;
const int		SCREEN_HEIGHT	=	600;

inline STRING	GetResPath(const STRING filename)
{
	STRING filepath("../../../Res/");
	filepath += filename;
	return std::move(filepath);
}

// 全局变量:
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

ULONG_PTR			g_gdiplusToken;
SR::Renderer		g_renderer;					// 软渲染器
Ext::OgreMeshLoader	g_meshLoader;				// .mesh加载器
Ext::ObjMeshLoader	g_objLoader;				// .obj加载器

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
		//////////////////////////////////////////////////////////
		/////////////// main game processing goes here
		g_renderer.RenderOneFrame();

		//显示一些辅助信息
		{
			const VEC4& pos = g_renderer.m_camera.GetPos();
			char szText[256];
			sprintf_s(szText, ARRAYSIZE(szText), "lastFPS : %d, CamPos : (%f, %f, %f)", g_renderer.GetLastFPS(), pos.x, pos.y, pos.z);

			auto& renderList = g_renderer.GetRenderList();
			size_t nCulled = std::count_if(renderList.begin(), renderList.end(), [&](const SR::SRenderObj& obj){ return obj.m_bCull; });

			size_t nBackface = 0;
			for (size_t iObj=0; iObj<renderList.size(); ++iObj)
			{
				const SR::FaceList& faces = renderList[iObj].faces;
				for (size_t iFace=0; iFace<faces.size(); ++iFace)
				{
					if(faces[iFace].IsBackface) ++nBackface;
				}
			}

			char szText2[128];
			sprintf_s(szText2, ARRAYSIZE(szText2), "      Culled Object : %d, Culled Backface : %d", nCulled, nBackface);
			strcat_s(szText, sizeof(szText), szText2);

			SR::RenderUtil::DrawText(10, 10, szText, RGB(0,255,0));
		}

		{
			const float speed = g_renderer.m_camera.GetMoveSpeed();
			char szText[256];
			sprintf_s(szText, ARRAYSIZE(szText), 
				"Camera Speed: %f . Press \"+/-\" to increase/decrease camera speed. Press R to toggle shade mode!", speed);

			SR::RenderUtil::DrawText(10, 35, szText, RGB(0,255,0));
		}

		//swap!!
		g_renderer.Present();
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

   hInst = hInstance; // 将实例句柄存储在全局变量中

   DWORD style = WS_SYSMENU | WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;
   hWnd = CreateWindow(szWindowClass, szTitle, style,
      CW_USEDEFAULT, 0, SCREEN_WIDTH, SCREEN_HEIGHT, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   /////////////////////////////////////////////////////////
   /////////////// Init here
   Gdiplus::GdiplusStartupInput gdiplusStartupInput;
   GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

   g_renderer.Init();
   g_renderer.m_hwnd = hWnd;
   g_renderer.SetRasterizeType(SR::eRasterizeType_Flat);

   //// Test case 1: Simple one triangle
   {
// 	   SR::SRenderObj obj;
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
// 	   obj.VB.push_back(v1);
// 	   obj.VB.push_back(v2);
// 	   obj.VB.push_back(v3);
// 
// 	   SR::SFace face(0,1,2);
// 	   face.faceNormal = VEC3::UNIT_Z;
// 	   obj.faces.push_back(face);
// 
// 	   obj.boundingRadius = SR::RenderUtil::ComputeBoundingRadius(obj.VB);
// 	   obj.texture.LoadTexture(GetResPath("marine_diffuse_blood.bmp"));
// 
// 	   obj.matWorldIT = obj.matWorld.Inverse();
// 	   obj.matWorldIT = obj.matWorldIT.Transpose();
// 
// 	   g_renderer.AddRenderable(obj);
   }

   //// Test case 2: marine.mesh
   {
// 	   try
// 	   {
// 		   g_meshLoader.LoadMeshFile(GetResPath("marine.mesh.xml"));
// 		   g_meshLoader.m_obj.texture.LoadTexture(GetResPath("marine_diffuse_blood.bmp"));
// 	   }
// 	   catch (std::exception& e)
// 	   {
// 		   MessageBoxA(hWnd, e.what(), "Error", MB_ICONERROR);
// 		   return FALSE;
// 	   }
// 
// 	   g_renderer.AddRenderable(g_meshLoader.m_obj);
   }
  
   //// Test case 3: sponza.obj
   {
	   if(!g_objLoader.LoadMeshFile(GetResPath("Sponza\\sponza.obj")))
	   {
		   MessageBoxA(hWnd, "Reading .obj failed!", "Error", MB_ICONERROR);
		   return FALSE;
	   }
	   g_renderer.AddRenderObjs(g_objLoader.m_objs);
   }

   g_renderer.m_camera.SetPosition(VEC3(0,0,200));

   //居中鼠标
   RECT rcClient;
   GetClientRect(hWnd, &rcClient);
   int w = rcClient.right - rcClient.left;
   int h = rcClient.bottom - rcClient.top;
   SetCursorPos(rcClient.left+w/2, rcClient.top+h/2);
   ShowWindow(hWnd, SW_SHOWNORMAL);
   UpdateWindow(hWnd);

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
					g_renderer.m_camera.AddMoveSpeed(1.0f);
					return 0;
				}
				break;
			case VK_SUBTRACT:
				{
					g_renderer.m_camera.AddMoveSpeed(-0.2f);
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
				g_renderer.ToggleShadingMode();
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
