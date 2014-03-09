#include "stdafx.h"
#include "RayTracer.h"
#include "Camera.h"
#include "Renderer.h"
#include "Scene.h"
#include "Intersection.h"


namespace SR
{
	//------------------------------------------------------------------------------------
	RayTracer::RayTracer()
	{
		m_pLight = new SPointLight;
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

			std::pair<bool, float> hitResult = ray.Intersect_Triangle(v0, v1, v2);
			assert(!hitResult.first);

			ray.m_dir = VEC3(0, 0, -100);
			ray.m_dir.Normalize();

			hitResult = ray.Intersect_Triangle(v0, v1, v2);
			assert(hitResult.first);

			ray.m_dir = VEC3(2, 17, 100);
			ray.m_dir.Normalize();

			hitResult = ray.Intersect_Triangle(v0, v1, v2);
			assert(!hitResult.first);

			ray.m_dir = v1;
			ray.m_dir.Normalize();

			hitResult = ray.Intersect_Triangle(v0, v1, v2);
			intersectPt = ray.GetPoint(hitResult.second);
			assert(hitResult.first && intersectPt==v1);
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
					// Emit shadow ray
					RAY shadowRay;
					shadowRay.m_origin = intersection.pt;
					shadowRay.m_dir = Common::Sub_Vec3_By_Vec3(m_pLight->pos, intersection.pt);
					shadowRay.m_dir.Normalize();

					if (_IsInShadow(pScene, shadowRay))
					{
						*destBuffer = 0x0;
					}
					else
					{
						*destBuffer = _Shade(intersection).GetAsInt();
					}
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
		// This is different from transform by inv-proj matrix, WHY?
 		VEC3 imagePlanePt;
 		imagePlanePt.z = -cam->GetNearClip();
		imagePlanePt.x = fx * cam->GetImagePlaneHalfW();
		imagePlanePt.y = fy * cam->GetImagePlaneHalfH();

		oViewRay.m_origin = VEC3::ZERO;
		oViewRay.m_dir = imagePlanePt;
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
		RayTraceRenderList& objList = pScene->m_renderList_RayTrace;

		// TODO: Primary ray only need to test for objects in frustum
		for (size_t i=0; i<objList.size(); ++i)
		{
			RayTraceRenderable* obj = objList[i];

			VEC3 intersectPt;

			if (obj->DoRayIntersect(intersectPt, worldRay))
			{
				SIntersection curIntersect;
				curIntersect.pt = intersectPt;
				curIntersect.normal = obj->GetNormal(intersectPt);
				curIntersect.color = obj->m_color;

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
		VEC3 lightDir = Common::Sub_Vec3_By_Vec3(m_pLight->pos, intersection.pt);
		lightDir.Normalize();

		float nl = Common::DotProduct_Vec3_By_Vec3(intersection.normal, lightDir);

		SColor c = SColor::BLACK;
		if (nl > 0)
		{
			c = intersection.color * nl;
		}
		
		return c;
	}
	//------------------------------------------------------------------------------------
	bool RayTracer::_IsInShadow( Scene* pScene, const RAY& ray )
	{
		RayTraceRenderList& objList = pScene->m_renderList_RayTrace;

		for (size_t i=0; i<objList.size(); ++i)
		{
			RayTraceRenderable* obj = objList[i];
			VEC3 intersectPt;

			if (obj->m_bCastShadow && obj->DoRayIntersect(intersectPt, ray))
			{
				VEC3 testDir = Common::Sub_Vec3_By_Vec3(intersectPt, m_pLight->pos);
				// Is the intersection point near than the light?
				if(Common::DotProduct_Vec3_By_Vec3(testDir, ray.m_dir) < 0)
					return true;
			}
		}

		return false;
	}
}