#ifndef FTL_THREADPOOL_HPP
#define FTL_THREADPOOL_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlThreadPool.h"
#endif

namespace SR
{
	///////////////////////////////////////////// CFJobBase ///////////////////////////////////////////////////
	template <typename T>
	CFJobBase<T>::CFJobBase()
		:m_JobParam(T())
	{
		m_nJobPriority = 0;
		m_nJobIndex = 0;
		m_pThreadPool = NULL;
		m_dwErrorStatus = ERROR_SUCCESS;
	}
	template <typename T>
	CFJobBase<T>::CFJobBase(T& t)
		:m_JobParam(t)
	{
		m_nJobPriority = 0;
		m_nJobIndex = 0;
		m_pThreadPool = NULL;
		m_dwErrorStatus = ERROR_SUCCESS;
	}

	template <typename T>
	CFJobBase<T>::~CFJobBase()
	{
		//FTLASSERT(NULL == m_hEventJobStop);
	}

	template <typename T>
	bool CFJobBase<T>::operator < (const CFJobBase<T> & other) const
	{
		COMPARE_MEM_LESS(m_nJobPriority, other);
		COMPARE_MEM_LESS(m_nJobIndex, other);
		return true;
	}
	
	template <typename T>
	LONG CFJobBase<T>::SetJobPriority(LONG nNewPriority)
	{
		LONG nOldPriority = m_nJobPriority;
		m_nJobPriority = nNewPriority;
		return nOldPriority;
	}

	template <typename T>
	LONG CFJobBase<T>::GetJobIndex() const
	{
		return m_nJobIndex;
	}
	
	template <typename T>
	DWORD CFJobBase<T>::GetErrorStatus() const
	{
		return m_dwErrorStatus;
	}

	template <typename T>
	LPCTSTR CFJobBase<T>::GetErrorInfo() const
	{
		//return m_strFromatErrorInfo.GetString();
	}

	template <typename T>
	BOOL CFJobBase<T>::RequestCancel()
	{
		BOOL bRet = FALSE;
		//API_VERIFY(SetEvent(m_hEventJobStop));
		return bRet;
	}

	template <typename T>
	void CFJobBase<T>::_SetErrorStatus(DWORD dwErrorStatus, LPCTSTR pszErrorInfo)
	{
		m_dwErrorStatus = dwErrorStatus;
		if (pszErrorInfo)
		{
			m_strFormatErrorInfo.Format(TEXT("%s"), pszErrorInfo);
		}
	}

	template <typename T>
	void CFJobBase<T>::_NotifyProgress(LONGLONG nCurPos, LONGLONG nTotalSize)
	{
		FTLASSERT(m_pThreadPool);
		m_pThreadPool->_NotifyJobProgress(this, nCurPos, nTotalSize);
	}

	template <typename T>
	void CFJobBase<T>::_NotifyCancel()
	{
		m_pThreadPool->_NotifyJobCancel(this);
	}

	template <typename T>
	void CFJobBase<T>::_NotifyError()
	{
		m_pThreadPool->_NotifyJobError(this, m_dwErrorStatus, m_strFormatErrorInfo);
	}

	template <typename T>
	void CFJobBase<T>::_NotifyError(DWORD dwError, LPCTSTR pszDescription)
	{
		m_pThreadPool->_NotifyJobError(this, dwError, pszDescription);
	}

	template <typename T>
	BOOL CFJobBase<T>::OnInitialize()
	{
		BOOL bRet = TRUE;
		return bRet;
	}

