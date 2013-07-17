/********************************************************************
	created:	9:7:2013   22:15
	filename	Utility.h
	author:		maval

	purpose:	实用工具,杂七杂八
*********************************************************************/
#ifndef Utility_h__
#define Utility_h__

typedef std::string STRING;

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
		assert(t >= 0.0f && t<= 1.0f);
		return T(s + (e - s) * t);
	}

	std::wstring	AnsiToUnicode(const char* src);
	STRING			UnicodeToEngine(const WCHAR* src);
}

#endif // Utility_h__