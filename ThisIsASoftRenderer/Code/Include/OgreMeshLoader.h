/********************************************************************
	created:	7:7:2013   22:47
	filename	OgreMeshLoader.h
	author:		maval

	purpose:	Ogre mesh¼ÓÔØÆ÷
*********************************************************************/
#ifndef OgreMeshLoader_h__
#define OgreMeshLoader_h__

#include "MeshLoaderBase.h"

namespace Ext
{
	class OgreMeshLoader : public MeshLoader
	{
	protected:
		virtual bool LoadImpl(const STRING& filename, bool bFlipUV);
	};
}

#endif // OgreMeshLoader_h__