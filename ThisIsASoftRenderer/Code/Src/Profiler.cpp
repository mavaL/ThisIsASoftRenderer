#include "stdafx.h"
#include "Profiler.h"
#include "RenderUtil.h"
#include "Renderer.h"

namespace Ext
{
	Profiler::Profiler()
	{
		// 20 should enough
		m_vecMipColor.resize(20);
		for (int i=0; i<20; ++i)
		{
			m_vecMipColor[i].r = (rand() % 256) / 255.0f;
			m_vecMipColor[i].g = (rand() % 256) / 255.0f;
			m_vecMipColor[i].b = (rand() % 256) / 255.0f;
			m_vecMipColor[i].a = 1.0f;
		}
	}

	Profiler::~Profiler()
	{
	}

	void Profiler::DisplayHelpInfo()
	{
		{
			const VEC4& pos = g_env.renderer->m_camera.GetPos();
			const float speed = g_env.renderer->m_camera.GetMoveSpeed();
			char szText[128];
			sprintf_s(szText, ARRAYSIZE(szText), "CamPos : (%f, %f, %f) Camera Speed: %f .", pos.x, pos.y, pos.z, speed);

			SR::RenderUtil::DrawText(10, 10, szText, 0xff00ff00);
		}

		{
			char szText[128];
			sprintf_s(szText, ARRAYSIZE(szText), 
				"Press \"+/-\" to change camera speed. Press T to toggle scene !");

			SR::RenderUtil::DrawText(10, 35, szText, 0xff00ff00);
		}

#if USE_PROFILER == 1
		{
			char szText[128];
			sprintf_s(szText, ARRAYSIZE(szText), "lastFPS : %d, RenderedTris : %d, Culled Object : %d, Backface : %d, Culled Face : %d", 
				m_frameStatics.lastFPS, m_frameStatics.nRenderedFace, m_frameStatics.nObjCulled, 
				m_frameStatics.nBackFace, m_frameStatics.nFaceCulled);

			SR::RenderUtil::DrawText(10, 60, szText, 0xff00ff00);
		}

		{
			char szText[128];
			int i = 0;
			for (auto iter=m_frameStatics.threadStatMap.begin(); iter!=m_frameStatics.threadStatMap.end(); ++iter,++i)
			{
				const SThreadStat& stat = iter->second;
				sprintf_s(szText, ARRAYSIZE(szText), "Thread%d : %d Tris,   %d Fragment", i, stat.nRenderedTri, stat.nRenderedPixel);
				
				SR::RenderUtil::DrawText(0, SCREEN_HEIGHT-25.0f*(i+1), szText, 0xffff0000);
			}
		}
#endif
	}
}