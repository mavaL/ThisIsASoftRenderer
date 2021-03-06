/********************************************************************
	created:	2:7:2013   22:35
	filename	Camera.h
	author:		maval

	purpose:	摄像机类.
				注意我们约定该相机初始z轴与世界z轴相反.
*********************************************************************/
#ifndef Camera_h__
#define Camera_h__

#include "Prerequiestity.h"
#include "MathDef.h"

namespace SR
{
	class Camera
	{
	public:
		Camera();

		bool	m_bActive;

	public:
		void	Update();

		void	SetNearClip(float n);
		void	SetFarClip(float f);
		void	SetPosition(const VEC3& pos);
		void	SetDirection(const VEC3& dir);
		void	Yaw(float angle);
		void	SetMoveSpeed(float fSpeed) { m_moveSpeed = fSpeed; }
		void	AddMoveSpeed(float delta);
		float	GetMoveSpeed()	{ return m_moveSpeed; }

		const VEC4&		GetPos() const		{ return m_viewPt;	}
		VEC4			GetDirection() const;
		VEC4			GetRight() const;
		float	GetNearClip() const	{ return m_nearClip; }
		float	GetFarClip() const	{ return m_farClip; }
		float	GetFov() const		{ return m_fov; }
		float	GetAspectRatio() const	{ return m_aspectRatio; }
		float	GetImagePlaneHalfW() const { return m_imagePlane_r; }
		float	GetImagePlaneHalfH() const { return m_imagePlane_t; }

		const MAT44&	GetViewMatrix() const	{ return m_matView; }
		const MAT44&	GetProjMatrix() const	{ return m_matProj; }
		const MAT44&	GetInvViewMatrix() const	{ return m_matInvView; }
		const MAT44&	GetInvProjMatrix() const	{ return m_matInvProj; }

		//对物体进行视锥裁减测试.被剪裁返回true.
		bool	ObjectFrustumCulling(const RenderObject& obj);

		void	_BuildViewMatrix();
		void	_BuildProjMatrix();

	private:
 		VEC4	m_viewPt;

		float	m_nearClip;
		float	m_farClip;
		float	m_fov;			//xz面视野角(弧度值)
		float	m_aspectRatio;
		float	m_imagePlane_r, m_imagePlane_t;	// Half-dimension of image plane

		bool	m_fixYawAxis;	//固定yaw轴为y轴,一般漫游相机这样就够了.飞行模拟类型的不fix,因为需要roll.
		float	m_moveSpeed;

		MAT44	m_matView;
		MAT44	m_matProj;
		MAT44	m_matRot;		//摄像机的世界旋转
		MAT44	m_matInvView;
		MAT44	m_matInvProj;
	};
}

#endif // Camera_h__