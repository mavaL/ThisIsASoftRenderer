#include "stdafx.h"
#include "Renderer.h"
#include "Scene.h"
#include "OgreMeshLoader.h"
#include "ObjMeshLoader.h"


#define ADD_TEST_SCENE($setupFunc, $enterFunc)			\
{														\
	Scene::SetupFunc setupFunc = $setupFunc;			\
	Scene::EnterFunc enterFunc = $enterFunc;			\
	Scene* pScene = new Scene(setupFunc, enterFunc);	\
	m_scenes.push_back(pScene);							\
}											

namespace SR
{
	void SetupTestScene1(Scene* scene)
	{
		SR::RenderObject obj;

		SR::SVertex v1, v2, v3;
		v1.pos = VEC4(-20, -15, 0, 1);
		v2.pos = VEC4(20, -15, 0, 1);
		v3.pos = VEC4(0, 15, 0, 1);

		v1.normal = VEC3::UNIT_Z;
		v2.normal = VEC3::UNIT_Z;
		v3.normal = VEC3::UNIT_Z;

		v1.uv = VEC2(0.0f, 1.0f);
		v2.uv = VEC2(1.0f, 1.0f);
		v3.uv = VEC2(0.5f, 0.0f);

		obj.m_verts.push_back(v1);
		obj.m_verts.push_back(v2);
		obj.m_verts.push_back(v3);

		SR::SFace face(0,1,2);
		face.faceNormal = VEC3::UNIT_Z; 
		obj.m_faces.push_back(face);

		SR::SMaterial* mat = new SR::SMaterial;
		g_env.renderer->AddMaterial("MatTriangle", mat);
		obj.m_pMaterial = mat;

		SR::RenderUtil::ComputeAABB(obj);

		scene->m_renderList.push_back(obj);
	}

	void EnterTestScene1()
	{
		g_env.renderer->m_camera.SetPosition(VEC3(0,0,200));
		g_env.renderer->m_camera.SetMoveSpeed(3.0f);
		g_env.renderer->m_camera.SetDirection(VEC3::NEG_UNIT_Z);
		g_env.renderer->SetRasterizeType(SR::eRasterizeType_Flat);
	}

	void SetupTestScene2(Scene* scene)
	{
		SR::RenderObject obj;

		SR::SVertex v1, v2, v3 ,v4;
		v1.pos = VEC4(-500, 0, -500, 1);
		v2.pos = VEC4(500, 0, -500, 1);
		v3.pos = VEC4(-500, 0, 500, 1);
		v4.pos = VEC4(500, 0, 500, 1);

		v1.normal = VEC3::UNIT_Y;
		v2.normal = VEC3::UNIT_Y;
		v3.normal = VEC3::UNIT_Y;
		v4.normal = VEC3::UNIT_Y;

		v1.uv = VEC2(0, 0);
		v2.uv = VEC2(10, 0);
		v3.uv = VEC2(0, 10);
		v4.uv = VEC2(10, 10);

		obj.m_verts.push_back(v1);
		obj.m_verts.push_back(v2);
		obj.m_verts.push_back(v3);
		obj.m_verts.push_back(v4);

		SR::SFace face1(0,2,1);
		SR::SFace face2(1,2,3);
		face1.faceNormal = VEC3::UNIT_Y; 
		face2.faceNormal = VEC3::UNIT_Y; 
		obj.m_faces.push_back(face1);
		obj.m_faces.push_back(face2);

		SR::SMaterial* mat = new SR::SMaterial;
		mat->pDiffuseMap = new SR::STexture;
		mat->pDiffuseMap->LoadTexture(GetResPath("ChesePanel.bmp"));
		g_env.renderer->AddMaterial("Chese", mat);
		obj.m_pMaterial = mat;

		SR::RenderUtil::ComputeAABB(obj);

		scene->m_renderList.push_back(obj);
	}

	void EnterTestScene2()
	{
		g_env.renderer->m_camera.SetFarClip(10000);
		g_env.renderer->m_camera.SetPosition(VEC3(-20,706,1706));
		g_env.renderer->m_camera.SetMoveSpeed(5.0f);
		g_env.renderer->m_camera.SetDirection(VEC3(0,-1,-2));
		g_env.renderer->SetRasterizeType(eRasterizeType_TexturedGouraud);
	}

