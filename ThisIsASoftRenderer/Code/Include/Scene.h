/********************************************************************
	created:	6:8:2013   22:00
	filename	Scene.h
	author:		maval

	purpose:	渲染场景简单封装
*********************************************************************/
#ifndef Scene_h__
#define Scene_h__

#include "Prerequiestity.h"
#include "RenderObject.h"

namespace SR
{
	class Scene
	{
	public:
		typedef std::function<void(Scene*)>	SetupFunc;
		typedef std::function<void()>		EnterFunc;

	public:
		Scene(SetupFunc& setupFunc, EnterFunc& enterFunc)
		:m_bSetup(false),m_setupFunc(setupFunc),m_enterFunc(enterFunc) {}

		RenderList	m_renderList;	//场景中所有渲染物体

	public:
		void	Enter()
		{
			if(!m_bSetup)
			{
				m_setupFunc(this);
				m_bSetup = true;
			}
			m_enterFunc();
		}

	private:
		SetupFunc	m_setupFunc;
		EnterFunc	m_enterFunc;
		bool		m_bSetup;
	};
}

#endif // Scene_h__