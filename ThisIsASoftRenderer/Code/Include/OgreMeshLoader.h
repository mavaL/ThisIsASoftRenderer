/********************************************************************
	created:	7:7:2013   22:47
	filename	OgreMeshLoader.h
	author:		maval

	purpose:	Ogre mesh¼ÓÔØÆ÷
*********************************************************************/
#ifndef OgreMeshLoader_h__
#define OgreMeshLoader_h__

#include "GeometryDef.h"

namespace Ext
{
	class OgreMeshLoader
	{
	public:
		bool	LoadMeshFile(const STRING& filename);

		SR::SRenderObj	m_obj;
	};
}

#endif // OgreMeshLoader_h__