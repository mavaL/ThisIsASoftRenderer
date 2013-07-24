/********************************************************************
	created:	2013/07/23
	created:	23:7:2013   10:23
	filename: 	Prerequiestity.h
	author:		maval
	
	purpose:	项目前提文件
*********************************************************************/
#ifndef Prerequiestity_h__
#define Prerequiestity_h__

typedef std::string STRING;

namespace Common
{
	class Vector2;
	class Vector3;
	class Vector4;
	class Matrix44;
	class PixelBox;
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
	class Camera;
	class RenderObject;
	class Rasterizer;
	class RasWireFrame;
	class RasFlat;
	class RasGouraud;
	class RasTexturedGouraud;
}

namespace Ext
{
	class MeshLoader;
	class OgreMeshLoader;
	class ObjMeshLoader;
}

//整个项目直接取用的全局变量
struct SGlobalEnv 
{
	HWND					hwnd;					// 主窗口句柄
	SR::Renderer*			renderer;				// 渲染器
	Ext::OgreMeshLoader*	meshLoader;				// .mesh加载器
	Ext::ObjMeshLoader*		objLoader;				// .obj加载器
};
extern SGlobalEnv	g_env;

#endif // Prerequiestity_h__