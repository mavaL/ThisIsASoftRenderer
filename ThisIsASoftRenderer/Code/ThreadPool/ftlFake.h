//Fishjam Template Library

#pragma once

#ifndef FTL_FAKE_H
#define FTL_FAKE_H

#ifdef FTL_BASE_H
#  error ftlfake.h can't use any other SR file
#endif 

#define FTL_BASE_H

#if defined ATLTRACE
# define FTLTRACE           ATLTRACE
# define FTLASSERT          ATLASSERT

#elif defined TRACE
# define FTLTRACE           TRACE
# define FTLASSERT          ASSERT
#else
#  error must have ATLTRACE or (MFC)TRACE
#endif

#include <functional>

namespace SR
{

#define DISABLE_COPY_AND_ASSIGNMENT(className)  \
private:\
	className(const className& ref);\
	className& operator = (const className& ref)

    #ifndef _countof
    #  define _countof(arr) (sizeof(arr) / sizeof(arr[0]))
    #endif

    #define API_VERIFY(x)   \
        bRet = (x); \
        FTLASSERT(TRUE == bRet);

	# define API_VERIFY_EXCEPT1(x,e1)\
		bRet = (x);\
		if(FALSE == bRet)\
		{\
			DWORD dwLastError = GetLastError();\
			if(dwLastError != e1)\
			{\
				FTLASSERT(FALSE && TEXT("Error Happend"));\
			}\
			SetLastError(dwLastError);\
		}

    #define COM_VERIFY(x)   \
        hr = (x); \
        FTLASSERT(SUCCEEDED(hr));

    #define DX_VERIFY(x)   \
        hr = (x); \
        FTLASSERT(SUCCEEDED(hr));

    #ifndef SAFE_FREE_BSTR
    #  define SAFE_FREE_BSTR(s) if(NULL != (s) ){ ::SysFreeString((s)); (s) = NULL; }
    #endif

    #ifndef SAFE_RELEASE
    #  define SAFE_RELEASE(p)  if( NULL != (p) ){ (p)->Release(); (p) = NULL; }
    #endif 

	#ifndef SAFE_DELETE
	#  define SAFE_DELETE(p)	if(NULL != (p) ) { delete p; p = NULL; }
	#endif

	#ifndef SAFE_DELETE_ARRAY
	#  define SAFE_DELETE_ARRAY(p) if( NULL != (p) ){ delete [] (p); (p) = NULL; }
	#endif


	#define FTL_MIN(a,b)                (((a) < (b)) ? (a) : (b))

    #define CHECK_POINTER_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p)\
        {\
            FTLASSERT(NULL != p);\
            return r;\
        }

	#ifndef SAFE_CLOSE_INTERNET_HANDLE
	#  ifdef FTL_DEBUG
	#    define SAFE_CLOSE_INTERNET_HANDLE(h) if(NULL != (h)) { BOOL oldbRet = bRet; API_VERIFY(::InternetCloseHandle((h))); (h) = NULL; bRet = oldbRet; }
	#  else
	#    define SAFE_CLOSE_INTERNET_HANDLE(h) if((NULL) != (h)) { ::InternetCloseHandle((h)); (h) = NULL; bRet = bRet; }
	#  endif
	#endif

#define COMPARE_MEM_LESS(f, o) \
	if( f < o.f ) { return true; }\
		else if( f > o.f ) { return false; }

#  define FTLEXPORT
#  define FTLINLINE inline

#  define DEFAULT_BLOCK_TRACE_THRESHOLD  (100)
#define FUNCTION_BLOCK_TRACE(t)			void(t)

	template <typename T>
	struct UnreferenceLess : public std::binary_function<T, T, bool>
	{
		bool operator()(const T& _Left, const T& _Right) const
		{
			return (*_Left < *_Right);
		}
	};

	template<typename T = CFLockObject>
	class CFAutoLock
	{
	public:
		explicit CFAutoLock<T>(T* pLockObj)
		{
			FTLASSERT(pLockObj);
			m_pLockObj = pLockObj;
			m_pLockObj->Lock(INFINITE);
		}
		~CFAutoLock()
		{
			m_pLockObj->UnLock();
		}
	private:
		T*   m_pLockObj;
	};


#    define SAFE_CLOSE_HANDLE(h,v) if((v) != (h)) { ::CloseHandle((h)); (h) = (v); bRet = bRet; }
//#  define SAFE_DELETE_ARRAY(p) if( NULL != (p) ){ delete [] (p); (p) = NULL; }

#ifndef QQUOTE
#  define    QUOTE(x)        #x
#  define    QQUOTE(y)       QUOTE(y)
#endif //QQUOTE

#define TODO(desc) message(__FILE__ "(" QQUOTE(__LINE__) ") : TODO: " #desc)

}
#endif //FTL_FAKE_H