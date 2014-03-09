#ifndef Intersection_inl__
#define Intersection_inl__

namespace Common
{
	//------------------------------------------------------------------------------------
	__forceinline VEC3 Ray::GetPoint( float t ) const
	{
		return Add_Vec3_By_Vec3(m_origin, Common::Multiply_Vec3_By_K(m_dir, t));
	}
	//------------------------------------------------------------------------------------
	__forceinline std::pair<bool, float> Ray::Intersect_Plane( const SR::RayTrace_Plane& plane ) const
	{
		// p = o + vt
		// p * n = d
		// t = (d - (o * n)) / (n * v)

 		float dot = Common::DotProduct_Vec3_By_Vec3(plane.normal, m_dir);
 		if(Ext::Equal(dot, 0.0f))
 			return std::pair<bool, float>(false, 0.0f);

		float t = (plane.d - Common::DotProduct_Vec3_By_Vec3(m_origin, plane.normal)) / dot; 
		if (t < 0 ||
			t < 1e-03)	// Avoid self-shadowing cause by imprecision problem!
			return std::pair<bool, float>(false, 0.0f);

		return std::pair<bool, float>(true, t);
	}
	//------------------------------------------------------------------------------------
	__forceinline std::pair<bool, float> Ray::Intersect_Box( const SR::RayTrace_Box& box ) const
	{
		//// TODO: Just a stupid prototype method, should optimize it
		//const VEC3& minp = box.minPt;
		//const VEC3& maxp = box.maxPt;

		//const SR::RayTrace_Plane plane[6] = 
		//{
		//	SR::RayTrace_Plane(VEC3::UNIT_Y		, maxp.y),	// Top
		//	SR::RayTrace_Plane(VEC3::NEG_UNIT_Y	, minp.y),	// Bottom
		//	SR::RayTrace_Plane(VEC3::NEG_UNIT_X	, minp.x),	// Left
		//	SR::RayTrace_Plane(VEC3::UNIT_X		, maxp.x),	// Right
		//	SR::RayTrace_Plane(VEC3::UNIT_Z		, maxp.z),	// Front
		//	SR::RayTrace_Plane(VEC3::NEG_UNIT_Z	, minp.z),	// Back
		//};

		//const VEC3 planeMinMaxPt[6][2] =
		//{
		//	VEC3(minp.x, maxp.y-0.1f, minp.z), VEC3(maxp.x, maxp.y+0.1f, maxp.z),	// Top
		//	VEC3(minp.x, minp.y-0.1f, minp.z), VEC3(maxp.x, minp.y+0.1f, maxp.z),	// Bottom
		//	VEC3(minp.x-0.1f, minp.y, minp.z), VEC3(minp.x+0.1f, maxp.y, maxp.z),	// Left
		//	VEC3(maxp.x-0.1f, minp.y, minp.z), VEC3(maxp.x+0.1f, maxp.y, maxp.z),	// Right
		//	VEC3(minp.x, minp.y, maxp.z-0.1f), VEC3(maxp.x, maxp.y, maxp.z+0.1f),	// Front
		//	VEC3(minp.x, minp.y, minp.z-0.1f), VEC3(maxp.x, maxp.y, minp.z+0.1f),	// Back
		//};

		//bool bHit = false;
		//float t = 0.0f;

		//for (int i=0; i<6; ++i)
		//{
		//	// Step 1 : Hit the plane
		//	std::pair<bool, float> hitResult = Intersect_Plane(plane[i]);

		//	if (!hitResult.first)
		//		continue;

		//	// Step 2 : Is the hit point inside rectangle
		//	const float cur_t = hitResult.second;
		//	const VEC3 hitPt = GetPoint(cur_t);

		//	if (hitPt > planeMinMaxPt[i][0] && hitPt < planeMinMaxPt[i][1])
		//	{
		//		// Step 3 : Is it the nearest hit point
		//		if (!bHit || cur_t<t)
		//		{
		//			t = cur_t;
		//			bHit = true;
		//		}
		//	}
		//}

		//return std::pair<bool, float>(bHit, t);

		// From Ogre, faster 10x than above my code :D

		float lowt = 0.0f;
		float t;
		bool hit = false;
		Vector3 hitpoint;
		const VEC3& min = box.minPt;
		const VEC3& max = box.maxPt;
		const Vector3& rayorig = m_origin;
		const Vector3& raydir = m_dir;

		// Check each face in turn, only check closest 3
		// Min x
		if (rayorig.x <= min.x && raydir.x > 0)
		{
			t = (min.x - rayorig.x) / raydir.x;
			if (t > 1e-03)
			{
				// Substitute t back into ray and check bounds and dist
				hitpoint = GetPoint(t);
				if (hitpoint.y >= min.y && hitpoint.y <= max.y &&
					hitpoint.z >= min.z && hitpoint.z <= max.z &&
					(!hit || t < lowt))
				{
					hit = true;
					lowt = t;
				}
			}
		}
		// Max x
		if (rayorig.x >= max.x && raydir.x < 0)
		{
			t = (max.x - rayorig.x) / raydir.x;
			if (t > 1e-03)
			{
				// Substitute t back into ray and check bounds and dist
				hitpoint = GetPoint(t);
				if (hitpoint.y >= min.y && hitpoint.y <= max.y &&
					hitpoint.z >= min.z && hitpoint.z <= max.z &&
					(!hit || t < lowt))
				{
					hit = true;
					lowt = t;
				}
			}
		}
		// Min y
		if (rayorig.y <= min.y && raydir.y > 0)
		{
			t = (min.y - rayorig.y) / raydir.y;
			if (t > 1e-03)
			{
				// Substitute t back into ray and check bounds and dist
				hitpoint = GetPoint(t);
				if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
					hitpoint.z >= min.z && hitpoint.z <= max.z &&
					(!hit || t < lowt))
				{
					hit = true;
					lowt = t;
				}
			}
		}
		// Max y
		if (rayorig.y >= max.y && raydir.y < 0)
		{
			t = (max.y - rayorig.y) / raydir.y;
			if (t > 1e-03)
			{
				// Substitute t back into ray and check bounds and dist
				hitpoint = GetPoint(t);
				if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
					hitpoint.z >= min.z && hitpoint.z <= max.z &&
					(!hit || t < lowt))
				{
					hit = true;
					lowt = t;
				}
			}
		}
		// Min z
		if (rayorig.z <= min.z && raydir.z > 0)
		{
			t = (min.z - rayorig.z) / raydir.z;
			if (t > 1e-03)
			{
				// Substitute t back into ray and check bounds and dist
				hitpoint = GetPoint(t);
				if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
					hitpoint.y >= min.y && hitpoint.y <= max.y &&
					(!hit || t < lowt))
				{
					hit = true;
					lowt = t;
				}
			}
		}
		// Max z
		if (rayorig.z >= max.z && raydir.z < 0)
		{
			t = (max.z - rayorig.z) / raydir.z;
			if (t > 1e-03)
			{
				// Substitute t back into ray and check bounds and dist
				hitpoint = GetPoint(t);
				if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
					hitpoint.y >= min.y && hitpoint.y <= max.y &&
					(!hit || t < lowt))
				{
					hit = true;
					lowt = t;
				}
			}
		}

		return std::pair<bool, float>(hit, lowt);
	}
	//------------------------------------------------------------------------------------
	__forceinline std::pair<bool, float> Ray::Intersect_Triangle( const VEC3& p1, const VEC3& p2, const VEC3& p3 ) const
	{
		// First check ray-plane intersection
		SR::RayTrace_Plane plane(p1, p2, p3);

		std::pair<bool, float> hitResult = Intersect_Plane(plane);
		if(!hitResult.first)
			return std::pair<bool, float>(false, 0.0f);

		// Then check whether intersection point is inside triangle
		if(!IsPointInTriangle(GetPoint(hitResult.second), p1, p2, p3))
			return std::pair<bool, float>(false, 0.0f);

		return hitResult;
	}
	//------------------------------------------------------------------------------------
	__forceinline bool		IsPointInTriangle(const Vector3& pt, const Vector3& p1, const Vector3& p2, const Vector3& p3)
	{
		// First check whether point is on the plane
		SR::RayTrace_Plane plane(p1, p2, p3);
		float d1 = Common::DotProduct_Vec3_By_Vec3(plane.normal, pt);

		if(!Ext::Equal(d1, plane.d))
			return false;

		// for triangle: p = w1p1 + w2p2 + w3p3, w1 + w2 + w3 = 1
		const VEC3 v1 = Common::Sub_Vec3_By_Vec3(p2, p1);
		const VEC3 v2 = Common::Sub_Vec3_By_Vec3(p3, p1);
		const VEC3 R = Common::Sub_Vec3_By_Vec3(pt, p1);
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
	//------------------------------------------------------------------------------------
	__forceinline bool		IsPointInTriangle(const Vector2& pt, const Vector2& p1, const Vector2& p2, const Vector2& p3)
	{
		// for triangle: p = w1p1 + w2p2 + w3p3, w1 + w2 + w3 = 1
		const VEC2 v1 = Common::Sub_Vec2_By_Vec2(p2, p1);
		const VEC2 v2 = Common::Sub_Vec2_By_Vec2(p3, p1);
		const VEC2 R = Common::Sub_Vec2_By_Vec2(pt, p1);
		float dot = Common::DotProduct_Vec2_By_Vec2(v1, v2);
		float a = Common::DotProduct_Vec2_By_Vec2(v1, v1);
		float b = Common::DotProduct_Vec2_By_Vec2(v2, v2);
		float e = Common::DotProduct_Vec2_By_Vec2(R, v1);
		float f = Common::DotProduct_Vec2_By_Vec2(R, v2);
		float denom = 1.0f / (a * b - dot * dot);

		float w1 = (b * e + f * -dot) * denom;
		float w2 = (-dot * e + a * f) * denom;

		if(w1 < 0 || w2 < 0 || (w1+w2)>1)
			return false;

		return true;
	}
}

#endif // Intersection_inl__
