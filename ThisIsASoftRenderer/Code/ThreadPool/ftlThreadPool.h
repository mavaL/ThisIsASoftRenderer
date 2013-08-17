///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   ftlthread.h
/// @brief  Fishjam Template Library Base Header File.
/// @author fujie
/// @email  fishjam@163.com
/// @blogURL(Chinese) http://blog.csdn.net/fishjam
/// @date 03/30/2008
/// @defgroup ftlThreadPool ftl thread pool function and class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FTL_THREADPOOL_H
#define FTL_THREADPOOL_H
#pragma once

#ifndef FTL_BASE_H
#  error ftlThreadPool.h requires ftlbase.h to be included first
#endif

#include "ftlThread.h"
#include <set>
#include <map>
#include <atlstr.h>
//#include "ftlSharePtr.h"
//#include "ftlFunctional.h"

namespace SR
{
    //1. Can adjust running thread number automatic between min and max number according to the number of tasks
    //2. Can cancel any task at any time, even it's running(all the other Implemention can NOT do this, 
    //   WaitForThreadpoolWorkCallbacks can cancel the task that are waiting)
    //3. Can pause/resume/stop the whole thread pool; 
    //4. Support feedback notification(such as error, progress) by callback interface;
    //5. Implement by template, so you can translate params very easily 
    //6. Support task priority when submit a new task(not support adjust it dynamic);
    //7. Use the basic API,so can run under WinXP/Vista/WIn7, etc(CreateThreadpoolWork can use after Vista)
    //8. Support multi-instance(QueueUserWorkItem is single-instance) 
    //9. There is no manager thread, so the cost is low(QueueUserWorkItem has a manager thread)  
	template <typename T> class CFThreadPool;  

	template <typename T>
	class CFJobBase
	{
		friend class CFThreadPool<T>;   //allow CFThreadpool set m_pThreadPool/m_nJobIndex
	public:
		FTLINLINE CFJobBase();
		FTLINLINE CFJobBase(T& t);
		FTLINLINE virtual ~CFJobBase();

		//! compare job to decide the position in waiting container (Priority -> Index)
		bool operator < (const CFJobBase & other) const;

		//! Get or set priority, the smaller value has the higher priority, then will execute prior, default value is 0
		//  NOTE: the value must set before submit, can not adjust priority after submit now
		FTLINLINE LONG GetJobPriority() const { return m_nJobPriority; }
		FTLINLINE LONG SetJobPriority(LONG nNewPriority);


		FTLINLINE LONG GetJobIndex() const;
		FTLINLINE DWORD GetErrorStatus() const;
		FTLINLINE LPCTSTR GetErrorInfo() const;

        //! Job Param that user provide, this is the template argument
		T		m_JobParam;
		//FTLINLINE T& GetJobParam();
		//FTLINLINE const T& GetJobParam() const;

		//! Request the job to cancel
		FTLINLINE BOOL RequestCancel();
	protected:
		//! NOTE: if the job running, the logical is
        //  if( OnInitialize ){ Run -> OnFinalize };
		virtual BOOL OnInitialize();
		virtual BOOL Run() = 0;
		virtual VOID OnFinalize() = 0;

		//! if the job is cancel before running(cancel bye user or the thread pool stop), will call this function, need to clean the resources.
		FTLINLINE virtual VOID OnCancelJob() = 0;
	protected:
		FTLINLINE void _SetErrorStatus(DWORD dwErrorStatus, LPCTSTR pszErrorInfo);
		FTLINLINE void _NotifyProgress(LONGLONG nCurPos, LONGLONG nTotalSize);
		FTLINLINE void _NotifyError();
		FTLINLINE void _NotifyError(DWORD dwError, LPCTSTR pszDescription);
		FTLINLINE void _NotifyCancel();

        //! NOTE: this function is VERY IMPORTANT, the job will support stop/pause/resume by call this function frequently
        //        if don't want to support pause(such as a net transfer job), can pass 0 as dwMilliseconds
		FTLINLINE FTLThreadWaitType GetJobWaitType(DWORD dwMilliseconds = INFINITE) const;
	private:
		LONG		m_nJobPriority;
		LONG		m_nJobIndex;
		DWORD		m_dwErrorStatus;	//GetLastError
		CAtlString	m_strFormatErrorInfo;		
		HANDLE		m_hEventJobStop;
		//FJobStatus	m_JobStatus;
		CFThreadPool<T>* m_pThreadPool;
	};

	typedef enum tagGetJobType
	{
		typeStop,
		typeSubtractThread,
		typeGetJob,
		typeError,
	}GetJobType;

	//Callback functions, can access job param by pJob->m_JobParam
	FTLEXPORT template <typename T>
	class IFThreadPoolCallBack
	{
	public:
		//when job running, will call OnJobBegin and OnJobEnd by pool
		FTLINLINE virtual void OnJobBegin(LONG nJobIndex, CFJobBase<T>* pJob )
		{
		} 
		FTLINLINE virtual void OnJobEnd(LONG nJobIndex, CFJobBase<T>* pJob)
		{
		}

		//when job canceled before it run, will call OnJobCancel by pool
		FTLINLINE virtual void OnJobCancel(LONG nJobIndex, CFJobBase<T>* pJob)
		{
		}

		//the subclass of JobBase will call OnJobProgress and OnJobError, this is business-related 
		FTLINLINE virtual void OnJobProgress(LONG nJobIndex , CFJobBase<T>* pJob, LONGLONG nCurPos, LONGLONG nTotalSize)
		{
		}
		FTLINLINE virtual void OnJobError(LONG nJobIndex , CFJobBase<T>* pJob, DWORD dwError, LPCTSTR pszDescription)
		{
		}
	};

