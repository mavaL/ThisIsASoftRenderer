#include "stdafx.h"
#include "Intersection.h"

namespace SR
{
	//------------------------------------------------------------------------------------
	RayTrace_Box::RayTrace_Box( const VEC3& _minPt, const VEC3& _maxPt )
		:minPt(_minPt)
		,maxPt(_maxPt)
	{
		assert(minPt < maxPt);
	}
	//------------------------------------------------------------------------------------
	bool RayTrace_Box::DoRayIntersect( VEC3& oIntersectPt, const RAY& ray ) const
	{
		auto hitResult = ray.Intersect_Box(*this);

		if (hitResult.first)
		{
			oIntersectPt = ray.GetPoint(hitResult.second);
			return true;
		}
		else
		{
			return false;
		}
	}
	//------------------------------------------------------------------------------------
	VEC3 RayTrace_Box::GetNormal( const VEC3& surfacePt ) const
	{
// 		const VEC3 center = Common::Multiply_Vec3_By_K(Common::Add_Vec3_By_Vec3(minPt, maxPt), 0.5f);
// 		VEC3 n = Common::Sub_Vec3_By_Vec3(surfacePt, center);
// 		n.Normalize();

		if (Ext::Equal(surfacePt.x, minPt.x)) return VEC3::NEG_UNIT_X;
		else if (Ext::Equal(surfacePt.y, minPt.y)) return VEC3::NEG_UNIT_Y;
		else if (Ext::Equal(surfacePt.z, minPt.z)) return VEC3::NEG_UNIT_Z;
		else if (Ext::Equal(surfacePt.x, maxPt.x)) return VEC3::UNIT_X;
		else if (Ext::Equal(surfacePt.y, maxPt.y)) return VEC3::UNIT_Y;
		else if (Ext::Equal(surfacePt.z, maxPt.z)) return VEC3::UNIT_Z;

		assert(0);
//		return n;
	}
	//------------------------------------------------------------------------------------
	RayTrace_Plane::RayTrace_Plane( const VEC3& p1, const VEC3& p2, const VEC3& p3 )
	{
		const VEC3 v1 = Common::Sub_Vec3_By_Vec3(p2, p1);
		const VEC3 v2 = Common::Sub_Vec3_By_Vec3(p3, p1);

		normal = Common::CrossProduct_Vec3_By_Vec3(v1, v2);
		normal.Normalize();

		d = Common::DotProduct_Vec3_By_Vec3(p1, normal);
	}
	//------------------------------------------------------------------------------------
	RayTrace_Plane::RayTrace_Plane( const VEC3& _n, float _d )
	{
		// Assume normal has been normalized
		normal = _n;
		d = _d;
	}
	//------------------------------------------------------------------------------------
	bool RayTrace_Plane::DoRayIntersect( VEC3& oIntersectPt, const RAY& ray ) const
	{
		auto hitResult = ray.Intersect_Plane(*this);

		if (hitResult.first)
		{
			oIntersectPt = ray.GetPoint(hitResult.second);
			return true;
		}
		else
		{
			return false;
		}
	}
}