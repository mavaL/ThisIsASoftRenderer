#include "stdafx.h"
#include "RenderUtil.h"
#include "Renderer.h"
#include "PixelBox.h"
#include "RenderObject.h"
#include "Profiler.h"
#include "ThreadPool/MyJob.h"

namespace SR
{
	void RenderUtil::DrawLine_Bresenahams(int x0, int y0, int x1,int y1, SColor color, bool bClip)
	{
		int cxs, cys,
			cxe, cye;

		color.Saturate();

		// clip and draw each line
		cxs = x0;
		cys = y0;
		cxe = x1;
		cye = y1;

		if(bClip && !ClipLine(cxs, cys, cxe, cye))
			return;

		UCHAR* vb_start = (UCHAR*)g_env.renderer->m_backBuffer->GetDataPointer();
		int bytesPerPixel = g_env.renderer->m_backBuffer->GetBytesPerPixel();
		int lpitch = g_env.renderer->m_backBuffer->GetWidth() * bytesPerPixel;

		int dx,             // difference in x's
			dy,             // difference in y's
			dx2,            // dx,dy * 2
			dy2, 
			x_inc,          // amount in pixel space to move during drawing
			y_inc,          // amount in pixel space to move during drawing
			error,          // the discriminant i.e. error i.e. decision variable
			index;          // used for looping

		// pre-compute first pixel address in video buffer
		vb_start = vb_start + cxs*bytesPerPixel + cys*lpitch;

		// compute horizontal and vertical deltas
		dx = cxe-cxs;
		dy = cye-cys;

		// test which direction the line is going in i.e. slope angle
		if (dx>=0)
		{
			x_inc = bytesPerPixel;

		} // end if line is moving right
		else
		{
			x_inc = -bytesPerPixel;
			dx    = -dx;  // need absolute value

		} // end else moving left

		// test y component of slope

		if (dy>=0)
		{
			y_inc = lpitch;
		} // end if line is moving down
		else
		{
			y_inc = -lpitch;
			dy    = -dy;  // need absolute value

		} // end else moving up

		// compute (dx,dy) * 2
		dx2 = dx << 1;
		dy2 = dy << 1;

		// now based on which delta is greater we can draw the line
		if (dx > dy)
		{
			// initialize error term
			error = dy2 - dx; 

			// draw the line
			for (index=0; index <= dx; index++)
			{
				// set the pixel
				*(DWORD*)vb_start = color.GetAsInt();

				// test if error has overflowed
				if (error >= 0) 
				{
					error-=dx2;

					// move to next line
					vb_start+=y_inc;

				} // end if error overflowed

				// adjust the error term
				error+=dy2;

				// move to the next pixel
				vb_start+=x_inc;

			} // end for

		} // end if |slope| <= 1
		else
		{
			// initialize error term
			error = dx2 - dy; 

			// draw the line
			for (index=0; index <= dy; index++)
			{
				// set the pixel
				*(DWORD*)vb_start = color.GetAsInt();

				// test if error overflowed
				if (error >= 0)
				{
					error-=dy2;

					// move to next line
					vb_start+=x_inc;

				} // end if error overflowed

				// adjust the error term
				error+=dx2;

				// move to the next pixel
				vb_start+=y_inc;

			} // end for

		} // end else |slope| > 1
	}

	void RenderUtil::DrawLine_DDA( int x0, int y0, int x1,int y1, SColor color, bool bClip )
	{
		color.Saturate();

		int cxs, cys,
			cxe, cye;

		// clip and draw each line
		cxs = x0;
		cys = y0;
		cxe = x1;
		cye = y1;

		if(bClip && !ClipLine(cxs, cxe, cys, cye))
			return;

		UCHAR* vb_start = (UCHAR*)g_env.renderer->m_backBuffer->GetDataPointer();
		int bytesPerPixel = g_env.renderer->m_backBuffer->GetBytesPerPixel();
		int lpitch = g_env.renderer->m_backBuffer->GetWidth() * bytesPerPixel;

		vb_start = vb_start + cxs*bytesPerPixel + cys*lpitch;
		int dx = cxe-cxs;
		int dy = cye-cys;
		float k = abs(dy/(float)dx);
		float x_inc, y_inc;

		if (dx>=0)
		{
			x_inc = (float)bytesPerPixel;
		}
		else
		{
			x_inc = -(float)bytesPerPixel;
			dx = -dx;
		}
		if (dy>=0)
		{
			y_inc = 1;
		}
		else
		{
			y_inc = -1;
			dy = -dy;
		}

		y_inc *= k;
		float y=0;
		int x=0;
		*(DWORD*)vb_start = color.GetAsInt();
		for (int index=1; index<dx; ++index)
		{
			y += y_inc;
			x += (int)x_inc;
			*(DWORD*)(vb_start+(int)y*lpitch+x) = color.GetAsInt();
		}
	}

