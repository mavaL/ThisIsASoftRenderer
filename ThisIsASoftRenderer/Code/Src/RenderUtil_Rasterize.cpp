#include "stdafx.h"
#include "RenderUtil.h"
#include "Renderer.h"
#include "PixelBox.h"
#include "RenderObject.h"
#include "Profiler.h"
#include "ThreadPool/MyJob.h"

namespace SR
{
	void RenderUtil::DrawLine_DDA( int x0, int y0, int x1,int y1, const SColor& color )
	{
		int lerp_dx = x1 - x0;
		int lerp_dy = y1 - y0;

		if(!ClipLine(x0, y0, x1, y1))
			return;

		int dx = x1 - x0;
		int dy = y1 - y0;

		PixelBox* pBackBuffer = g_env.renderer->m_backBuffer.get();
		DWORD* destBuffer = (DWORD*)pBackBuffer->GetDataPointer() + y0 * pBackBuffer->GetWidth() + x0;
		const DWORD iColor = color.GetAsInt();

		if (dx == 0)		// Vertical line
		{
			int inc_y = dy > 0 ? 1 : -1;
			float dt = 1.0f / abs(lerp_dy);
			for (int y=y0; y!=y1; y+=inc_y)
			{
				*destBuffer = iColor;
				destBuffer += pBackBuffer->GetWidth() * inc_y;
			}
		}
		else if (dy == 0)	// Horizontal line
		{
			int inc_x = dx > 0 ? 1 : -1;
			float dt = 1.0f / abs(lerp_dx);
			for (int x=x0; x!=x1; x+=inc_x)
			{
				*destBuffer = iColor;
				destBuffer += inc_x;
			}
		}
		else
		{
			float k = abs(lerp_dy/(float)lerp_dx);
			float inc_x, inc_y;
			if (dx >= 0)
			{
				inc_x = 1.0f;
			} 
			else
			{
				inc_x = -1.0f;
				dx = -dx;
			}
			if (dy >= 0)
			{
				inc_y= 1.0f;
			} 
			else
			{
				inc_y = -1.0f;
				dy = -dy;
			}

			float dist = sqrt((float)dx*dx+dy*dy);

			inc_y *= k;
			float y = 0;
			int x = 0;
			for (int i=0; i<dx; ++i)
			{
				destBuffer[(int)y*pBackBuffer->GetWidth()+x] = iColor;

				y += inc_y;
				x += (int)inc_x;
			}
		}
	}

	bool RenderUtil::ClipLine( int& x1, int& y1, int& x2, int& y2 )
	{
		// Left edge
		if (x1 < min_clip_x && x2 < min_clip_x)
		{
			return false;
		} 
		else if(x1 >= min_clip_x && x2 >= min_clip_x)
		{
		}
		else		// Need clip
		{
			float t = (min_clip_x-x1)/(float)(x2-x1);
			float intersectY = y1 + (y2-y1)*t;
			if (x1 < x2)	//交点取代pt1
			{
				x1 = min_clip_x;
				y1 = Ext::Ftoi32_Fast(intersectY);
			} 
			else			//交点取代pt2
			{
				x2 = min_clip_x;
				y2 = Ext::Ftoi32_Fast(intersectY);
			}
		}
		// Bottom edge
		if (y1 >= max_clip_y && y2 >= max_clip_y)
		{
			return false;
		} 
		else if(y1 < max_clip_y && y2 < max_clip_y)
		{
		}
		else		// Need clip
		{
			float t = (max_clip_y-y1)/(float)(y2-y1);
			float intersectX = x1 + (x2-x1)*t;
			if (y1 < y2)	//交点取代pt2
			{
				x2 = Ext::Ftoi32_Fast(intersectX);
				y2 = max_clip_y;
			} 
			else			//交点取代pt1
			{
				x1 = Ext::Ftoi32_Fast(intersectX);
				y1 = max_clip_y;
			}
		}
		// Right edge
		if (x1 >= max_clip_x && x2 >= max_clip_x)
		{
			return false;
		} 
		else if(x1 < max_clip_x && x2 < max_clip_x)
		{
		}
		else		// Need clip
		{
			float t = (max_clip_x-x1)/(float)(x2-x1);
			float intersectY = y1 + (y2-y1)*t;
			if (x1 < x2)	//交点取代pt2
			{
				x2 = max_clip_x;
				y2 = Ext::Ftoi32_Fast(intersectY);
			} 
			else			//交点取代pt1
			{
				x1 = max_clip_x;
				y1 = Ext::Ftoi32_Fast(intersectY);
			}
		}
		// Top edge
		if (y1 < min_clip_y && y2 < min_clip_y)
		{
			return false;
		} 
		else if(y1 >= min_clip_y && y2 >= min_clip_y)
		{
		}
		else		// Need clip
		{
			float t = (min_clip_y-y1)/(float)(y2-y1);
			float intersectX = x1 + (x2-x1)*t;
			if (y1 < y2)	//交点取代pt1
			{
				x1 = Ext::Ftoi32_Fast(intersectX);
				y1 = min_clip_y;
			} 
			else			//交点取代pt2
			{
				x2 = Ext::Ftoi32_Fast(intersectX);
				y2 = min_clip_y;
			}
		}
		return true;
	}

