/********************************************************************
	created:	2013/08/08
	created:	8:8:2013   10:57
	filename: 	Profiler.h
	author:		maval
	
	purpose:	性能profile的封装
*********************************************************************/
#ifndef Profiler_h__
#define Profiler_h__

#include "Prerequiestity.h"
#include "ThreadPool/ftlThread.h"
#include "Color.h"

namespace Ext
{
	//统计线程完成的工作
	struct SThreadStat 
	{
		DWORD nRenderedTri;
		DWORD nRenderedPixel;
	};

	//渲染数据统计
	struct SFrameStatics 
	{
		void Reset()
		{
			nObjCulled = nBackFace = nFaceCulled = nRenderedFace = 0;
			threadStatMap.clear();
		}

		DWORD	nObjCulled;
		DWORD	nBackFace;
		DWORD	nFaceCulled;
		DWORD	nRenderedFace;
		DWORD	lastFPS;
		std::unordered_map<DWORD, SThreadStat> threadStatMap;
	};

	/////////////////////////////////////////////////////////////////////
	class Profiler
	{
	public:
		Profiler();
		~Profiler();

	public:
		void	DisplayHelpInfo();

		void	AddRenderedFace()
		{
#if USE_MULTI_THREAD == 1
			m_lock.Lock();
#endif
			DWORD tid = GetCurrentThreadId();
			++m_frameStatics.nRenderedFace;
			++m_frameStatics.threadStatMap[tid].nRenderedTri;

#if USE_MULTI_THREAD == 1
			m_lock.UnLock();
#endif
		}

		void	AddRenderedPixel()
		{
#if USE_MULTI_THREAD == 1
			m_lock.Lock();
#endif

			DWORD tid = GetCurrentThreadId();
			++m_frameStatics.threadStatMap[tid].nRenderedPixel;

#if USE_MULTI_THREAD == 1
			m_lock.UnLock();
#endif
		}

		SFrameStatics	m_frameStatics;
		std::vector<SR::SColor>	m_vecMipColor;		// Used for visualize mip distribution

	private:
		SR::CFCriticalSection	m_lock;
	};
}


#endif // Profiler_h__