	int RenderUtil::ClipLine( int& x1, int& y1, int& x2, int& y2 )
	{
		// this function clips the sent line using the globally defined clipping
		// region

		// internal clipping codes
#define CLIP_CODE_C  0x0000
#define CLIP_CODE_N  0x0008
#define CLIP_CODE_S  0x0004
#define CLIP_CODE_E  0x0002
#define CLIP_CODE_W  0x0001

#define CLIP_CODE_NE 0x000a
#define CLIP_CODE_SE 0x0006
#define CLIP_CODE_NW 0x0009 
#define CLIP_CODE_SW 0x0005

		int xc1=x1, 
			yc1=y1, 
			xc2=x2, 
			yc2=y2;

		int p1_code=0, 
			p2_code=0;

		// determine codes for p1 and p2
		if (y1 < min_clip_y)
			p1_code|=CLIP_CODE_N;
		else
			if (y1 > max_clip_y)
				p1_code|=CLIP_CODE_S;

		if (x1 < min_clip_x)
			p1_code|=CLIP_CODE_W;
		else
			if (x1 > max_clip_x)
				p1_code|=CLIP_CODE_E;

		if (y2 < min_clip_y)
			p2_code|=CLIP_CODE_N;
		else
			if (y2 > max_clip_y)
				p2_code|=CLIP_CODE_S;

		if (x2 < min_clip_x)
			p2_code|=CLIP_CODE_W;
		else
			if (x2 > max_clip_x)
				p2_code|=CLIP_CODE_E;

		// try and trivially reject
		if ((p1_code & p2_code)) 
			return(0);

		// test for totally visible, if so leave points untouched
		if (p1_code==0 && p2_code==0)
			return(1);

		// determine end clip point for p1
		switch(p1_code)
		{
		case CLIP_CODE_C: break;

		case CLIP_CODE_N:
			{
				yc1 = min_clip_y;
				xc1 = x1 + 0.5+(min_clip_y-y1)*(x2-x1)/(y2-y1);
			} break;
		case CLIP_CODE_S:
			{
				yc1 = max_clip_y;
				xc1 = x1 + 0.5+(max_clip_y-y1)*(x2-x1)/(y2-y1);
			} break;

		case CLIP_CODE_W:
			{
				xc1 = min_clip_x;
				yc1 = y1 + 0.5+(min_clip_x-x1)*(y2-y1)/(x2-x1);
			} break;

		case CLIP_CODE_E:
			{
				xc1 = max_clip_x;
				yc1 = y1 + 0.5+(max_clip_x-x1)*(y2-y1)/(x2-x1);
			} break;

			// these cases are more complex, must compute 2 intersections
		case CLIP_CODE_NE:
			{
				// north hline intersection
				yc1 = min_clip_y;
				xc1 = x1 + 0.5+(min_clip_y-y1)*(x2-x1)/(y2-y1);

				// test if intersection is valid, of so then done, else compute next
				if (xc1 < min_clip_x || xc1 > max_clip_x)
				{
					// east vline intersection
					xc1 = max_clip_x;
					yc1 = y1 + 0.5+(max_clip_x-x1)*(y2-y1)/(x2-x1);
				} // end if

			} break;

		case CLIP_CODE_SE:
			{
				// south hline intersection
				yc1 = max_clip_y;
				xc1 = x1 + 0.5+(max_clip_y-y1)*(x2-x1)/(y2-y1);	

				// test if intersection is valid, of so then done, else compute next
				if (xc1 < min_clip_x || xc1 > max_clip_x)
				{
					// east vline intersection
					xc1 = max_clip_x;
					yc1 = y1 + 0.5+(max_clip_x-x1)*(y2-y1)/(x2-x1);
				} // end if

			} break;

		case CLIP_CODE_NW: 
			{
				// north hline intersection
				yc1 = min_clip_y;
				xc1 = x1 + 0.5+(min_clip_y-y1)*(x2-x1)/(y2-y1);

				// test if intersection is valid, of so then done, else compute next
				if (xc1 < min_clip_x || xc1 > max_clip_x)
				{
					xc1 = min_clip_x;
					yc1 = y1 + 0.5+(min_clip_x-x1)*(y2-y1)/(x2-x1);	
				} // end if

			} break;

		case CLIP_CODE_SW:
			{
				// south hline intersection
				yc1 = max_clip_y;
				xc1 = x1 + 0.5+(max_clip_y-y1)*(x2-x1)/(y2-y1);	

				// test if intersection is valid, of so then done, else compute next
				if (xc1 < min_clip_x || xc1 > max_clip_x)
				{
					xc1 = min_clip_x;
					yc1 = y1 + 0.5+(min_clip_x-x1)*(y2-y1)/(x2-x1);	
				} // end if

			} break;

		default:break;

		} // end switch

		// determine clip point for p2
		switch(p2_code)
		{
		case CLIP_CODE_C: break;

		case CLIP_CODE_N:
			{
				yc2 = min_clip_y;
				xc2 = x2 + (min_clip_y-y2)*(x1-x2)/(y1-y2);
			} break;

		case CLIP_CODE_S:
			{
				yc2 = max_clip_y;
				xc2 = x2 + (max_clip_y-y2)*(x1-x2)/(y1-y2);
			} break;

		case CLIP_CODE_W:
			{
				xc2 = min_clip_x;
				yc2 = y2 + (min_clip_x-x2)*(y1-y2)/(x1-x2);
			} break;

		case CLIP_CODE_E:
			{
				xc2 = max_clip_x;
				yc2 = y2 + (max_clip_x-x2)*(y1-y2)/(x1-x2);
			} break;

			// these cases are more complex, must compute 2 intersections
		case CLIP_CODE_NE:
			{
				// north hline intersection
				yc2 = min_clip_y;
				xc2 = x2 + 0.5+(min_clip_y-y2)*(x1-x2)/(y1-y2);

				// test if intersection is valid, of so then done, else compute next
				if (xc2 < min_clip_x || xc2 > max_clip_x)
				{
					// east vline intersection
					xc2 = max_clip_x;
					yc2 = y2 + 0.5+(max_clip_x-x2)*(y1-y2)/(x1-x2);
				} // end if

			} break;

		case CLIP_CODE_SE:
			{
				// south hline intersection
				yc2 = max_clip_y;
				xc2 = x2 + 0.5+(max_clip_y-y2)*(x1-x2)/(y1-y2);	

				// test if intersection is valid, of so then done, else compute next
				if (xc2 < min_clip_x || xc2 > max_clip_x)
				{
					// east vline intersection
					xc2 = max_clip_x;
					yc2 = y2 + 0.5+(max_clip_x-x2)*(y1-y2)/(x1-x2);
				} // end if

			} break;

		case CLIP_CODE_NW: 
			{
				// north hline intersection
				yc2 = min_clip_y;
				xc2 = x2 + 0.5+(min_clip_y-y2)*(x1-x2)/(y1-y2);

				// test if intersection is valid, of so then done, else compute next
				if (xc2 < min_clip_x || xc2 > max_clip_x)
				{
					xc2 = min_clip_x;
					yc2 = y2 + 0.5+(min_clip_x-x2)*(y1-y2)/(x1-x2);	
				} // end if

			} break;

		case CLIP_CODE_SW:
			{
				// south hline intersection
				yc2 = max_clip_y;
				xc2 = x2 + 0.5+(max_clip_y-y2)*(x1-x2)/(y1-y2);	

				// test if intersection is valid, of so then done, else compute next
				if (xc2 < min_clip_x || xc2 > max_clip_x)
				{
					xc2 = min_clip_x;
					yc2 = y2 + 0.5+(min_clip_x-x2)*(y1-y2)/(x1-x2);	
				} // end if

			} break;

		default:break;

		} // end switch

		// do bounds check
		if ((xc1 < min_clip_x) || (xc1 > max_clip_x) ||
			(yc1 < min_clip_y) || (yc1 > max_clip_y) ||
			(xc2 < min_clip_x) || (xc2 > max_clip_x) ||
			(yc2 < min_clip_y) || (yc2 > max_clip_y) )
		{
			return(0);
		} // end if

		// store vars back
		x1 = xc1;
		y1 = yc1;
		x2 = xc2;
		y2 = yc2;

		return(1);
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
				RenderUtil::DrawLine_Bresenahams(lineX0, curY, lineX1, curY, color, true);
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
				RenderUtil::DrawLine_Bresenahams(lineX0, curY, lineX1, curY, color, true);
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
		g_env.renderer->GetCurRas()->RasTriangleSetup(scanLineData, vert0, vert1, vert2, eTriangleShape_Bottom);
		RasterizeScanLines(scanLineData, context);
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
		g_env.renderer->GetCurRas()->RasTriangleSetup(scanLineData, vert0, vert1, vert2, eTriangleShape_Top);		
		RasterizeScanLines(scanLineData, context);
#endif
	}

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

