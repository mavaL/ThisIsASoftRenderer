/********************************************************************
	created:	2013/07/22
	created:	22:7:2013   17:16
	filename: 	MeshLoaderBase.h
	author:		maval
	
	purpose:	模型加载器基类
*********************************************************************/
#ifndef MeshLoaderBase_h__
#define MeshLoaderBase_h__

#include "GeometryDef.h"

namespace Ext
{
	class MeshLoader
	{
	public:
		virtual ~MeshLoader() {}
		bool	LoadMeshFile(const STRING& filename);

		SR::RenderList	m_objs;

	protected:
		virtual bool LoadImpl(const STRING& filename) = 0;
	};
}

#endif // MeshLoaderBase_h__