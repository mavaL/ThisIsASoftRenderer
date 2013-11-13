/********************************************************************
	created:	2013/07/23
	created:	23:7:2013   10:23
	filename: 	Prerequiestity.h
	author:		maval
	
	purpose:	项目前提文件
*********************************************************************/
#ifndef Prerequiestity_h__
#define Prerequiestity_h__

const int		SCREEN_WIDTH	=	640;
const int		SCREEN_HEIGHT	=	480;
const int		PIXEL_MODE		=	4;		//像素模式(字节数). NB:只支持32位模式,不要更改
// clipping rectangle 
const int		min_clip_x		=	0;                          
const int		max_clip_x		=	(SCREEN_WIDTH-1);
const int		min_clip_y		=	0;
const int		max_clip_y		=	(SCREEN_HEIGHT-1);

#define USE_32BIT_INDEX			0			//是否使用32位顶点索引
#define USE_PERSPEC_CORRECT		1			//是否使用透视修正
#define USE_PROFILER			1			//是否使用Profiler
#define USE_MULTI_THREAD		0			//是否使用多线程
#define USE_SIMD				1			//是否使用SIMD


typedef std::string STRING;

namespace Common
{
	class Vector2;
	class Vector3;
	class Vector4;
	class Matrix44;
	class AxisAlignBBox;
}

typedef Common::Vector2		VEC2;
typedef Common::Vector3		VEC3;
typedef Common::Vector4		VEC4;
typedef Common::Matrix44	MAT44;
typedef Common::AxisAlignBBox	AABB;

namespace SR
{
	class Renderer;
	class PixelBox;
	class Scene;
	class Camera;
	class RenderObject;
	class Rasterizer;
	class RasWireFrame;
	class RasFlat;
	class RasGouraud;
	class RasTexturedGouraud;
	struct SRenderContext;
	struct SScanLinesData;
	struct SScanLine;
	struct SVertex;
	struct SFragment;
	struct STexture;
	struct SMaterial;
	struct MyJobParam;
	template<class T> class CFThreadPool;
}

namespace Ext
{
	class MeshLoader;
	class OgreMeshLoader;
	class ObjMeshLoader;
	class Profiler;
}

//整个项目直接取用的全局变量
struct SGlobalEnv 
{
	HWND					hwnd;					// 主窗口句柄
	SR::Renderer*			renderer;				// 渲染器
	Ext::OgreMeshLoader*	meshLoader;				// .mesh加载器
	Ext::ObjMeshLoader*		objLoader;				// .obj加载器
	Ext::Profiler*			profiler;				// 性能诊断器
	SR::CFThreadPool<void*>* jobMgr;	// 线程池	
};
extern SGlobalEnv	g_env;

#endif // Prerequiestity_h__