	bool RenderUtil::PreDrawTriangle( const SVertex*& vert0, const SVertex*& vert1, const SVertex*& vert2, eTriangleShape& retType )
	{
		int x0 = Ext::Ceil32_Fast(vert0->pos.x);
		int x1 = Ext::Ceil32_Fast(vert1->pos.x);
		int x2 = Ext::Ceil32_Fast(vert2->pos.x);
		int y0 = Ext::Ceil32_Fast(vert0->pos.y);
		int y1 = Ext::Ceil32_Fast(vert1->pos.y);
		int y2 = Ext::Ceil32_Fast(vert2->pos.y);

		//该三角面不在裁剪区域内,不绘制
		if(	(x0 < min_clip_x && x1 < min_clip_x && x2 < min_clip_x) ||
			(x0 > max_clip_x && x1 > max_clip_x && x2 > max_clip_x) ||
			(y0 < min_clip_y && y1 < min_clip_y && y2 < min_clip_y) ||
			(y0 > max_clip_y && y1 > max_clip_y && y2 > max_clip_y) )
			return false;

		//该三角面退化成了直线,不绘制
		if(	(x0 == x1 && x1 == x2) ||
			(y0 == y1 && y1 == y2)	)
			return false;

		/* 1.使3个顶点成为下面的布局:
				p0		
				|\		
				| \		
				|  \ p2 
				|  /	
				| /		
				|/		
				p1				*/
		if(y0 > y1)
		{
			Ext::Swap(vert0, vert1);
			Ext::Swap(y0, y1);
		}
		if(y0 > y2)
		{
			Ext::Swap(vert0, vert2);
			Ext::Swap(y0, y2);
		}
		if(y2 > y1)
		{
			Ext::Swap(vert2, vert1);
			Ext::Swap(y2, y1);
		}

		/* 2.若三角面是平底三角形
					p0
					/\
				   /  \
				  /____\
				 p1		p2
		*/
		if(y1 == y2)
		{
			retType = eTriangleShape_Bottom;
		}

		/* 3.若三角面是平顶三角形
				 p0______p2
				   \    /
					\  /
					 \/
					 p1
		*/
		else if(y0 == y2)
		{
			retType = eTriangleShape_Top;
		}

		/* 4.否则是平凡三角形.需要切割成平底三角形和平顶三角形.
					p0
				    |\
				    | \				x1 - x0	  x3 - x0
				 p3 |__\ p2			------- = -------
					|  /			y1 - y0	  y2 - y0
					| /	
					|/
					p1			*/
		else
		{
			retType = eTriangleShape_General;
		}

		return true;
	}

	void RenderUtil::DrawTriangle_Scanline( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, SColor color )
	{
		eTriangleShape triType;
		if(!PreDrawTriangle(vert0, vert1, vert2, triType))
			return;

		float x0, x1, x2, y0, y1, y2;
		x0 = vert0->pos.x;
		x1 = vert1->pos.x;
		x2 = vert2->pos.x;
		y0 = vert0->pos.y;
		y1 = vert1->pos.y;
		y2 = vert2->pos.y;

		switch (triType)
		{
		case eTriangleShape_Bottom:		DrawBottomTri_Scanline(x0, y0, x1, y1, x2, y2, color); break;
		case eTriangleShape_Top:		DrawTopTri_Scanline(x0, y0, x1, y1, x2, y2, color); break;
		case eTriangleShape_General:	
			{
				float x3 = x0 + (y2-y0) * (x1-x0) / (y1-y0);
				DrawBottomTri_Scanline(x0, y0, x3, y2, x2, y2, color);
				DrawTopTri_Scanline(x3, y2, x1, y1, x2, y2, color);
			}
			break;
		default: assert(0);
		}
	}

