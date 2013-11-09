// ThisIsASoftRenderer.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "Prerequiestity.h"
#include "../../Resource.h"
#include "Renderer.h"
#include "OgreMeshLoader.h"
#include "ObjMeshLoader.h"
#include "Utility.h"
#include "Profiler.h"
#include "ThreadPool/ftlFake.h"
#include "ThreadPool/ftlThreadPool.h"

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

ULONG_PTR			g_gdiplusToken;
SR::Renderer		g_renderer;			
Ext::OgreMeshLoader	g_meshLoader;		
Ext::ObjMeshLoader	g_objLoader;
Ext::Profiler		g_profiler;
SR::CFThreadPool<void*>	g_jobMgr;
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

	DWORD d1, d2;
	BOOL bb;
	GetSystemTimeAdjustment(&d1, &d2, &bb);

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
			g_profiler.DisplayHelpInfo();
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

   /////////////////////////////////////////////////////////
   /////////////// Init here
   Gdiplus::GdiplusStartupInput gdiplusStartupInput;
   GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

   //init globals
   g_env.hwnd		= hWnd;
   g_env.renderer	= &g_renderer;
   g_env.meshLoader = &g_meshLoader;
   g_env.objLoader	= &g_objLoader;
   g_env.profiler	= &g_profiler;
   g_env.jobMgr		= &g_jobMgr;

   g_env.renderer->Init();

#if USE_MULTI_THREAD == 1
   DWORD nThreadCount = Ext::GetLogicalCpuCount() - 1;
   g_env.jobMgr->Start(nThreadCount, nThreadCount);
#endif

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

	case WM_LBUTTONDOWN:
		g_env.renderer->m_camera.m_bActive = true;
		return 0;

	case WM_LBUTTONUP:
		g_env.renderer->m_camera.m_bActive = false;
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
			if(wParam == 't')
			{
				g_env.renderer->ToggleScene();
				return 0;
			}
		}
		break;

	case WM_DESTROY:
		{
			g_jobMgr.Stop();
			PostQuitMessage(0);
		}
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
