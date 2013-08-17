#include "stdafx.h"
#include "Scene.h"
#include "RenderUtil.h"

namespace SR
{
	void Scene::AddRenderObject( RenderObject* obj )
	{
		RenderUtil::ComputeAABB(*obj);

		if(obj->m_bStatic)
		{
			obj->m_worldAABB = obj->m_localAABB;
			obj->m_worldAABB.Transform(obj->m_matWorld);
		}

		m_renderList.push_back(obj);
	}

	Scene::~Scene()
	{
		std::for_each(m_renderList.begin(), m_renderList.end(), std::default_delete<RenderObject>());
		m_renderList.clear();
	}

}