	void RenderUtil::DrawBottomTri_Scanline( float x0, float y0, float x1, float y1, float x2, float y2, SColor color )
	{
		//NB: 为了填充规则,要保持x坐标有序
		if(x1 > x2)
		{
			Ext::Swap(x1, x2);
			Ext::Swap(y1, y2);
		}

		assert(Ext::Ceil32_Fast(y1) == Ext::Ceil32_Fast(y2));

		//左上填充规则
		int curY = Ext::Ceil32_Fast(y0);
		int endY = Ext::Ceil32_Fast(y1) - 1;
		float curLX = x0, curRX = x0;
		float left_incX = (x1-x0)/(y1-y0);
		float right_incX = (x2-x0)/(y2-y0);

		while (curY <= y1)
		{
			//左上填充规则
			int lineX0 = Ext::Ceil32_Fast(curLX);
			int lineX1 = Ext::Ceil32_Fast(curRX) - 1;

			if(lineX1 - lineX0 >= 0)
			{
				RenderUtil::DrawLine_DDA(lineX0, curY, lineX1, curY, color);
			}

			++curY;
			curLX += left_incX;
			curRX += right_incX;
		}

#if USE_PROFILER == 1
		g_env.profiler->AddRenderedFace();
#endif
	}

	void RenderUtil::DrawTopTri_Scanline( float x0, float y0, float x1, float y1, float x2, float y2, SColor color )
	{
		//NB: 为了填充规则,要保持x坐标有序
		if(x0 > x2)
		{
			Ext::Swap(x0, x2);
			Ext::Swap(y0, y2);
		}

		assert(Ext::Ceil32_Fast(y0) == Ext::Ceil32_Fast(y2));

		//左上填充规则
		int curY = Ext::Ceil32_Fast(y0);
		int endY = Ext::Ceil32_Fast(y1) - 1;
		float curLX = x0, curRX = x2;
		float left_incX = (x1-x0)/(y1-y0);
		float right_incX = (x1-x2)/(y1-y2);

		while (curY <= endY)
		{
			//左上填充规则
			int lineX0 = Ext::Ceil32_Fast(curLX);
			int lineX1 = Ext::Ceil32_Fast(curRX) - 1;

			if(lineX1 - lineX0 >= 0)
			{
				RenderUtil::DrawLine_DDA(lineX0, curY, lineX1, curY, color);
			}

			++curY;
			curLX += left_incX;
			curRX += right_incX;
		}

#if USE_PROFILER == 1
		g_env.profiler->AddRenderedFace();
#endif
	}

