// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include <omp.h>

#include "../../targetver.h"

#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息
// Windows 头文件:
//#include <windows.h>

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展


// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <cassert>

//STL
#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <unordered_map>
#include <algorithm>
#include <functional>

//tinyxml
#include <tinyxml/tinystr.h>
#include <tinyxml/tinyxml.h>


#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")


