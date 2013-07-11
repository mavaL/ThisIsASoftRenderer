/********************************************************************
	created:	9:7:2013   22:15
	filename	Utility.h
	author:		maval

	purpose:	实用工具,杂七杂八
*********************************************************************/
#ifndef Utility_h__
#define Utility_h__

typedef std::string STRING;

namespace Ext
{
	template<class T>
	inline void Swap(T& t1, T& t2)
	{
		T tmp = t1;
		t1 = t2;
		t2 = tmp;
	}
}

#endif // Utility_h__