	void RenderUtil::DrawTriangle_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, const SRenderContext& context )
	{
		eTriangleShape triType;
		if(!PreDrawTriangle(vert0, vert1, vert2, triType))
			return;

		switch (triType)
		{
		case eTriangleShape_Bottom: 
			{
				DrawBottomTri_Scanline_V2(vert0, vert1, vert2, context);
			}
			break;

		case eTriangleShape_Top: 
			{
				DrawTopTri_Scanline_V2(vert0, vert1, vert2, context);
			}
			break;

		case eTriangleShape_General:	
			{
				//创建切割生成的新顶点
				SVertex vert3;
				float y0 = vert0->pos.y;
				float y1 = vert1->pos.y;
				float y2 = vert2->pos.y;
				//插值各属性
				float t = (y2-y0)/(y1-y0);
				Ext::LinearLerp(vert3.pos, vert0->pos, vert1->pos, t);
#if USE_PERSPEC_CORRECT == 1
				g_env.renderer->GetCurRas()->LerpVertexAttributes(&vert3, vert0, vert1, t, eLerpType_Hyper);
#else
				g_env.renderer->GetCurRas()->LerpVertexAttributes(&vert3, vert0, vert1, t, eLerpType_Linear);
#endif
				vert3.pos.y = y2;

				DrawBottomTri_Scanline_V2(vert0, &vert3, vert2, context);
				DrawTopTri_Scanline_V2(&vert3, vert1, vert2, context);
			}
			break;
		default: assert(0);
		}
	}

	void RenderUtil::DrawBottomTri_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, const SRenderContext& context )
	{
		SScanLinesData scanLineData;

		//NB: 为了正确插值和填充规则,要保持x坐标有序
		if(vert1->pos.x > vert2->pos.x)
			Ext::Swap(vert1, vert2);

		assert(Ext::Ceil32_Fast(vert1->pos.y) == Ext::Ceil32_Fast(vert2->pos.y));

#if USE_MULTI_THREAD == 1
		JobParamRS* param = new JobParamRS;
		param->triType = eTriangleShape_Bottom;
		param->pMaterial = context.pMaterial;
		param->v0 = *vert0;
		param->v1 = *vert1;
		param->v2 = *vert2;
		param->texLod = context.texLod;

		JobRS* job = new JobRS(param);
		g_env.jobMgr->SubmitJob(job, nullptr);
#else
		scanLineData.pMaterial = context.pMaterial;
		scanLineData.texLod = context.texLod;
		g_env.renderer->GetCurRas()->RasTriangleSetup(scanLineData, vert0, vert1, vert2, eTriangleShape_Bottom);
		RasterizeScanLines(scanLineData);
#endif
	}

	void RenderUtil::DrawTopTri_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, const SRenderContext& context )
	{
		SScanLinesData scanLineData;

		//NB: 为了正确插值和填充规则,要保持x坐标有序
		if(vert0->pos.x > vert2->pos.x)
			Ext::Swap(vert0, vert2);

		assert(Ext::Ceil32_Fast(vert0->pos.y) == Ext::Ceil32_Fast(vert2->pos.y));

#if USE_MULTI_THREAD == 1
		JobParamRS* param = new JobParamRS;
		param->triType = eTriangleShape_Top;
		param->pMaterial = context.pMaterial;
		param->v0 = *vert0;
		param->v1 = *vert1;
		param->v2 = *vert2;
		param->texLod = context.texLod;

		JobRS* job = new JobRS(param);
		g_env.jobMgr->SubmitJob(job, nullptr);
#else
		scanLineData.pMaterial = context.pMaterial;
		scanLineData.texLod = context.texLod;
		g_env.renderer->GetCurRas()->RasTriangleSetup(scanLineData, vert0, vert1, vert2, eTriangleShape_Top);		
		RasterizeScanLines(scanLineData);
#endif
	}

	CFCriticalSection g_lock;

	void RenderUtil::RasterizeScanLines( SScanLinesData& scanLineData )
	{
		//定位输出位置
		int lpitch = g_env.renderer->m_backBuffer->GetWidth();

		DWORD* destBuffer = (DWORD*)g_env.renderer->m_backBuffer->GetDataPointer() + scanLineData.curY * lpitch;
		float* zBuffer = (float*)g_env.renderer->m_zBuffer->GetDataPointer() + scanLineData.curY * SCREEN_WIDTH;
		SFragment* fragBuf = g_env.renderer->m_fragmentBuffer + scanLineData.curY * SCREEN_WIDTH;

		Rasterizer* curRaster = g_env.renderer->GetCurRas();
		SScanLine scanLine;

		while (scanLineData.curY <= scanLineData.endY)
		{
			//左上填充规则:左
			scanLine.lineX0 = Ext::Ceil32_Fast(scanLineData.curP_L.x);
			scanLine.lineX1 = Ext::Ceil32_Fast(scanLineData.curP_R.x);

			if(scanLine.lineX1 - scanLine.lineX0 > 0)
			{
				curRaster->RasLineSetup(scanLine, scanLineData);

				if(scanLine.lineX1 > max_clip_x)
				{
					scanLine.lineX1 = max_clip_x;
				}

				// Rasterize a line
				for (int curX=scanLine.lineX0; curX<=scanLine.lineX1; ++curX)
				{
					g_lock.Lock();
					// Z-test
					if(scanLine.zw.x < zBuffer[curX])
					{
						zBuffer[curX] = scanLine.zw.x;

						g_lock.UnLock();

						scanLine.pFragmeng = &fragBuf[curX];
						scanLine.pFragmeng->pMaterial = scanLineData.pMaterial;
						scanLine.pFragmeng->finalColor = destBuffer + curX;

						curRaster->RasterizePixel(scanLine, scanLineData);
					}
					else
					{
						g_lock.UnLock();
					}
					
					// Advance to next pixel
					curRaster->RaterizeAdvancePixel(scanLine);
					Common::Add_Vec2_By_Vec2(scanLine.zw, scanLine.zw, scanLine.dzdw);
				}
			}

			// Advance to next line
			curRaster->RaterizeAdvanceLine(scanLineData);
			++scanLineData.curY;
			Common::Add_Vec3_By_Vec3(scanLineData.curP_L, scanLineData.curP_L, scanLineData.dp_L);
			Common::Add_Vec3_By_Vec3(scanLineData.curP_R, scanLineData.curP_R, scanLineData.dp_R);

			destBuffer += lpitch;
			zBuffer += SCREEN_WIDTH;
			fragBuf += SCREEN_WIDTH;
		}
#if USE_PROFILER == 1
		g_env.profiler->AddRenderedFace();
#endif
	}
}