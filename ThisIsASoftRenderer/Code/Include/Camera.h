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
	struct SRenderObj;

	class Camera
	{
	public:
		Camera();

	public:
		void	Update();

		void	SetPosition(const VEC3& pos);
		void	SetDirection(const VEC3& dir);
		void	Yaw(float angle);
		void	AddMoveSpeed(float delta);
		inline float	GetMoveSpeed()	{ return m_moveSpeed; }

		inline const VEC4&		GetPos() const		{ return m_viewPt;	}
		VEC4			GetDirection() const;
		VEC4			GetRight() const;
		inline float	GetNearClip() const	{ return m_nearClip; }
		inline float	GetFarClip() const	{ return m_farClip; }
		inline float	GetFov() const		{ return m_fov; }
		inline float	GetAspectRatio() const	{ return m_aspectRatio; }

		inline const MAT44&	GetViewMatrix() const	{ return m_matView; }
		inline const MAT44&	GetProjMatrix() const	{ return m_matProj; }

		//对物体进行视锥裁减测试.被剪裁返回true.
		bool	ObjectFrustumCulling(const SRenderObj& obj);

	private:
		void	_BuildViewMatrix();
		void	_BuildProjMatrix();

	private:
 		VEC4	m_viewPt;

		float	m_nearClip;
		float	m_farClip;
		float	m_fov;			//xz面视野角(弧度值)
		float	m_aspectRatio;

		bool	m_fixYawAxis;	//固定yaw轴为y轴,一般漫游相机这样就够了.飞行模拟类型的不fix,因为需要roll.
		float	m_moveSpeed;

		MAT44	m_matView;
		MAT44	m_matProj;
		MAT44	m_matRot;		//摄像机的世界旋转
	};
}

#endif // Camera_h__