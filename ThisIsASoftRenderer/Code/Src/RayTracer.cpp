#include "stdafx.h"
#include "RayTracer.h"
#include "Camera.h"
#include "Renderer.h"
#include "Scene.h"


namespace SR
{
	//------------------------------------------------------------------------------------
	RayTracer::RayTracer()
	{
		m_pLight = new SPointLight;
		m_pLight->pos = VEC3(-300, 300, 300);
		m_pLight->color = SColor::WHITE;
	}
	//------------------------------------------------------------------------------------
	RayTracer::~RayTracer()
	{
		SAFE_DELETE(m_pLight);
	}
	//------------------------------------------------------------------------------------
	void RayTracer::RunIntersectUnitTest()
	{
		// Ray-triangle 
		{
			VEC3 intersectPt;
			VEC3 v0(-20, -15, -100);
			VEC3 v1(20, -15, -100);
			VEC3 v2(0, 15, -100);

			RAY ray;
			ray.m_origin = VEC3::ZERO;

			ray.m_dir = VEC3(0, 0, 100);
			ray.m_dir.Normalize();

			bool bIntersect = ray.Intersect_Triangle(intersectPt, v0, v1, v2);
			assert(!bIntersect);

			ray.m_dir = VEC3(0, 0, -100);
			ray.m_dir.Normalize();

			bIntersect = ray.Intersect_Triangle(intersectPt, v0, v1, v2);
			assert(bIntersect);

			ray.m_dir = VEC3(2, 17, 100);
			ray.m_dir.Normalize();

			bIntersect = ray.Intersect_Triangle(intersectPt, v0, v1, v2);
			assert(!bIntersect);

			ray.m_dir = v1;
			ray.m_dir.Normalize();

			bIntersect = ray.Intersect_Triangle(intersectPt, v0, v1, v2);
			assert(bIntersect && intersectPt==v1);
		}
	}
	//------------------------------------------------------------------------------------
	void RayTracer::ProcessScene( Scene* pScene )
	{
		PixelBox* pBackBuffer = g_env.renderer->GetFrameBuffer();
		DWORD* destBuffer = (DWORD*)pBackBuffer->GetDataPointer();

		// Primary ray
		RAY viewRay, worldRay;
		SIntersection intersection;
		
		for (int y=0; y<SCREEN_HEIGHT; ++y)
		{
			for (int x=0; x<SCREEN_WIDTH; ++x, ++destBuffer)
			{
				_GetRayFromScreenPt(worldRay, viewRay, x, y);

				// Get nearest intersect point
				if(_GetIntersection(intersection, pScene, worldRay, viewRay))
				{
					*destBuffer = _Shade(intersection).GetAsInt();
				}
			}
		}
	}
	//------------------------------------------------------------------------------------
	void RayTracer::_GetRayFromScreenPt( RAY& oWorldRay, RAY& oViewRay, int x, int y )
	{
		Camera* cam = &g_env.renderer->m_camera;

		// Screen pt -> NDC pt
		float fx = x / (float)SCREEN_WIDTH;
		float fy = -y / (float)SCREEN_HEIGHT;
		fx = (fx - 0.5f) * 2;
		fy = (fy + 0.5f) * 2;

		// Transform to view space
		VEC4 imagePlanePt;
		imagePlanePt = Common::Transform_Vec4_By_Mat44(VEC4(fx,fy,0,1), cam->GetInvProjMatrix());
		Common::Multiply_Vec4_By_K(imagePlanePt, imagePlanePt, 1 / imagePlanePt.w);
		imagePlanePt.z = -cam->GetNearClip();

		oViewRay.m_origin = VEC3::ZERO;
		oViewRay.m_dir = imagePlanePt.GetVec3();
		oViewRay.m_dir.Normalize();

		// Transform to world space
		oWorldRay.m_origin = cam->GetPos().GetVec3();
		oWorldRay.m_dir = Common::Transform_Vec3_By_Mat44(oViewRay.m_dir, cam->GetInvViewMatrix(), false).GetVec3();
		oWorldRay.m_dir.Normalize();
	}
	//------------------------------------------------------------------------------------
	bool RayTracer::_GetIntersection( SIntersection& oIntersection, Scene* pScene, const RAY& worldRay, const RAY& viewRay )
	{
		std::vector<SIntersection> vecIntersect;
		RenderList& objList = pScene->m_renderList_solid;

		for (size_t i=0; i<objList.size(); ++i)
		{
			RenderObject* obj = objList[i];

			VEC3 intersectPt;
			// First check AABB..
			if (!worldRay.Intersect_Box(intersectPt, obj->m_localAABB.m_minCorner, obj->m_localAABB.m_maxCorner))
				continue;

			// Gather it
			const VEC3& v0 = obj->m_verts[0].pos.GetVec3();
			const VEC3& v1 = obj->m_verts[1].pos.GetVec3();
			const VEC3& v2 = obj->m_verts[2].pos.GetVec3();

			if (worldRay.Intersect_Triangle(intersectPt, v0, v1, v2))
			{
				SIntersection curIntersect;
				curIntersect.pt = intersectPt;
				curIntersect.normal = obj->m_verts[0].normal;

				vecIntersect.push_back(curIntersect);
			}
		}

		// Get the one has nearest view space Z
		const VEC3& camPos = g_env.renderer->m_camera.GetPos().GetVec3();
		int idx = -1;
		float fMinDistSq = FLT_MAX;
		for (size_t i=0; i<vecIntersect.size(); ++i)
		{
			VEC3 v = Common::Sub_Vec3_By_Vec3(vecIntersect[i].pt, camPos);
			float fDistSq = Common::DotProduct_Vec3_By_Vec3(v, v);

			if (fDistSq < fMinDistSq)
			{
				fMinDistSq = fDistSq;
				idx = i;
				oIntersection = vecIntersect[i];
			}
		}

		return idx != -1;
	}
	//------------------------------------------------------------------------------------
	SColor RayTracer::_Shade( const SIntersection& intersection )
	{
		return SColor::WHITE;
	}
}