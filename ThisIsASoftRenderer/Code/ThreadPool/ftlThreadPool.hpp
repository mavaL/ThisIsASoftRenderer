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
		m_hEventJobStop = NULL;
		m_dwErrorStatus = ERROR_SUCCESS;
	}
	template <typename T>
	CFJobBase<T>::CFJobBase(T& t)
		:m_JobParam(t)
	{
		m_nJobPriority = 0;
		m_nJobIndex = 0;
		m_pThreadPool = NULL;
		m_hEventJobStop = NULL;
		m_dwErrorStatus = ERROR_SUCCESS;
	}

	template <typename T>
	CFJobBase<T>::~CFJobBase()
	{
		FTLASSERT(NULL == m_hEventJobStop);
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
		return m_strFromatErrorInfo.GetString();
	}

	template <typename T>
	BOOL CFJobBase<T>::RequestCancel()
	{
		BOOL bRet = FALSE;
		API_VERIFY(SetEvent(m_hEventJobStop));
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
		FTLASSERT(NULL == m_hEventJobStop);
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
			m_hEventJobStop,
			m_pThreadPool->m_hEventStop,
			m_pThreadPool->m_hEventContinue
		};
		DWORD dwResult = ::WaitForMultipleObjects(_countof(waitEvent), waitEvent, FALSE, dwMilliseconds);
		switch (dwResult)
		{
		case WAIT_OBJECT_0:
			waitType = ftwtStop;		//Job Stop Event
			break;
		case WAIT_OBJECT_0 + 1:
			waitType = ftwtStop;		//Thread Pool Stop Event
			break;
		case WAIT_OBJECT_0 + 2:
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

		m_hEventAllThreadComplete = ::CreateEvent(NULL, TRUE, TRUE, NULL);
		FTLASSERT(NULL != m_hEventAllThreadComplete);

		m_hEventContinue = ::CreateEvent(NULL, TRUE, TRUE, NULL);
		FTLASSERT(NULL != m_hEventContinue);

		m_hSemaphoreJobToDo = ::CreateSemaphore(NULL, 0, MAXLONG, NULL);
		FTLASSERT(NULL != m_hSemaphoreJobToDo);

// 		m_hSemaphoreSubtractThread = CreateSemaphore(NULL, 0, MAXLONG, NULL);
// 		FTLASSERT(NULL != m_hSemaphoreSubtractThread);
	}

	template <typename T>  
	CFThreadPool<T>::~CFThreadPool(void)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		BOOL bRet = FALSE;
		API_VERIFY(StopAndWait(FTL_MAX_THREAD_DEADLINE_CHECK));
		_DestroyPool();

		FTLASSERT(m_WaitingJobs.empty());
		FTLASSERT(m_DoingJobs.empty());
		FTLASSERT(0 == m_nRunningThreadNum);
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
		API_VERIFY(ResetEvent(m_hEventAllThreadComplete));
		API_VERIFY(SetEvent(m_hEventContinue));
		
		{
			CFAutoLock<CFLockObject>   locker(&m_lockThreads);
			_AddJobThread(m_nMinNumThreads);
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
	BOOL CFThreadPool<T>::StopAndWait(DWORD dwTimeOut /* = FTL_MAX_THREAD_DEADLINE_CHECK */)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		BOOL bRet = TRUE;
		API_VERIFY(Stop());
		API_VERIFY(Wait(dwTimeOut));
		return bRet;
	}

	template <typename T>
	BOOL CFThreadPool<T>::Wait(DWORD dwTimeOut /* = FTL_MAX_THREAD_DEADLINE_CHECK */)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		FTLTRACE(TEXT("CFThreadPool::Wait, dwTimeOut=%d\n"), dwTimeOut);

		BOOL bRet = TRUE;

		DWORD dwResult = WaitForSingleObject(m_hEventAllThreadComplete, dwTimeOut);
		switch (dwResult)
		{
		case WAIT_OBJECT_0: 
            //all thread exit, 
            //NOTE: if m_hEventAllThreadComplete is fired, it means that the last thread WILL exit, 
            //      although there is little difference with thread really exit, but it is not VERY IMPORTANT for here.
			bRet = TRUE;
			break;
		case WAIT_TIMEOUT:
			FTLTRACE(TEXT("!!!CFThreadPool::Wait, Not all thread over in %d millisec\n"), dwTimeOut);
			FTLASSERT(FALSE && TEXT("CFThreadPool::Wait TimeOut"));
			SetLastError(ERROR_TIMEOUT);
			bRet = FALSE;
			break;
		default:
			FTLASSERT(FALSE);
			bRet = FALSE;
			break;
		}

		{
			CFAutoLock<CFLockObject> locker(&m_lockThreads);
            for (TaskThreadContrainer::iterator iter = m_TaskThreads.begin();
                iter != m_TaskThreads.end(); ++iter)
            {
                CloseHandle(iter->second);
            }
            m_TaskThreads.clear();
		}

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
				DWORD dwResult = WaitForSingleObject(m_hSemaphoreJobToDo, FTL_MAX_THREAD_DEADLINE_CHECK); 
				API_VERIFY(dwResult == WAIT_OBJECT_0);

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
		{
			CFAutoLock<CFLockObject> locker(&m_lockWaitingJobs);
			m_nJobIndex++;
			pJob->m_pThreadPool = this;         //access private variable, and assign this
			pJob->m_nJobIndex = m_nJobIndex;	//access private variable, and set the job index

			if (pOutJobIndex)
			{
				*pOutJobIndex = pJob->m_nJobIndex;
			}
			
			m_WaitingJobs.insert(pJob);
			API_VERIFY(ReleaseSemaphore(m_hSemaphoreJobToDo, 1L, NULL));
		}
