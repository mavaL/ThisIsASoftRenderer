#ifndef MathDef_Intersection_h__
#define MathDef_Intersection_h__

namespace Common
{
	//------------------------------------------------------------------------------------
	__forceinline bool Ray::Intersect_Box( VEC3& oIntersectPt, const VEC3& minPt, const VEC3& maxPt ) const
	{
		return true;
	}
	//------------------------------------------------------------------------------------
	__forceinline bool Ray::Intersect_Triangle( VEC3& oIntersectPt, const VEC3& p1, const VEC3& p2, const VEC3& p3 ) const
	{
		// First check ray-plane intersection
		// p = o + vt
		// p * n = d
		// t = (d - (o * n)) / (n * v)

		VEC3 v1 = Common::Sub_Vec3_By_Vec3(p2, p1);
		VEC3 v2 = Common::Sub_Vec3_By_Vec3(p3, p1);
		VEC3 n = Common::CrossProduct_Vec3_By_Vec3(v1, v2);
		n.Normalize();

		float d = Common::DotProduct_Vec3_By_Vec3(p1, n);

		float tmp = Common::DotProduct_Vec3_By_Vec3(n, m_dir);
		if(!(tmp < 0))
			return false;

		float t = (d - Common::DotProduct_Vec3_By_Vec3(m_origin, n)) / tmp; 
		if (t < 0)
			return false;

		// Then check whether intersection point is inside triangle
		// for triangle: p = w1p1 + w2p2 + w3p3, w1 + w2 + w3 = 1
		oIntersectPt = GetPoint(t);

		VEC3 R = Common::Sub_Vec3_By_Vec3(oIntersectPt, p1);
		float dot = Common::DotProduct_Vec3_By_Vec3(v1, v2);
		float a = Common::DotProduct_Vec3_By_Vec3(v1, v1);
		float b = Common::DotProduct_Vec3_By_Vec3(v2, v2);
		float e = Common::DotProduct_Vec3_By_Vec3(R, v1);
		float f = Common::DotProduct_Vec3_By_Vec3(R, v2);
		float denom = 1.0f / (a * b - dot * dot);

		float w1 = (b * e + f * -dot) * denom;
		float w2 = (-dot * e + a * f) * denom;

		if(w1 < 0 || w2 < 0 || (w1+w2)>1)
			return false;

		return true;
	}
}

#endif // MathDef_Intersection_h__
