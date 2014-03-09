/********************************************************************
	created:	15:2:2014   12:30
	filename	Intersection.h
	author:		maval

	purpose:	Ray-geometry intersection algorithm
*********************************************************************/
#ifndef Intersection_h__
#define Intersection_h__

#include "Prerequiestity.h"
#include "MathDef.h"
#include "Color.h"

namespace SR
{
	///////////////////////////////////////////////////
	class RayTraceRenderable
	{
	public:
		RayTraceRenderable():m_bCastShadow(false),m_color(SColor::WHITE) {}
		virtual ~RayTraceRenderable() {}

		virtual bool	DoRayIntersect(VEC3& oIntersectPt, const RAY& ray) const = 0;
		virtual VEC3	GetNormal(const VEC3& surfacePt) const = 0;

		bool	m_bCastShadow;
		SColor	m_color;
	};
	typedef std::vector<RayTraceRenderable*>	RayTraceRenderList;

	///////////////////////////////////////////////////
	class RayTrace_Box : public RayTraceRenderable
	{
	public:
		VEC3	minPt, maxPt;	// TODO: For now just considered AABB, not OBB

		RayTrace_Box(const VEC3& _minPt, const VEC3& _maxPt);

		virtual bool	DoRayIntersect(VEC3& oIntersectPt, const RAY& ray) const;
		virtual VEC3	GetNormal(const VEC3& surfacePt) const;
	};

	///////////////////////////////////////////////////
	class RayTrace_Plane : public RayTraceRenderable
	{
	public:
		VEC3 normal;
		float d;

		RayTrace_Plane(const VEC3& _n, float _d);
		RayTrace_Plane(const VEC3& p1, const VEC3& p2, const VEC3& p3);

		virtual bool	DoRayIntersect(VEC3& oIntersectPt, const RAY& ray) const;
		virtual VEC3	GetNormal(const VEC3& surfacePt) const { return normal; }
	};
}

namespace Common
{
	class Ray
	{
	public:
		Ray():m_origin(VEC3::ZERO),m_dir(VEC3::ZERO) {}

		VEC3	m_origin;
		VEC3	m_dir;

		VEC3	GetPoint(float t) const;

		std::pair<bool, float>	Intersect_Box(const SR::RayTrace_Box& box) const;
		std::pair<bool, float>	Intersect_Triangle(const VEC3& p1, const VEC3& p2, const VEC3& p3) const;
		std::pair<bool, float>	Intersect_Plane(const SR::RayTrace_Plane& plane) const;
	};

	bool		IsPointInTriangle(const Vector3& pt, const Vector3& p1, const Vector3& p2, const Vector3& p3);
	bool		IsPointInTriangle(const Vector2& pt, const Vector2& p1, const Vector2& p2, const Vector2& p3);
}

#include "Intersection.inl"

#endif // Intersection_h__