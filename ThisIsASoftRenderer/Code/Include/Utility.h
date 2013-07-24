/********************************************************************
	created:	9:7:2013   22:15
	filename	Utility.h
	author:		maval

	purpose:	ʵ�ù���,�����Ӱ�
*********************************************************************/
#ifndef Utility_h__
#define Utility_h__

#include "Prerequiestity.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) if(p) { delete p; p=nullptr; }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) if(p) { delete []p; p=nullptr; }
#endif

namespace Ext
{
	template<class T>
	inline void Swap(T& t1, T& t2)
	{
		T tmp = t1;
		t1 = t2;
		t2 = tmp;
	}

	template<class T>
	inline T Clamp(const T& val, const T& left, const T& right)
	{
		if(val < left)
			return left;
		else if(val > right)
			return right;
		else
			return val;
	}

	template<class T>
	inline T LinearLerp(const T& s, const T& e, float t)
	{
		return T(s + (e - s) * t);
	}

	__forceinline int Ceil32_Fast(float x)
	{
		const float h = 0.5f;
		int t;

		_asm
		{
			fld	x
			fadd	h
			fistp	t
		}

		return t;
	}

	__forceinline int Floor32_Fast(float x)
	{
		const float h = 0.5f;
		int t;

		_asm
		{
			fld	x
			fsub	h
			fistp	t
		}

		return t;
	}

	__forceinline int Ftoi32_Fast(float x)
	{
		int t;
		_asm
		{
			fld	x
			fistp	t
		}

		return t;

		// SSE?
		//return _mm_cvtt_ss2si(_mm_load_ss(&x)); 
	}

	std::wstring	AnsiToUnicode(const char* src);
	STRING			UnicodeToEngine(const WCHAR* src);
}

#endif // Utility_h__