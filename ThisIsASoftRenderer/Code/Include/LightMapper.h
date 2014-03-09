/********************************************************************
	created:	6:3:2014   22:50
	filename	LightMapper.h
	author:		maval

	purpose:	Light map generator
				Note: All meshes must have own light map uv atlas!

				Reference : http://www.flipcode.com/archives/Light_Mapping_Theory_and_Implementation.shtml
*********************************************************************/
#ifndef LightMapper_h__
#define LightMapper_h__


#include "Prerequiestity.h"
#include "MathDef.h"
#include "Color.h"
#include "RenderObject.h"

namespace SR
{
	class LightMapper
	{
	public:
		LightMapper();
		~LightMapper();

		SPointLight*	m_pLight;

	public:
		// For now, the whole scene is one single mesh, so produce only one light map.
		STexture*	GenerateLightMap(int mapWidth, int mapHeight, RenderObject* pMeshToGen, const RenderList& sceneMeshes);

	private:
		VEC3		_GetPosFromFace(const SVertex& vert0, const SVertex& vert1, const SVertex& vert2, 
			const VEC3& normal, const VEC2& uv);
		SColor		_CalcLumel(const RenderList& sceneMeshes, const VEC3& pos, const VEC3& normal);
	};
}


#endif // LightMapper_h__