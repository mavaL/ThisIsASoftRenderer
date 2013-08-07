/********************************************************************
	created:	2013/08/07
	created:	7:8:2013   14:55
	filename: 	Color.h
	author:		maval
	
	purpose:	简单的颜色类
*********************************************************************/
#ifndef Color_h__
#define Color_h__

#include "Prerequiestity.h"

static float inv_byte = 1.0f / 255;

namespace SR
{
	struct SColor 
	{
		SColor() {}
		SColor(float _r, float _g, float _b, float _a = 1.0f):a(_a),r(_r),g(_g),b(_b) {}

		void Set(float _r, float _g, float _b, float _a = 1.0f)
		{
			a = _a; r = _r; g = _g; b = _b;
		}

		SColor operator* (float k) const
		{ 
			return SColor(r*k, g*k, b*k, a*k);
		}

		SColor& operator*= (float k)
		{
			a *= k;
			r *= k;
			g *= k;
			b *= k;

			return *this;
		}

		SColor operator* (const VEC3& v) const
		{
			return SColor(r*v.x, g*v.y, b*v.z, a);
		}

		SColor& operator*= (const VEC3& rhs)
		{
			r *= rhs.x;
			g *= rhs.y;
			b *= rhs.z;

			return *this;
		}

		SColor& operator+= (const SColor& rhs)
		{
			a += rhs.a;
			r += rhs.r;
			g += rhs.g;
			b += rhs.b;

			return *this;
		}

		SColor& operator*= (const SColor& rhs)
		{
			a *= rhs.a;
			r *= rhs.r;
			g *= rhs.g;
			b *= rhs.b;

			return *this;
		}

		void	Saturate()
		{
			a = Ext::Clamp(a, 0.0f, 1.0f);
			r = Ext::Clamp(r, 0.0f, 1.0f);
			g = Ext::Clamp(g, 0.0f, 1.0f);
			b = Ext::Clamp(b, 0.0f, 1.0f);
		}

		void	SetAsInt(DWORD color)
		{
			a = ((color >> 24) & 0xff) * inv_byte;
			r = ((color >> 16) & 0xff) * inv_byte;
			g = ((color >> 8) & 0xff) * inv_byte;
			b = ((color >> 0) & 0xff) * inv_byte;
		}

		DWORD	GetAsInt() const
		{
			BYTE tmp_a = Ext::Ftoi32_Fast(a * 255);
			BYTE tmp_r = Ext::Ftoi32_Fast(r * 255);
			BYTE tmp_g = Ext::Ftoi32_Fast(g * 255);
			BYTE tmp_b = Ext::Ftoi32_Fast(b * 255);

			DWORD ret = (tmp_a << 24) + (tmp_r << 16) + (tmp_g << 8) + (tmp_b);

			return ret;
		}

		float a, r, g, b;

		static SColor WHITE;
		static SColor BLACK;
		static SColor BLUE;
		static SColor RED;
	};
}

namespace Ext
{
	//特化SColor
	template<> inline void LinearLerp(SR::SColor& result, const SR::SColor& s, const SR::SColor& e, float t)
	{
		LinearLerp(result.a, s.a, e.a, t);
		LinearLerp(result.r, s.r, e.r, t);
		LinearLerp(result.g, s.g, e.g, t);
		LinearLerp(result.b, s.b, e.b, t);
	}
}

#endif // Color_h__