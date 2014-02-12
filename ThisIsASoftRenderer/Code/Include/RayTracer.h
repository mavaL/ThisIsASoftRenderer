/********************************************************************
	created:	10:2:2014   20:30
	filename	RayTracer.h
	author:		maval

	purpose:	Ray tracer
*********************************************************************/
#ifndef RayTracer_h__
#define RayTracer_h__

#include "Prerequiestity.h"
#include "MathDef.h"
#include "Color.h"

namespace SR
{
	class RayTracer
	{
		struct SIntersection 
		{
			VEC3	pt;
			VEC3	normal;
		};

	public:
		RayTracer();
		~RayTracer();

	public:
		// Intersection unit test
		void	RunIntersectUnitTest();

		void	ProcessScene(Scene* pScene);

	private:
		void	_GetRayFromScreenPt(RAY& oWorldRay, RAY& oViewRay, int x, int y);
		bool	_GetIntersection(SIntersection& oIntersection, Scene* pScene, const RAY& worldRay, const RAY& viewRay);
		SColor	_Shade(const SIntersection& intersection);

		SPointLight*	m_pLight;
	};
}


#endif // RayTracer_h__