	void SetupTestScene3(Scene* scene)
	{
		try
		{
			if(!g_env.meshLoader->LoadMeshFile(GetResPath("marine.mesh.xml"), true))
				throw std::logic_error("Error, Load .mesh file failed!");

			SR::SMaterial* mat = new SR::SMaterial;
			mat->pDiffuseMap = new SR::STexture;
			mat->pDiffuseMap->LoadTexture(GetResPath("marine_diffuse_blood.bmp"));
			mat->bUseHalfLambert = true;
			mat->bUseBilinearSampler = true;
			mat->ambient.Set(0.3f, 0.3f, 0.3f);
			mat->diffuse.Set(0.6f, 0.6f, 0.6f);
			mat->specular.Set(0.3f, 0.3f, 0.3f);
			g_env.renderer->AddMaterial("MatMarine", mat);
			g_env.meshLoader->m_objs[0].m_pMaterial = mat;
		}
		catch (std::exception& e)
		{
			MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR);
			return;
		}

		scene->m_renderList.push_back(g_env.meshLoader->m_objs[0]);
	}

	void EnterTestScene3()
	{
		g_env.renderer->m_camera.SetPosition(VEC3(0,0,10));
		g_env.renderer->m_camera.SetMoveSpeed(0.1f);
		g_env.renderer->m_camera.SetDirection(VEC3::NEG_UNIT_Z);
		g_env.renderer->SetRasterizeType(eRasterizeType_TexturedGouraud);
	}

	void SetupTestScene4(Scene* scene)
	{
		try
		{
			if(!g_env.meshLoader->LoadMeshFile(GetResPath("teapot.mesh.xml"), true))
				throw std::logic_error("Error, Load .mesh file failed!");

			SR::SMaterial* mat = new SR::SMaterial;
			mat->ambient.Set(0.3f, 0.3f, 0.3f);
			mat->diffuse.Set(0.5f, 0.5f, 0.5f);
			mat->specular.Set(0.3f, 0.3f, 0.3f);
			mat->shiness = 50;
			g_env.renderer->AddMaterial("MatTeapot", mat);
			g_env.meshLoader->m_objs[0].m_pMaterial = mat;
		}
		catch (std::exception& e)
		{
			MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR);
			return;
		}

		scene->m_renderList.push_back(g_env.meshLoader->m_objs[0]);
	}

	void EnterTestScene4()
	{
		g_env.renderer->m_camera.SetPosition(VEC3(0,0,200));
		g_env.renderer->m_camera.SetMoveSpeed(1.0f);
		g_env.renderer->m_camera.SetDirection(VEC3::NEG_UNIT_Z);
		g_env.renderer->SetRasterizeType(eRasterizeType_BlinnPhong);
	}

	void SetupTestScene5(Scene* scene)
	{
		try
		{
			if(!g_env.objLoader->LoadMeshFile(GetResPath("Sponza\\sponza.obj"), true))
				throw std::logic_error("Error, Load .obj file failed!");
		}
		catch (std::exception& e)
		{
			MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR);
			return;
		}

		scene->m_renderList.insert(scene->m_renderList.end(), g_env.objLoader->m_objs.begin(), g_env.objLoader->m_objs.end());

		std::for_each(scene->m_renderList.begin(), scene->m_renderList.end(), [&](SR::RenderObject& obj)
		{
			obj.m_pMaterial->bUseBilinearSampler = true;
		});
	}

	void EnterTestScene5()
	{
		g_env.renderer->m_camera.SetPosition(VEC3(-1.8f, 6.6f, -4.7f));
		g_env.renderer->m_camera.SetMoveSpeed(2.0f);
		g_env.renderer->m_camera.SetDirection(VEC3::NEG_UNIT_Z);
		g_env.renderer->SetRasterizeType(eRasterizeType_BlinnPhong);
	}

	void Renderer::_InitAllScene()
	{
		//// Test Scene 1: One simple triangle
		ADD_TEST_SCENE(SetupTestScene1, EnterTestScene1);

		//// Test Scene 2: 透视校正纹理映射
		ADD_TEST_SCENE(SetupTestScene2, EnterTestScene2);

		//// Test Scene 3: marine.mesh
		ADD_TEST_SCENE(SetupTestScene3, EnterTestScene3);

		//// Test Scene 4: teapot.mesh + Phong模型
		ADD_TEST_SCENE(SetupTestScene4, EnterTestScene4);

		//// Test Scene 5: sponza.obj
		ADD_TEST_SCENE(SetupTestScene5, EnterTestScene5);
	}
}

