#include "stdafx.h"
#include "Camera.h"
#include "Renderer.h"
#include "RenderObject.h"

namespace SR
{
	Camera::Camera()
	:m_viewPt(0, 0, 0, 1)
	,m_nearClip(1)
	,m_farClip(1000)
	,m_fixYawAxis(true)
	,m_moveSpeed(1.0f)
	,m_bActive(false)
	,m_imagePlane_r(-1)
	,m_imagePlane_t(-1)
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
		if(dx) yawDelta = -dx/5.0f;
		if(dy) pitchDelta = -dy/5.0f;

		lastCursorPos = curCursorPos;

		if(!m_bActive)
			return;

		//相机旋转
		if(dx)
		{
			Yaw(yawDelta);
		}
		if(dy)
		{
			MAT44 rotX;
			rotX.FromAxisAngle(VEC3::UNIT_X, pitchDelta);
			//pitch
			m_matRot = Common::Multiply_Mat44_By_Mat44(m_matRot, rotX);
		}

		//相机移动
		VEC4 forward = GetDirection();
		VEC4 right = GetRight();

		forward = Common::Multiply_Vec4_By_K(forward, m_moveSpeed);
		right = Common::Multiply_Vec4_By_K(right, m_moveSpeed);
		
		if(GetAsyncKeyState('W') < 0)		m_viewPt = Add_Vec4_By_Vec4(m_viewPt, forward);
		else if(GetAsyncKeyState('S') < 0)	m_viewPt = Sub_Vec4_By_Vec4(m_viewPt, forward);
		if(GetAsyncKeyState('A') < 0)		m_viewPt = Sub_Vec4_By_Vec4(m_viewPt, right);
		else if(GetAsyncKeyState('D') < 0)	m_viewPt = Add_Vec4_By_Vec4(m_viewPt, right);

		m_viewPt.w = 1;

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
		
		VEC3 zAxis(dir);
		//NB: 相机z轴默认是反的
		zAxis.Neg();
		zAxis.Normalize();

		VEC3 xAxis, yAxis;
		xAxis = Common::CrossProduct_Vec3_By_Vec3(VEC3::UNIT_Y, zAxis);
		xAxis.Normalize();

		yAxis = Common::CrossProduct_Vec3_By_Vec3(zAxis, xAxis);
		yAxis.Normalize();

		m_matRot.FromAxises(xAxis, yAxis, zAxis);
	}

	void Camera::_BuildViewMatrix()
	{
		m_matView = m_matRot.Transpose();
		
		VEC4 trans;
		trans = Common::Transform_Vec4_By_Mat44(m_viewPt, m_matView);
 		m_matView.SetTranslation(VEC4(-trans.x, -trans.y, -trans.z, 1));

		m_matInvView = m_matView.Inverse();
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
		r = m_nearClip*tanf(m_fov/2);
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

		m_matInvProj = m_matProj.Inverse();

		m_imagePlane_r = GetNearClip() * tanf(GetFov() / 2);
		m_imagePlane_t = m_imagePlane_r / GetAspectRatio();
	}

	VEC4 Camera::GetDirection() const
	{
		//NB: 相机初始z轴是反的
		return Common::Transform_Vec3_By_Mat44(VEC3::NEG_UNIT_Z, m_matRot, false);
	}

	VEC4 Camera::GetRight() const	
	{
		return Common::Transform_Vec3_By_Mat44(VEC3::UNIT_X, m_matRot, false);
	}

	bool Camera::ObjectFrustumCulling( const RenderObject& obj )
	{
		const AABB& aabb = obj.m_worldAABB;

		//物体坐标转换到相机空间进行裁减
		VEC4 pos(aabb.GetCenter(), 1.0f);
		Common::Transform_Vec4_By_Mat44(pos, pos, GetViewMatrix());

		float n = GetNearClip();
		float f = GetFarClip();
		float fov = GetFov();
		float half_w = n * std::tan(fov/2);
		float half_h = half_w / GetAspectRatio();

		//检测前后面
		if(-pos.z+aabb.m_boundingRadius <= n || -pos.z-aabb.m_boundingRadius >= f)
			return true;

		//检测左右面
		float planeX = half_w * pos.z / -n;
		if(pos.x - planeX >= aabb.m_boundingRadius ||
			pos.x + aabb.m_boundingRadius <= -planeX)
			return true;

		//检测上下面
		float planeY = half_h * pos.z / -n;
		if(pos.y - planeY >= aabb.m_boundingRadius ||
			pos.y + aabb.m_boundingRadius <= -planeY)
			return true;

		return false;
	}

	void Camera::Yaw( float angle )
	{
		MAT44 rot;
		rot.FromAxisAngle(VEC3::UNIT_Y, angle);
		if(m_fixYawAxis)	m_matRot = Common::Multiply_Mat44_By_Mat44(rot, m_matRot);
		else				m_matRot = Common::Multiply_Mat44_By_Mat44(m_matRot, rot);
	}

	void Camera::AddMoveSpeed( float delta )
	{
		m_moveSpeed += delta;
		m_moveSpeed = Ext::Clamp(m_moveSpeed, 0.1f, 10.0f);
	}

	void Camera::SetNearClip( float n )
	{
		m_nearClip = n;
		_BuildProjMatrix();
	}

	void Camera::SetFarClip( float f )
	{
		m_farClip = f;
		_BuildProjMatrix();
	}
}