//		BOOL bb = SwitchToThread();//try to wakes up a waiting thread
// 		{
// 			//when all the threads are running and can add thread then add a new thread
// 			CFAutoLock<CFLockObject> locker(&m_lockThreads);
//             LONG nCurNumThreads = (LONG)m_TaskThreads.size();
// 			FTLASSERT(m_nRunningJobNumber <= nCurNumThreads);
// 			BOOL bNeedMoreThread = (m_nRunningJobNumber == nCurNumThreads) && (nCurNumThreads < m_nMaxNumThreads); 
// 			if (bNeedMoreThread)
// 			{
// 				API_VERIFY(_AddJobThread(1L));
// 			}
// 
// 			FTLTRACE(TEXT("CFThreadPool::SubmitJob, pJob[%d] = %p, m_nRunningJobNumber=%d, m_nCurNumThreads=%d, bNeedMoreThread=%d\n"),
// 				pJob->m_nJobIndex, pJob, m_nRunningJobNumber, m_TaskThreads.size(), bNeedMoreThread);
// 		}

		return bRet;	
	}

	template <typename T>  
	FTLINLINE void CFThreadPool<T>::Flush()
	{
		//TODO: 主线程这时应变为消费者,一起处理
		while (!g_env.jobMgr->IsAllFinished())
		{
			SwitchToThread();
		}
	}

	template <typename T>  
	FTLINLINE BOOL CFThreadPool<T>::IsAllFinished()
	{
		CFAutoLock<CFLockObject> lock2(&m_lockWaitingJobs);
		CFAutoLock<CFLockObject> lock1(&m_lockDoingJobs);

		return m_DoingJobs.empty() && m_WaitingJobs.empty(); 
	}

	template <typename T>  
	BOOL CFThreadPool<T>::CancelJob(LONG nJobIndex)
	{
		FTLTRACE(TEXT("CFThreadPool::CancelJob, JobIndex=%d\n"), nJobIndex);

		if (nJobIndex <= 0 || nJobIndex > m_nJobIndex)
		{
			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}

		BOOL bRet = TRUE;
		BOOL bFoundWaiting = FALSE;
		BOOL bFoundDoing = FALSE;
		{
			//1.find in waiting jobs container, because does not know the priority value, so iterate all the jobs
			//TODO: boost::multi_index ?
			CFAutoLock<CFLockObject> locker(&m_lockWaitingJobs);
			for (WaitingJobContainer::iterator iterWaiting = m_WaitingJobs.begin();
				iterWaiting != m_WaitingJobs.end();
				++iterWaiting)
			{
				if ((*iterWaiting)->GetJobIndex() == nJobIndex)
				{
					//find it, then cancel the waiting job
                    bFoundWaiting = TRUE;
					DWORD dwResult = WaitForSingleObject(m_hSemaphoreJobToDo, INFINITE); //release semaphore to make sure it's same with the job counts
					FTLASSERT(dwResult == WAIT_OBJECT_0);

					CFJobBase<T>* pJob = *iterWaiting;
					FTLASSERT(pJob);
					FTLASSERT(pJob->GetJobIndex() == nJobIndex);

					_NotifyJobCancel(pJob);
					//pJob->m_JobStatus = jsCancel;
					pJob->OnCancelJob();
					
					m_WaitingJobs.erase(iterWaiting);
					break;
				}
			}
		}

		if (!bFoundWaiting)
		{
			//2. find in running jobs -- NOTE: cannot change order between step 1 and step 2.
			CFAutoLock<CFLockObject> locker(&m_lockDoingJobs);
			DoingJobContainer::iterator iterDoing = m_DoingJobs.find(nJobIndex);
			if (iterDoing != m_DoingJobs.end())
			{
				bFoundDoing = TRUE;

				CFJobBase<T>* pJob = iterDoing->second;
				FTLASSERT(pJob);
				FTLASSERT(pJob->GetJobIndex() == nJobIndex);
				
				//Request Cancel
				pJob->RequestCancel();
			}
		}
		if (!bFoundWaiting && !bFoundDoing)
		{
            //3.cannot find in waiting or doing container, it means that the job has already finished
			//do nothing here
		}

		return bRet;
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
		
		SAFE_CLOSE_HANDLE(m_hSemaphoreJobToDo,NULL);
		//SAFE_CLOSE_HANDLE(m_hSemaphoreSubtractThread,NULL);
		SAFE_CLOSE_HANDLE(m_hEventAllThreadComplete, NULL);
		SAFE_CLOSE_HANDLE(m_hEventContinue,NULL);
		SAFE_CLOSE_HANDLE(m_hEventStop,NULL);
	}

	template <typename T>  
	GetJobType CFThreadPool<T>::_GetJob(CFJobBase<T>** ppJob)
	{
		FUNCTION_BLOCK_TRACE(0);
		HANDLE hWaitHandles[] = 
		{
			m_hEventStop,                 //user stop thread pool
			m_hSemaphoreJobToDo,          //there are waiting jobs
			//m_hSemaphoreSubtractThread,   //need subtract thread
		};

		DWORD dwResult = WaitForMultipleObjects(_countof(hWaitHandles), hWaitHandles, FALSE, INFINITE);

		switch(dwResult)
		{
		case WAIT_OBJECT_0:				//m_hEventStop
			return typeStop;
		case WAIT_OBJECT_0 + 1:			//m_hSemaphoreJobToDo
			break;
// 		case WAIT_OBJECT_0 + 2:			//m_hSemaphoreSubtractThread
// 			return typeSubtractThread;
		default:
			FTLASSERT(FALSE);
			return typeStop;
		}

		{
			//Get a new job from waiting container
			CFAutoLock<CFLockObject> lockerWating(&m_lockWaitingJobs);
			FTLASSERT(!m_WaitingJobs.empty());
			WaitingJobContainer::iterator iterBegin = m_WaitingJobs.begin();

			CFJobBase<T>* pJob = *iterBegin;
			FTLASSERT(pJob);

			*ppJob = pJob;
			m_WaitingJobs.erase(iterBegin);
			{
				//push it into doing container
				CFAutoLock<CFLockObject> lockerDoing(&m_lockDoingJobs);
				m_DoingJobs.insert(DoingJobContainer::value_type(pJob->GetJobIndex(), pJob));			
			}
		}
		return typeGetJob;	
	}

	template <typename T>  
	void CFThreadPool<T>::_DoJobs()
	{
		BOOL bRet = FALSE;
		FUNCTION_BLOCK_TRACE(0);
		CFJobBase<T>* pJob = NULL;
		GetJobType getJobType = typeStop;
		while(typeGetJob == (getJobType = _GetJob(&pJob)))
		{
			InterlockedIncrement(&m_nRunningJobNumber);
			INT nJobIndex = pJob->GetJobIndex();
			FTLTRACE(TEXT("CFThreadPool Begin Run Job %d\n"), nJobIndex);

			API_VERIFY(pJob->OnInitialize());
			if (bRet)
			{
				FTLASSERT(NULL == pJob->m_hEventJobStop);
				pJob->m_hEventJobStop = CreateEvent(NULL, TRUE, FALSE, NULL);
				//pJob->m_JobStatus = jsDoing;

				_NotifyJobBegin(pJob);
				pJob->Run();
				_NotifyJobEnd(pJob);

				//if (jsDoing == pJob->m_JobStatus)
				//{
				//	pJob->m_JobStatus = jsDone;
				//}
				SAFE_CLOSE_HANDLE(pJob->m_hEventJobStop, NULL);
				pJob->OnFinalize();
			}
			InterlockedDecrement(&m_nRunningJobNumber);

			FTLTRACE(TEXT("CFThreadPool End Run Job %d\n"), nJobIndex);
			{
				//Job finished, then remove it doing container
				CFAutoLock<CFLockObject> lockerDoing(&m_lockDoingJobs);
				DoingJobContainer::iterator iter = m_DoingJobs.find(nJobIndex);
				FTLASSERT(m_DoingJobs.end() != iter);
				if (m_DoingJobs.end() != iter)
				{
					m_DoingJobs.erase(iter);
				}
			}

			//whether need to subtract a thread
// 			BOOL bNeedSubtractThread = FALSE;
// 			{
// 				CFAutoLock<CFLockObject> lockerWaitingJobs(&m_lockWaitingJobs);
// 				CFAutoLock<CFLockObject> lockerThreads(&m_lockThreads);
//                 //when there is no more jobs in waiting container, and current thread count is bigger than minimum thread count, then will reduce a thread
// 				bNeedSubtractThread = (m_WaitingJobs.empty() && ((LONG)m_TaskThreads.size() > m_nMinNumThreads) && !HadRequestStop());
// 				if (bNeedSubtractThread)
// 				{
// 					ReleaseSemaphore(m_hSemaphoreSubtractThread, 1L, NULL);
// 				}
// 			}
		}
// 		if (typeSubtractThread == getJobType)  
// 		{
//             //Need subtract a thread, then quit self thread
// 			//FUNCTION_BLOCK_NAME_TRACE(TEXT("typeSubtractThread, will remove self thread"), DEFAULT_BLOCK_TRACE_THRESHOLD);
// 			CFAutoLock<CFLockObject> locker(&m_lockThreads);
// 			DWORD dwCurrentThreadId = GetCurrentThreadId();
// 			{
// 				HANDLE hOldTemp = m_TaskThreads[dwCurrentThreadId];
//                 m_TaskThreads.erase(dwCurrentThreadId);
// 				CloseHandle(hOldTemp);
// 
// 				FTLTRACE(TEXT("CFThreadPool Subtract a thread, thread id = %d(0x%x), curThreadNum = %d\n"),
// 					dwCurrentThreadId, dwCurrentThreadId, m_TaskThreads.size());
// 			}
// 		}
// 		else //typeStop
// 		{
// 			//Do Nothing
// 		}
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
		LONG nRunningNumber = InterlockedIncrement(&pThreadPool->m_nRunningThreadNum);
		if (1 == nRunningNumber)
		{
			ResetEvent(pThreadPool->m_hEventAllThreadComplete);
		}

		pThreadPool->_DoJobs();

		nRunningNumber = InterlockedDecrement(&pThreadPool->m_nRunningThreadNum);
		if (0 == nRunningNumber)
		{
			//if this is the last thread, then fire the complete event
			SetEvent(pThreadPool->m_hEventAllThreadComplete);
		}
		return(0);
	}
}

#endif //FTL_THREADPOOL_HPP