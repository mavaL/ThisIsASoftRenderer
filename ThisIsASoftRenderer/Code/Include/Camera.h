/********************************************************************
	created:	2:7:2013   22:35
	filename	Camera.h
	author:		maval

	purpose:	摄像机类.
				注意我们约定该相机初始z轴与世界z轴相反.
*********************************************************************/
#ifndef Camera_h__
#define Camera_h__

#include "MathDef.h"

namespace SR
{
	class Camera
	{
	public:
		Camera();

	public:
		void	Update();

		void	SetPosition(const VEC3& pos);
		void	SetDirection(const VEC3& dir);
		const VEC4 GetDirection() const;
		const VEC4 GetRight() const;

		const MAT44&	GetViewMatrix() const	{ return m_matView; }
		const MAT44&	GetProjMatrix() const	{ return m_matProj; }
		const VEC4&		GetViewPt() const		{ return m_viewPt;	}

	private:
		void	_BuildViewMatrix();
		void	_BuildProjMatrix();

	private:
 		VEC4	m_viewPt;

		float	m_nearClip;
		float	m_farClip;
		float	m_fov;			//视野角(弧度值)
		float	m_aspectRatio;
		bool	m_fixYawAxis;	//固定yaw轴为y轴,一般漫游相机这样就够了.飞行模拟类型的不fix,因为需要roll.

		MAT44	m_matView;
		MAT44	m_matProj;
		MAT44	m_matRot;		//摄像机的世界旋转
	};
}

#endif // Camera_h__