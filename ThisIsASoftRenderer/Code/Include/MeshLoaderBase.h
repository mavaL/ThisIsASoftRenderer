/********************************************************************
	created:	2013/07/22
	created:	22:7:2013   17:16
	filename: 	MeshLoaderBase.h
	author:		maval
	
	purpose:	模型加载器基类
*********************************************************************/
#ifndef MeshLoaderBase_h__
#define MeshLoaderBase_h__

#include "Prerequiestity.h"
#include "RenderObject.h"

namespace Ext
{
	class MeshLoader
	{
	public:
		virtual ~MeshLoader() {}

		/** param 
			bStatic: 要加载的模型是否在场景中是静态的(即世界矩阵始终为单位矩阵),这样就一次性计算出世界包围盒,不用以后每帧更新
			bFlipUV: 是否翻转纹理坐标V,用于.bmp纹理
		**/
		bool	LoadMeshFile(const STRING& filename, bool bStatic, bool bFlipUV = false);

		SR::RenderList	m_objs;

	protected:
		virtual bool LoadImpl(const STRING& filename, bool bFlipUV) = 0;
	};
}

#endif // MeshLoaderBase_h__