	template <typename T>
	FTLThreadWaitType CFJobBase<T>::GetJobWaitType(DWORD dwMilliseconds /* = INFINITE*/) const
	{
		FUNCTION_BLOCK_TRACE(0);
		FTLThreadWaitType waitType = ftwtError;
		HANDLE waitEvent[] = 
		{
			m_pThreadPool->m_hEventStop,
			m_pThreadPool->m_hEventContinue
		};
		DWORD dwResult = ::WaitForMultipleObjects(_countof(waitEvent), waitEvent, FALSE, dwMilliseconds);
		switch (dwResult)
		{
		case WAIT_OBJECT_0:
			waitType = ftwtStop;		//Thread Pool Stop Event
			break;
		case WAIT_OBJECT_0 + 1:
			waitType = ftwtContinue;	//Thread Pool Continue Event
			break;
		case WAIT_TIMEOUT:
			waitType = ftwtTimeOut;
			break;
		default:
			FTLASSERT(FALSE);
			waitType = ftwtError;
			break;
		}
		return waitType;
	}

	//////////////////////////////////////    CFThreadPool    ///////////////////////////////////////////////////
	template <typename T>  
	CFThreadPool<T>::CFThreadPool(IFThreadPoolCallBack<T>* pCallBack /* = NULL*/)
		:m_pCallBack(pCallBack)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);

		m_nMinNumThreads = 0;
		m_nMaxNumThreads = 1;

		m_nRunningJobNumber = 0;
		m_nJobIndex = 0;
		m_nRunningThreadNum = 0;

		m_hEventStop = CreateEvent(NULL, TRUE, FALSE, NULL);
		FTLASSERT(NULL != m_hEventStop);

		m_hEventContinue = ::CreateEvent(NULL, TRUE, TRUE, NULL);
		FTLASSERT(NULL != m_hEventContinue);

		m_hEventFlushJobs = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		FTLASSERT(NULL != m_hEventFlushJobs);
	}

	template <typename T>  
	CFThreadPool<T>::~CFThreadPool(void)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);

		_DestroyPool();

		FTLASSERT(m_WaitingJobs.empty());
	}

	template <typename T>  
	BOOL CFThreadPool<T>::Start(LONG nMinNumThreads, LONG nMaxNumThreads)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		FTLASSERT( 0 <= nMinNumThreads );
		FTLASSERT( nMinNumThreads <= nMaxNumThreads );       
		FTLTRACE(TEXT("CFThreadPool::Start, ThreadNum is [%d-%d]\n"), nMinNumThreads, nMaxNumThreads);

		BOOL bRet = TRUE;
		m_nMinNumThreads = nMinNumThreads;
		m_nMaxNumThreads = nMaxNumThreads;

		API_VERIFY(ResetEvent(m_hEventStop));
		API_VERIFY(SetEvent(m_hEventContinue));
		
		{
			CFAutoLock<CFLockObject>   locker(&m_lockThreads);
			_AddJobThread(m_nMinNumThreads);
			m_nRunningThreadNum = m_nMinNumThreads;
		}
		return bRet;
	}

	template <typename T>  
	BOOL CFThreadPool<T>::Stop()
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		FTLTRACE(TEXT("CFThreadPool::Stop\n"));

		BOOL bRet = TRUE;
		API_VERIFY(SetEvent(m_hEventStop));
		return bRet;
	}

	template <typename T>
	BOOL CFThreadPool<T>::Pause()
	{
		FTLTRACE(TEXT("CFThreadPool::Pause\n"));
		BOOL bRet = FALSE;
		API_VERIFY(::ResetEvent(m_hEventContinue));
		return bRet;
	}

	template <typename T>
	BOOL CFThreadPool<T>::Resume()
	{
		FTLTRACE(TEXT("CFThreadPool::Resume\n"));
		BOOL bRet = FALSE;
		API_VERIFY(::SetEvent(m_hEventContinue));
		return bRet;
	}

	template <typename T>  
	BOOL CFThreadPool<T>::ClearUndoWork()
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);

		BOOL bRet = TRUE;
		{
			CFAutoLock<CFLockObject> locker(&m_lockWaitingJobs);
			FTLTRACE(TEXT("CFThreadPool::ClearUndoWork, waitingJob Number is %d\n"), m_WaitingJobs.size());
			while (!m_WaitingJobs.empty())
			{
                //release the corresponding semaphore, to make sure it's same value with m_WaitingJobs
// 				DWORD dwResult = WaitForSingleObject(m_hSemaphoreJobToDo, FTL_MAX_THREAD_DEADLINE_CHECK); 
// 				API_VERIFY(dwResult == WAIT_OBJECT_0);

				WaitingJobContainer::iterator iterBegin = m_WaitingJobs.begin();
				CFJobBase<T>* pJob = *iterBegin;
				FTLASSERT(pJob);
				_NotifyJobCancel(pJob);
				pJob->OnCancelJob();
				m_WaitingJobs.erase(iterBegin);
			}
		}
		return bRet;
	}

	template <typename T>  
	BOOL CFThreadPool<T>::_AddJobThread(LONG nThreadNum)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		BOOL bRet = TRUE;
		{
			CFAutoLock<CFLockObject> locker(&m_lockThreads);
			if ((LONG)m_TaskThreads.size() + nThreadNum > m_nMaxNumThreads)
			{
				FTLASSERT(FALSE);
				SetLastError(ERROR_INVALID_PARAMETER);
				bRet = FALSE;
			}
			else
			{
				unsigned int threadId = 0;
				for(LONG i = 0;i < nThreadNum; i++)
				{
                    HANDLE hThread = (HANDLE) _beginthreadex( NULL, 0, JobThreadProc, this, 0, &threadId);
                    FTLASSERT(hThread != NULL);
                    FTLASSERT(m_TaskThreads.find(threadId) == m_TaskThreads.end());
                    m_TaskThreads[threadId] = hThread;

					FTLTRACE(TEXT("CFThreadPool::_AddJobThread, ThreadId=%d(0x%x), CurNumThreads=%d\n"),
						threadId, threadId, m_TaskThreads.size());
				}
				bRet = TRUE;
			}
		}
		return bRet;
	}

	template <typename T>  
	BOOL CFThreadPool<T>::SubmitJob(CFJobBase<T>* pJob, LONG* pOutJobIndex)
	{
		FTLASSERT(NULL != m_hEventStop);
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		BOOL bRet = FALSE;

		//adds job and wakes up a waiting thread
		m_lockWaitingJobs.Lock();
			
		m_nJobIndex++;
		pJob->m_pThreadPool = this;         //access private variable, and assign this
		pJob->m_nJobIndex = m_nJobIndex;	//access private variable, and set the job index

		if (pOutJobIndex)
		{
			*pOutJobIndex = pJob->m_nJobIndex;
		}

		m_WaitingJobs.push_back(pJob);

		m_lockWaitingJobs.UnLock();

		return bRet;	
	}

	template <typename T>  
	FTLINLINE void CFThreadPool<T>::Flush()
	{
		BOOL bRet = FALSE;
		PulseEvent(m_hEventFlushJobs);

		//主线程也加入处理任务
		bool bContinue = true;
		while (bContinue)
		{
			bContinue = _DoJob();
		}
	}

	template <typename T>  
	BOOL CFThreadPool<T>::HadRequestStop() const
	{
		_ASSERT(NULL != m_hEventStop);
		BOOL bRet = (WaitForSingleObject(m_hEventStop, 0) == WAIT_OBJECT_0);
		return bRet;
	}

	template <typename T>
	BOOL CFThreadPool<T>::HadRequestPause() const
	{
		BOOL bRet = (WaitForSingleObject(m_hEventContinue, 0) == WAIT_TIMEOUT);
		return bRet;
	}

	template <typename T>  
	void CFThreadPool<T>::_DestroyPool()
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		BOOL bRet = FALSE;
		API_VERIFY(ClearUndoWork());
		
		SAFE_CLOSE_HANDLE(m_hEventFlushJobs,NULL);
		SAFE_CLOSE_HANDLE(m_hEventContinue,NULL);
		SAFE_CLOSE_HANDLE(m_hEventStop,NULL);
	}

	template <typename T>  
	void CFThreadPool<T>::_DoJobs()
	{
		FUNCTION_BLOCK_TRACE(0);

		while (true)
		{
			HANDLE hWaitHandles[] = 
			{
				m_hEventStop,                 //user stop thread pool
				m_hEventFlushJobs,          //there are waiting jobs
			};

			DWORD dwResult = WaitForMultipleObjects(_countof(hWaitHandles), hWaitHandles, FALSE, INFINITE);

			switch(dwResult)
			{
			case WAIT_OBJECT_0 + 1:
				{
					bool bContine = true;
					while(bContine)
					{
						bContine = _DoJob();
					}
				}
				break;

			default: break;
			}
		}
	}

	template <typename T>
	FTLINLINE bool CFThreadPool<T>::_DoJob()
	{
		BOOL bRet = FALSE;
		//Get a new job from waiting container
		CFJobBase<T>* pJob = nullptr;

		m_lockWaitingJobs.Lock();

		if(!m_WaitingJobs.empty())
		{
			pJob = m_WaitingJobs.front();
			FTLASSERT(pJob);
			m_WaitingJobs.pop_front();

			m_lockWaitingJobs.UnLock();
		}
		else
		{
			m_lockWaitingJobs.UnLock();
		}

		if (pJob)
		{
			InterlockedIncrement(&m_nRunningJobNumber);
			INT nJobIndex = pJob->GetJobIndex();
			FTLTRACE(TEXT("CFThreadPool Begin Run Job %d\n"), nJobIndex);

			//API_VERIFY(pJob->OnInitialize());

			//_NotifyJobBegin(pJob);
			pJob->Run();
			//_NotifyJobEnd(pJob);

			pJob->OnFinalize();

			InterlockedDecrement(&m_nRunningJobNumber);

			FTLTRACE(TEXT("CFThreadPool End Run Job %d\n"), nJobIndex);

			return true;
		}

		return false;
	}

	template <typename T>  
	void CFThreadPool<T>::_NotifyJobBegin(CFJobBase<T>* pJob)
	{
		FTLASSERT(pJob);
		if (pJob && m_pCallBack)
		{
			m_pCallBack->OnJobBegin(pJob->GetJobIndex(), pJob);
		}
	}

	template <typename T>  
	void CFThreadPool<T>::_NotifyJobEnd(CFJobBase<T>* pJob)
	{
		FTLASSERT(pJob);
		if (pJob && m_pCallBack)
		{
			m_pCallBack->OnJobEnd(pJob->GetJobIndex(), pJob);
		}
	}

	template <typename T>  
	void CFThreadPool<T>::_NotifyJobCancel(CFJobBase<T>* pJob)
	{
		FTLASSERT(pJob);
		if (pJob && m_pCallBack)
		{
			m_pCallBack->OnJobCancel(pJob->GetJobIndex(), pJob);
		}
	}

	template <typename T>  
	void CFThreadPool<T>::_NotifyJobProgress(CFJobBase<T>* pJob, LONGLONG nCurPos, LONGLONG nTotalSize)
	{
		FTLASSERT(pJob);
		if (pJob && m_pCallBack)
		{
			m_pCallBack->OnJobProgress(pJob->GetJobIndex(), pJob, nCurPos, nTotalSize);
		}
	}

	template <typename T>  
	void CFThreadPool<T>::_NotifyJobError(CFJobBase<T>* pJob, DWORD dwError, LPCTSTR pszDescription)
	{
		FTLASSERT(pJob);
		if (pJob && m_pCallBack)
		{
			m_pCallBack->OnJobError(pJob->GetJobIndex(), pJob, dwError, pszDescription);
		}
	}

	template <typename T>  
	unsigned int CFThreadPool<T>::JobThreadProc(void *pThis)
	{
		FUNCTION_BLOCK_TRACE(0);
		CFThreadPool<T>* pThreadPool = (CFThreadPool<T>*)pThis;

		pThreadPool->_DoJobs();

		return(0);
	}
}

#endif //FTL_THREADPOOL_HPP