			if(scanLine.lineX1 - scanLine.lineX0 >= 0)
			{
				curRaster->RasLineSetup(scanLine, scanLineData);

				if(scanLine.lineX1 > max_clip_x)
				{
					scanLine.lineX1 = max_clip_x;
				}

				// Rasterize a line
				for (int curX=scanLine.lineX0; curX<=scanLine.lineX1; ++curX)
				{
					// Z-test
					if(scanLine.z < zBuffer[curX])
					{
						zBuffer[curX] = scanLine.z;

						curRaster->RasterizePixel(scanLine, scanLineData);
						
#if USE_MULTI_THREAD == 1
// 						SFragment& frag = fragBuf[curX];
// 						frag.bActive = true;
// 						frag.pMaterial = scanLineData.pMaterial;
// 						frag.finalColor = destBuffer + curX;
// 						frag.texLod = scanLineData.texLod;
// 						frag.uv.Set(curUV.x*inv_w, curUV.y*inv_w);
// 						frag.worldPos.Set(curPW.x*inv_w, curPW.y*inv_w, curPW.z*inv_w);
// 						frag.normal.Set(curN.x*inv_w, curN.y*inv_w, curN.z*inv_w);
#else
						scanLine.pixelColor.Saturate();
						// Output to back buffer
						destBuffer[curX] = scanLine.pixelColor.GetAsInt();

#if USE_PROFILER == 1
						g_env.profiler->AddRenderedPixel();
#endif
#endif
					}
					
					// Advance to next pixel
					Common::Add_Vec3_By_Vec3(scanLine.curClr, scanLine.curClr, scanLine.deltaClr);
					Common::Add_Vec2_By_Vec2(scanLine.curUV, scanLine.curUV, scanLine.deltaUV);
					Common::Add_Vec3_By_Vec3(scanLine.curPW, scanLine.curPW, scanLine.deltaPW);
					Common::Add_Vec3_By_Vec3(scanLine.curN, scanLine.curN, scanLine.deltaN);
					Common::Add_Vec3_By_Vec3(scanLine.curLightDir, scanLine.curLightDir, scanLine.deltaLightDir);
					Common::Add_Vec3_By_Vec3(scanLine.curHVector, scanLine.curHVector, scanLine.deltaHVector);
					scanLine.z += scanLine.dz;
					scanLine.w += scanLine.dw;
				}
			}

