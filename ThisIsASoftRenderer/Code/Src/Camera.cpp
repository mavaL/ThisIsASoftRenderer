#include "stdafx.h"
#include "Camera.h"

namespace SR
{
	Camera::Camera()
	:m_viewPt(0, 0, 0, 1)
	,m_nearClip(1)
	,m_farClip(100)
	,m_fov(90)
	{
		
	}

	void Camera::Update()
	{
		//更新输入
		POINT curCursorPos;
		GetCursorPos(&curCursorPos);
		static POINT lastCursorPos = curCursorPos;

		long dx = curCursorPos.x - lastCursorPos.x;
		long dy = curCursorPos.y - lastCursorPos.y;

		//yaw
		if(dx)
		{
			Common::SMatrix44 matRot;
			matRot.FromAxisAngle(Common::SVector3(0,1,0), (float)dx/10);
			//世界y轴
			m_matCamWorld = Common::Multiply_Mat44_By_Mat44(matRot, m_matCamWorld);
		}
		//pitch
		if(dy)
		{
			Common::SMatrix44 matRot;
			matRot.FromAxisAngle(Common::SVector3(1,0,0), (float)dy/10);
			//local x轴
			m_matCamWorld = Common::Multiply_Mat44_By_Mat44(m_matCamWorld, matRot);
		}

		lastCursorPos = curCursorPos;

		Common::SVector4 offset(0, 0, 0, 1);
		if(GetAsyncKeyState('W') < 0)		offset.z -= 0.5f;
		else if(GetAsyncKeyState('S') < 0)	offset.z += 0.5f;
		if(GetAsyncKeyState('A') < 0)		offset.x -= 0.5f;
		else if(GetAsyncKeyState('D') < 0)	offset.x += 0.5f;

		offset = Common::Transform_Vec4_By_Mat44(offset, m_matCamWorld);
		m_viewPt = Common::Add_Vec4_By_Vec4(m_viewPt, offset);
		m_viewPt.w = 1.0f;

		//更新相机矩阵
		m_matView = m_matCamWorld.Transpose();
		m_matView.SetTranslation(Common::SVector4(-m_viewPt.x, -m_viewPt.y, -m_viewPt.z, m_viewPt.w));
	}
}

