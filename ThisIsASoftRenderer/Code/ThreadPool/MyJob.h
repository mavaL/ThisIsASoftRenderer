/********************************************************************
	created:	12:8:2013   7:34
	filename	MyJob.h
	author:		maval

	purpose:	多线程渲染任务划分
*********************************************************************/
#ifndef MyJob_h__
#define MyJob_h__

#include "Prerequiestity.h"
#include "ThreadPool/ftlFake.h"
#include "ThreadPool/ftlThreadPool.h"
#include "MathDef.h"
#include "GeometryDef.h"

namespace SR
{
	/////////////////////////////////////////////////////
	////// VS
	struct JobParamVS
	{
		RenderObject* object;
	};

	class JobVS : public CFJobBase<void*>
	{
		DISABLE_COPY_AND_ASSIGNMENT(JobVS);
	public:
		JobVS(void* param):CFJobBase(param) {}
	public:
		//overload for CFJobBase
		virtual BOOL OnInitialize();
		virtual BOOL Run();
		virtual VOID OnFinalize();
		virtual void OnCancelJob();
	};

	/////////////////////////////////////////////////////
	////// RS
	struct JobParamRS
	{
		SVertex			v0,v1,v2;
		eTriangleShape	triType;
		SMaterial*		pMaterial;
		int				texLod;
	};

	class JobRS : public CFJobBase<void*>
	{
		DISABLE_COPY_AND_ASSIGNMENT(JobRS);
	public:
		JobRS(void* param):CFJobBase(param) {}
	public:
		//overload for CFJobBase
		virtual BOOL OnInitialize();
		virtual BOOL Run();
		virtual VOID OnFinalize();
		virtual void OnCancelJob();
	};

	/////////////////////////////////////////////////////
	////// PS
	struct JobParamPS
	{
		SFragment* frag;
	};

	class JobPS : public CFJobBase<void*>
	{
		DISABLE_COPY_AND_ASSIGNMENT(JobPS);
	public:
		JobPS(void* param):CFJobBase(param) {}
	public:
		//overload for CFJobBase
		virtual BOOL OnInitialize();
		virtual BOOL Run();
		virtual VOID OnFinalize();
		virtual void OnCancelJob();
	};
}


#endif // MyJob_h__