#include "stdafx.h"
#include "Camera.h"
#include "Renderer.h"

namespace SR
{
	Camera::Camera()
	:m_viewPt(0, 0, 0, 1)
	,m_nearClip(1)
	,m_farClip(100)
	,m_fixYawAxis(true)
	{
		m_fov = Common::Angle_To_Radian(45);
		m_aspectRatio = SCREEN_WIDTH / (float)SCREEN_HEIGHT;
	}

	void Camera::Update()
	{
		//更新输入
		POINT curCursorPos;
		GetCursorPos(&curCursorPos);
		static POINT lastCursorPos = curCursorPos;

		long dx = curCursorPos.x - lastCursorPos.x;
		long dy = curCursorPos.y - lastCursorPos.y;

		float yawDelta = 0, pitchDelta = 0;
		if(dx) yawDelta = (float)dx/5;
		if(dy) pitchDelta = (float)dy/5;

		lastCursorPos = curCursorPos;

		//相机旋转
		if(dx || dy)
		{
			MAT44 rotY, rotX;
			rotY.FromAxisAngle(VEC3::UNIT_Y, yawDelta);
			rotX.FromAxisAngle(VEC3::UNIT_X, pitchDelta);
			//yaw
			if(m_fixYawAxis) m_matRot = Common::Multiply_Mat44_By_Mat44(rotY, m_matRot);
			else			m_matRot = Common::Multiply_Mat44_By_Mat44(m_matRot, rotY);
			//pitch
			m_matRot = Common::Multiply_Mat44_By_Mat44(m_matRot, rotX);
		}

		//相机移动
		VEC4 offset(0, 0, 0, 1);
		const VEC4 forward = GetDirection();
		const VEC4 right = GetRight();
		
		if(GetAsyncKeyState('W') < 0)		offset = Add_Vec4_By_Vec4(offset, forward);
		else if(GetAsyncKeyState('S') < 0)	offset = Sub_Vec4_By_Vec4(offset, forward);
		if(GetAsyncKeyState('A') < 0)		offset = Sub_Vec4_By_Vec4(offset, right);
		else if(GetAsyncKeyState('D') < 0)	offset = Add_Vec4_By_Vec4(offset, right);

		m_viewPt = Common::Add_Vec4_By_Vec4(m_viewPt, offset);
		m_viewPt.w = 1.0f;

		_BuildViewMatrix();
		_BuildProjMatrix();
	}

	void Camera::SetPosition( const VEC3& pos )
	{
		m_viewPt = VEC4(pos, 1);
	}

	void Camera::SetDirection( const VEC3& dir )
	{
		assert(m_fixYawAxis && "Error! Currently only support fix yaw axis mode..");
		
		VEC3 zAxis = dir;
		zAxis.Normalize();

		VEC3 xAxis, yAxis;
		xAxis = Common::CrossProduct_Vec3_By_Vec3(zAxis, VEC3::UNIT_Y);
		xAxis.Normalize();

		yAxis = Common::CrossProduct_Vec3_By_Vec3(xAxis, zAxis);
		yAxis.Normalize();

		m_matRot.FromAxises(xAxis, yAxis, zAxis);
	}

	void Camera::_BuildViewMatrix()
	{
		m_matView = m_matRot.Transpose();
		m_matView.SetTranslation(VEC4(-m_viewPt.x, -m_viewPt.y, -m_viewPt.z, m_viewPt.w));
	}

	void Camera::_BuildProjMatrix()
	{
		/*	这是第一个版本的透视投影矩阵,即"蛮干"求解法.
			它的缺陷在于为了变换出xy坐标都在[-1, 1]的CVV空间,
			必须满足以下条件:
			1.视距d=1.
			2.fov=90度.
			3.视口AspectRatio=1.
			否则变换不出CVV.

			MatProj = ( 1, 0,	0,	0
						0, 1,	0,	0
						0,  0,	1,	0
						0,  0, 1/d,  0 )

			投影变换加齐次除法后:
			x' = x * d / z, y' = y * d / z	*/

		//普适版投影矩阵.推导见: http://blog.csdn.net/popy007/article/details/1797121
		float r,l,t,b;
		r = m_nearClip*std::tan(m_fov/2);
		l = -r;
		t = r/m_aspectRatio;
		b= -t;

		m_matProj.m00 = 2*m_nearClip/(r-l);
		m_matProj.m01 = 0;
		m_matProj.m02 = (r+l)/(r-l);
		m_matProj.m03 = 0;

		m_matProj.m10 = 0;
		m_matProj.m11 = 2*m_nearClip/(t-b);
		m_matProj.m12 = (t+b)/(t-b);
		m_matProj.m13 = 0;

		m_matProj.m20 = 0;
		m_matProj.m21 = 0;
		m_matProj.m22 = -(m_farClip+m_nearClip)/(m_farClip-m_nearClip);
		m_matProj.m23 = -2*m_farClip*m_nearClip/(m_farClip-m_nearClip);

		m_matProj.m30 = 0;
		m_matProj.m31 = 0;
		m_matProj.m32 = -1;
		m_matProj.m33 = 0;
	}

	const VEC4 Camera::GetDirection() const
	{
		//NB: 相机初始z轴是反的
		return Common::Transform_Vec3_By_Mat44(VEC3::NEG_UNIT_Z, m_matRot, false);
	}

	const VEC4 Camera::GetRight() const	
	{
		return Common::Transform_Vec3_By_Mat44(VEC3::UNIT_X, m_matRot, false);
	}

}

