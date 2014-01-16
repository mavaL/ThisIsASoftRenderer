#include "stdafx.h"
#include "Scene.h"
#include "RenderUtil.h"
#include "Renderer.h"

namespace SR
{
	void Scene::AddRenderObject( RenderObject* obj )
	{
		RenderUtil::ComputeAABB(*obj);

		// If object is static in scene, we can do some pre-calcs
		// instead of doing them at run-time.
		if(obj->m_bStatic)
		{
			obj->m_worldAABB = obj->m_localAABB;
			obj->m_worldAABB.Transform(obj->m_matWorld);
		}

		if (obj->m_pMaterial->pDiffuseMap && obj->m_pMaterial->pDiffuseMap->bMipMap)
		{
			obj->CalcAllFaceTexArea();
		}
		
		if(obj->m_pMaterial->bTransparent)
			m_renderList_trans.push_back(obj);
		else
			m_renderList_solid.push_back(obj);
	}

	Scene::~Scene()
	{
		std::for_each(m_renderList_solid.begin(), m_renderList_solid.end(), std::default_delete<RenderObject>());
		std::for_each(m_renderList_trans.begin(), m_renderList_trans.end(), std::default_delete<RenderObject>());
		m_renderList_solid.clear();
		m_renderList_trans.clear();
	}

	void Scene::Enter()
	{
		if(!m_bSetup)
		{
			m_setupFunc(this);
			m_bSetup = true;
		}
		m_enterFunc(this);

		g_env.renderer->m_camera._BuildViewMatrix();
		g_env.renderer->m_camera._BuildProjMatrix();
	}
}