	FTLEXPORT template <typename T>  
	class CFThreadPool
	{
        //! allow job access m_hEventStop/m_hEventContinue in GetJobWaitType
		friend class CFJobBase<T>; 
		//DISABLE_COPY_AND_ASSIGNMENT(CFThreadPool);
	public:
		FTLINLINE CFThreadPool(IFThreadPoolCallBack<T>* pCallBack = NULL);
		FTLINLINE virtual ~CFThreadPool(void);

		//! Start the thread pool, will create nMinNumThreads threads at the first,
        //  then will adjust the thread number between nMinNumThreads and nMaxNumThreads according the running tasks.
		FTLINLINE BOOL Start(LONG nMinNumThreads, LONG nMaxNumThreads);

		//! Request to stop the thread pool
		//! NOTE:this just set StopEvent, need Job to handle it according to GetJobWaitType result
		FTLINLINE BOOL Stop();

        //! wait all the job thread exit
        FTLINLINE BOOL Wait(DWORD dwTimeOut = FTL_MAX_THREAD_DEADLINE_CHECK);

        //! Stop and wait all the job thread exit
		FTLINLINE BOOL StopAndWait(DWORD dwTimeOut = FTL_MAX_THREAD_DEADLINE_CHECK);

		//! clear the works that still waiting
		FTLINLINE BOOL ClearUndoWork();

		//! Submit job to the thread pool, If there is not any idle thread and current running thread number is smaller than max number 
        //! then will create a new thread. 
        //! @param [in] pJob, the work job
        //! @param [out] pOutJobIndex, the job index, can CancelJob by it later
		FTLINLINE BOOL SubmitJob(CFJobBase<T>* pJob, LONG* pOutJobIndex);

		//! Cancel special job
		FTLINLINE BOOL CancelJob(LONG nJobIndex);
		//FTLINLINE BOOL PauseJob(LONG nJobIndex);
		//FTLINLINE BOOL ResumeJob(LONG nJobIndex);

		//! Request pause thread pool
		FTLINLINE BOOL Pause();

		//! Request resume thread pool
		FTLINLINE BOOL Resume();

        FTLINLINE BOOL HadRequestPause() const;
		FTLINLINE BOOL HadRequestStop() const;

		FTLINLINE BOOL IsAllFinished();
		FTLINLINE void Flush();

	protected:
		//! add running thread, if current thread number + nThreadNum <= m_nMaxNumThreads, it will success
		FTLINLINE BOOL _AddJobThread(LONG nThreadNum);
		FTLINLINE void _DestroyPool();
		FTLINLINE void _DoJobs();

		FTLINLINE GetJobType _GetJob(CFJobBase<T>** ppJob);

		FTLINLINE void _NotifyJobBegin(CFJobBase<T>* pJob);
		FTLINLINE void _NotifyJobEnd(CFJobBase<T>* pJob);
		FTLINLINE void _NotifyJobCancel(CFJobBase<T>* pJob);

		FTLINLINE void _NotifyJobProgress(CFJobBase<T>* pJob, LONGLONG nCurPos, LONGLONG nTotalSize);
		FTLINLINE void _NotifyJobError(CFJobBase<T>* pJob, DWORD dwError, LPCTSTR pszDescription); 

	protected:
		LONG m_nMinNumThreads;					//! the minimum running thread count
		LONG m_nMaxNumThreads;					//! the maximum running thread count
		IFThreadPoolCallBack<T>* m_pCallBack;	//! callback interface to provide
		LONG m_nJobIndex;						//! job index, will increase after each SubmitJob

		LONG m_nRunningJobNumber;				//! Current running job count
		LONG m_nRunningThreadNum;				//! Current running thread count(will fire m_hEventAllThreadComplete after all job thread end)

        typedef std::map<DWORD, HANDLE>   TaskThreadContrainer;     //!maintain thread, thread id -> handle
        TaskThreadContrainer m_TaskThreads;

		//HANDLE	m_hMgrThread;					//! Manager thread handle

		//! maintain all the waiting jobs, will order by priority -> jobIndex
		typedef typename UnreferenceLess< CFJobBase<T> * >	JobBaseUnreferenceLess;
		typedef std::set<CFJobBase<T>*, JobBaseUnreferenceLess > WaitingJobContainer;
		WaitingJobContainer		m_WaitingJobs;

		//! maintain all the running jobs
		typedef std::map<LONG, CFJobBase<T>* >	DoingJobContainer;
		DoingJobContainer		m_DoingJobs;

		HANDLE m_hEventStop;                    //! Stop pool event
		HANDLE m_hEventAllThreadComplete;		//! all thread exit will fire this event
		HANDLE m_hEventContinue;				//! The pool continue running event
		HANDLE m_hSemaphoreJobToDo;             //! Semaphore for waiting job number
        HANDLE m_hSemaphoreSubtractThread;      //! Semaphore for subtract thread, the initial value is 0

		CFCriticalSection m_lockDoingJobs;		//! access for m_DoingJobs
		CFCriticalSection m_lockWaitingJobs;    //! access for m_WaitingJobs
		CFCriticalSection m_lockThreads;        //! access for m_TaskThreads

		static unsigned int CALLBACK JobThreadProc(void *pParam);    //! Job work thread function
		//static unsigned int CALLBACK MgrThreadProc(void* pParam);	 //! Manager work thread function
	};
}

#endif //FTL_THREADPOOL_H

#ifndef USE_EXPORT
#  include "ftlThreadPool.hpp"
#endif