			// Advance to next line
			++scanLineData.curY;
			Common::Add_Vec3_By_Vec3(scanLineData.curP_L, scanLineData.curP_L, scanLineData.dp_L);
			Common::Add_Vec3_By_Vec3(scanLineData.curP_R, scanLineData.curP_R, scanLineData.dp_R);
			Common::Add_Vec3_By_Vec3(scanLineData.clr_L, scanLineData.clr_L, scanLineData.dclr_L);
			Common::Add_Vec3_By_Vec3(scanLineData.clr_R, scanLineData.clr_R, scanLineData.dclr_R);
			Common::Add_Vec2_By_Vec2(scanLineData.curUV_L, scanLineData.curUV_L, scanLineData.duv_L);
			Common::Add_Vec2_By_Vec2(scanLineData.curUV_R, scanLineData.curUV_R, scanLineData.duv_R);
			Common::Add_Vec3_By_Vec3(scanLineData.curPW_L, scanLineData.curPW_L, scanLineData.dpw_L);
			Common::Add_Vec3_By_Vec3(scanLineData.curPW_R, scanLineData.curPW_R, scanLineData.dpw_R);
			Common::Add_Vec3_By_Vec3(scanLineData.curN_L, scanLineData.curN_L, scanLineData.dn_L);
			Common::Add_Vec3_By_Vec3(scanLineData.curN_R, scanLineData.curN_R, scanLineData.dn_R);
			Common::Add_Vec3_By_Vec3(scanLineData.curLightDir_L, scanLineData.curLightDir_L, scanLineData.dLightDir_L);
			Common::Add_Vec3_By_Vec3(scanLineData.curLightDir_R, scanLineData.curLightDir_R, scanLineData.dLightDir_R);
			Common::Add_Vec3_By_Vec3(scanLineData.curHVector_L, scanLineData.curHVector_L, scanLineData.dHVector_L);
			Common::Add_Vec3_By_Vec3(scanLineData.curHVector_R, scanLineData.curHVector_R, scanLineData.dHVector_R);

			destBuffer += lpitch;
			zBuffer += SCREEN_WIDTH;
			fragBuf += SCREEN_WIDTH;
		}
#if USE_PROFILER == 1
		g_env.profiler->AddRenderedFace();
#endif
	}
}