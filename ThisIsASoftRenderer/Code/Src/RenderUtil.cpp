#include "stdafx.h"
#include "Renderer.h"
#include "PixelBox.h"
#include "RenderObject.h"

// clipping rectangle 
const int	min_clip_x = 0;                          
const int	max_clip_x = (SCREEN_WIDTH-1);
const int	min_clip_y = 0;
const int	max_clip_y = (SCREEN_HEIGHT-1);

namespace SR
{
	void RenderUtil::DrawLine_Bresenahams(int x0, int y0, int x1,int y1, SColor color, bool bClip)
	{
		int cxs, cys,
			cxe, cye;

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
				*(DWORD*)vb_start = color.color;

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
				*(DWORD*)vb_start = color.color;

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
		*(DWORD*)vb_start = color.color;
		for (int index=1; index<dx; ++index)
		{
			y += y_inc;
			x += (int)x_inc;
			*(DWORD*)(vb_start+(int)y*lpitch+x) = color.color;
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

	void RenderUtil::DrawText( float x, float y, const STRING& text, const Gdiplus::Color& color )
	{
		Gdiplus::Graphics g(g_env.renderer->m_bmBackBuffer.get());
		g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
		Gdiplus::Font font(L"Arial", 11);
		Gdiplus::PointF origin(x, y);
		Gdiplus::SolidBrush bru(Gdiplus::Color(255,0,255,0));
		Gdiplus::StringFormat format;
		format.SetAlignment(Gdiplus::StringAlignmentNear);

		std::wstring wstr = Ext::AnsiToUnicode(text.c_str());
		g.DrawString(wstr.c_str(), -1, &font, origin, &format, &bru);
	}

	void RenderUtil::ComputeAABB( RenderObject& obj )
	{
		AABB& aabb = obj.m_localAABB;
		const VertexBuffer& verts = obj.m_verts;

		//AABB start with the first pos
		aabb.m_minCorner = verts[0].pos.GetVec3();
		aabb.m_maxCorner = verts[0].pos.GetVec3();

		for (size_t i=1; i<verts.size(); ++i)
		{
			aabb.Merge(verts[i].pos);
		}
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

		++g_env.renderer->m_frameStatics.nRenderedFace;
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

		++g_env.renderer->m_frameStatics.nRenderedFace;
	}

	void RenderUtil::SortTris_PainterAlgorithm( const VertexBuffer& verts, FaceList& faces )
	{
		//用画家算法对所有面进行排序(根据三角面3个顶点的平均z值)
		//NB: 该算法在某些面重叠的情况下是不正确的,见截图Compare.jpg
		std::sort(faces.begin(), faces.end(), [&](const SFace& face1, const SFace& face2)->bool
		{
			const Index idx1 = face1.index1;
			const Index idx2 = face1.index2;
			const Index idx3 = face1.index3;

			float z1 = verts[idx1].pos.z + verts[idx2].pos.z + verts[idx3].pos.z;
			z1 *= 0.33333f;

			const Index idx4 = face2.index1;
			const Index idx5 = face2.index2;
			const Index idx6 = face2.index3;

			float z2 = verts[idx4].pos.z + verts[idx5].pos.z + verts[idx6].pos.z;
			z2 *= 0.33333f;

			return z1 > z2;
		});
	}

	void RenderUtil::DrawTriangle_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, bool bTextured, bool bPerPixel, const SRenderContext& context )
	{
		eTriangleShape triType;
		if(!PreDrawTriangle(vert0, vert1, vert2, triType))
			return;

		RasGouraud::SScanLineData scanLineData;

		switch (triType)
		{
		case eTriangleShape_Bottom: DrawBottomTri_Scanline_V2(vert0, vert1, vert2, bTextured, bPerPixel, context, scanLineData); break;
		case eTriangleShape_Top: DrawTopTri_Scanline_V2(vert0, vert1, vert2, bTextured, bPerPixel, context, scanLineData); break;
		case eTriangleShape_General:	
			{
				//创建切割生成的新顶点
				SVertex vert3;
				float x0 = vert0->pos.x;
				float x1 = vert1->pos.x;
				float x2 = vert2->pos.x;
				float y0 = vert0->pos.y;
				float y1 = vert1->pos.y;
				float y2 = vert2->pos.y;
				float z0 = vert0->pos.z;
				float z1 = vert1->pos.z;
				float w0 = vert0->pos.w;
				float w1 = vert1->pos.w;
				//线性插值各属性
				float t = (y2-y0)/(y1-y0);
				Ext::LinearLerp(vert3.pos, vert0->pos, vert1->pos, t);
				vert3.pos.y = y2;
				Ext::LinearLerp(vert3.color, vert0->color, vert1->color, t);
				vert3.color.a = 255;

#if USE_PERSPEC_CORRECT == 1
				Ext::HyperLerp(vert3.uv, vert0->uv, vert1->uv, t, w0, w1);
				//双曲插值最后一步
				float inv_w = 1 / vert3.pos.w;
				vert3.uv.x *= inv_w;
				vert3.uv.y *= inv_w;

				if(bPerPixel)
				{
					Ext::HyperLerp(vert3.worldPos, vert0->worldPos, vert1->worldPos, t, w0, w1);
					Ext::HyperLerp(vert3.worldNormal, vert0->worldNormal, vert1->worldNormal, t, w0, w1);
					//双曲插值最后一步
					vert3.worldPos.x *= inv_w;
					vert3.worldPos.y *= inv_w;
					vert3.worldPos.z *= inv_w;
					vert3.worldNormal.x *= inv_w;
					vert3.worldNormal.y *= inv_w;
					vert3.worldNormal.z *= inv_w;
				}
#else
				Ext::LinearLerp(vert3.uv, vert0->uv, vert1->uv, t);

				if(bPerPixel)
				{
					Ext::LinearLerp(vert3.worldPos, vert0->worldPos, vert1->worldPos, t);
					Ext::LinearLerp(vert3.worldNormal, vert0->worldNormal, vert1->worldNormal, t);
				}
#endif
				DrawBottomTri_Scanline_V2(vert0, &vert3, vert2, bTextured, bPerPixel, context, scanLineData);
				DrawTopTri_Scanline_V2(&vert3, vert1, vert2, bTextured, bPerPixel, context, scanLineData);
			}
			break;
		default: assert(0);
		}
	}

	void RenderUtil::DrawBottomTri_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, bool bTextured, bool bPerPixel, const SRenderContext& context, RasGouraud::SScanLineData& scanLineData )
	{
		//NB: 为了正确插值和填充规则,要保持x坐标有序
		if(vert1->pos.x > vert2->pos.x)
			Ext::Swap(vert1, vert2);

		const VEC4& p0 = vert0->pos;
		const VEC4& p1 = vert1->pos;
		const VEC4& p2 = vert2->pos;

#if USE_PERSPEC_CORRECT == 1
		const VEC2 uv0(vert0->uv.x*p0.w, vert0->uv.y*p0.w);
		const VEC2 uv1(vert1->uv.x*p1.w, vert1->uv.y*p1.w);
		const VEC2 uv2(vert2->uv.x*p2.w, vert2->uv.y*p2.w);

		const VEC3 wp0(vert0->worldPos.x*p0.w, vert0->worldPos.y*p0.w, vert0->worldPos.z*p0.w);
		const VEC3 wp1(vert1->worldPos.x*p1.w, vert1->worldPos.y*p1.w, vert1->worldPos.z*p1.w);
		const VEC3 wp2(vert2->worldPos.x*p2.w, vert2->worldPos.y*p2.w, vert2->worldPos.z*p2.w);

		const VEC3 wn0(vert0->worldNormal.x*p0.w, vert0->worldNormal.y*p0.w, vert0->worldNormal.z*p0.w);
		const VEC3 wn1(vert1->worldNormal.x*p1.w, vert1->worldNormal.y*p1.w, vert1->worldNormal.z*p1.w);
		const VEC3 wn2(vert2->worldNormal.x*p2.w, vert2->worldNormal.y*p2.w, vert2->worldNormal.z*p2.w);
#else
		const VEC2 uv0(vert0->uv.x, vert0->uv.y);
		const VEC2 uv1(vert1->uv.x, vert1->uv.y);
		const VEC2 uv2(vert2->uv.x, vert2->uv.y);

		const VEC3& wp0 = vert0->worldPos.GetVec3();
		const VEC3& wp1 = vert1->worldPos.GetVec3();
		const VEC3& wp2 = vert2->worldPos.GetVec3();

		const VEC3& wn0 = vert0->worldNormal;
		const VEC3& wn1 = vert1->worldNormal;
		const VEC3& wn2 = vert2->worldNormal;

#endif

		//NB: 顶点颜色理论上也应该进行透视校正,这里偷个懒不做了
		const SColor& c0 = vert0->color;
		const SColor& c1 = vert1->color;
		const SColor& c2 = vert2->color;

		assert(Ext::Ceil32_Fast(p1.y) == Ext::Ceil32_Fast(p2.y));

		///填充光栅化结构
		//左上填充规则:上
		scanLineData.curY = Ext::Ceil32_Fast(p0.y), scanLineData.endY = Ext::Ceil32_Fast(p1.y) - 1;
		//位置坐标及增量
		float inv_dy_L = 1.0f / (p1.y - p0.y);
		float inv_dy_R = 1.0f / (p2.y - p0.y);
		scanLineData.curP_L.Set(p0.x, p0.z, p0.w);
		scanLineData.curP_R.Set(p0.x, p0.z, p0.w);
		scanLineData.dp_L.Set((p1.x-p0.x)*inv_dy_L, (p1.z-p0.z)*inv_dy_L, (p1.w-p0.w)*inv_dy_L);
		scanLineData.dp_R.Set((p2.x-p0.x)*inv_dy_R, (p2.z-p0.z)*inv_dy_R, (p2.w-p0.w)*inv_dy_R);
		//当前两端点uv分量及增量
		scanLineData.curUV_L = uv0;
		scanLineData.curUV_R = uv0;
		scanLineData.duv_L.Set((uv1.x-uv0.x)*inv_dy_L, (uv1.y-uv0.y)*inv_dy_L);
		scanLineData.duv_R.Set((uv2.x-uv0.x)*inv_dy_R, (uv2.y-uv0.y)*inv_dy_R);

		if(bPerPixel)
		{
			//世界坐标及增量
			scanLineData.curPW_L = wp0;
			scanLineData.curPW_R = wp0;
			scanLineData.dpw_L.Set((wp1.x-wp0.x)*inv_dy_L, (wp1.y-wp0.y)*inv_dy_L, (wp1.z-wp0.z)*inv_dy_L);
			scanLineData.dpw_R.Set((wp2.x-wp0.x)*inv_dy_R, (wp2.y-wp0.y)*inv_dy_R, (wp2.z-wp0.z)*inv_dy_R);
			//世界法线及增量
			scanLineData.curN_L = wn0;
			scanLineData.curN_R = wn0;
			scanLineData.dn_L.Set((wn1.x-wn0.x)*inv_dy_L, (wn1.y-wn0.y)*inv_dy_L, (wn1.z-wn0.z)*inv_dy_L);
			scanLineData.dn_R.Set((wn2.x-wn0.x)*inv_dy_R, (wn2.y-wn0.y)*inv_dy_R, (wn2.z-wn0.z)*inv_dy_R);
		}
		else	//逐像素光照就不用插值顶点颜色了
		{
			//当前两端点颜色分量及增量
			scanLineData.clr_L.Set(c0.r, c0.g, c0.b);
			scanLineData.clr_R.Set(c0.r, c0.g, c0.b);
			scanLineData.dclr_L.Set((c1.r-c0.r)*inv_dy_L, (c1.g-c0.g)*inv_dy_L, (c1.b-c0.b)*inv_dy_L);
			scanLineData.dclr_R.Set((c2.r-c0.r)*inv_dy_R, (c2.g-c0.g)*inv_dy_R, (c2.b-c0.b)*inv_dy_R);
		}

		DrawScanLines(scanLineData, bTextured, bPerPixel, context);

		++g_env.renderer->m_frameStatics.nRenderedFace;
	}

	void RenderUtil::DrawTopTri_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, bool bTextured, bool bPerPixel, const SRenderContext& context, RasGouraud::SScanLineData& scanLineData )
	{
		//NB: 为了正确插值和填充规则,要保持x坐标有序
		if(vert0->pos.x > vert2->pos.x)
			Ext::Swap(vert0, vert2);

		const VEC4& p0 = vert0->pos;
		const VEC4& p1 = vert1->pos;
		const VEC4& p2 = vert2->pos;
#if USE_PERSPEC_CORRECT == 1
		const VEC2 uv0(vert0->uv.x*p0.w, vert0->uv.y*p0.w);
		const VEC2 uv1(vert1->uv.x*p1.w, vert1->uv.y*p1.w);
		const VEC2 uv2(vert2->uv.x*p2.w, vert2->uv.y*p2.w);

		const VEC3 wp0(vert0->worldPos.x*p0.w, vert0->worldPos.y*p0.w, vert0->worldPos.z*p0.w);
		const VEC3 wp1(vert1->worldPos.x*p1.w, vert1->worldPos.y*p1.w, vert1->worldPos.z*p1.w);
		const VEC3 wp2(vert2->worldPos.x*p2.w, vert2->worldPos.y*p2.w, vert2->worldPos.z*p2.w);

		const VEC3 wn0(vert0->worldNormal.x*p0.w, vert0->worldNormal.y*p0.w, vert0->worldNormal.z*p0.w);
		const VEC3 wn1(vert1->worldNormal.x*p1.w, vert1->worldNormal.y*p1.w, vert1->worldNormal.z*p1.w);
		const VEC3 wn2(vert2->worldNormal.x*p2.w, vert2->worldNormal.y*p2.w, vert2->worldNormal.z*p2.w);
#else
		const VEC2 uv0(vert0->uv.x, vert0->uv.y);
		const VEC2 uv1(vert1->uv.x, vert1->uv.y);
		const VEC2 uv2(vert2->uv.x, vert2->uv.y);

		const VEC3& wp0 = vert0->worldPos.GetVec3();
		const VEC3& wp1 = vert1->worldPos.GetVec3();
		const VEC3& wp2 = vert2->worldPos.GetVec3();

		const VEC3& wn0 = vert0->worldNormal;
		const VEC3& wn1 = vert1->worldNormal;
		const VEC3& wn2 = vert2->worldNormal;

#endif
		//NB: 顶点颜色理论上也应该进行透视校正,这里偷个懒不做了
		const SColor& c0 = vert0->color;
		const SColor& c1 = vert1->color;
		const SColor& c2 = vert2->color;

		assert(Ext::Ceil32_Fast(p0.y) == Ext::Ceil32_Fast(p2.y));

		///填充光栅化结构
		//左上填充规则:上
		scanLineData.curY = Ext::Ceil32_Fast(p0.y), scanLineData.endY = Ext::Ceil32_Fast(p1.y) - 1;
		//屏幕坐标及增量
		float inv_dy_L = 1.0f / (p1.y - p0.y);
		float inv_dy_R = 1.0f / (p1.y - p2.y);
		scanLineData.curP_L.Set(p0.x, p0.z, p0.w);
		scanLineData.curP_R.Set(p2.x, p2.z, p2.w);
		scanLineData.dp_L.Set((p1.x-p0.x)*inv_dy_L, (p1.z-p0.z)*inv_dy_L, (p1.w-p0.w)*inv_dy_L);
		scanLineData.dp_R.Set((p1.x-p2.x)*inv_dy_R, (p1.z-p2.z)*inv_dy_R, (p1.w-p2.w)*inv_dy_R);
		//当前两端点uv分量及增量
		scanLineData.curUV_L = uv0;
		scanLineData.curUV_R = uv2;
		scanLineData.duv_L.Set((uv1.x-uv0.x)*inv_dy_L, (uv1.y-uv0.y)*inv_dy_L);
		scanLineData.duv_R.Set((uv1.x-uv2.x)*inv_dy_R, (uv1.y-uv2.y)*inv_dy_R);

		if(bPerPixel)
		{
			//世界坐标及增量
			scanLineData.curPW_L = wp0;
			scanLineData.curPW_R = wp2;
			scanLineData.dpw_L.Set((wp1.x-wp0.x)*inv_dy_L, (wp1.y-wp0.y)*inv_dy_L, (wp1.z-wp0.z)*inv_dy_L);
			scanLineData.dpw_R.Set((wp1.x-wp2.x)*inv_dy_R, (wp1.y-wp2.y)*inv_dy_R, (wp1.z-wp2.z)*inv_dy_R);
			//世界法线及增量
			scanLineData.curN_L = wn0;
			scanLineData.curN_R = wn2;
			scanLineData.dn_L.Set((wn1.x-wn0.x)*inv_dy_L, (wn1.y-wn0.y)*inv_dy_L, (wn1.z-wn0.z)*inv_dy_L);
			scanLineData.dn_R.Set((wn1.x-wn2.x)*inv_dy_R, (wn1.y-wn2.y)*inv_dy_R, (wn1.z-wn2.z)*inv_dy_R);
		}
		else	//逐像素光照就不用插值顶点颜色了
		{
			//当前两端点颜色分量及增量
			scanLineData.clr_L.Set(c0.r, c0.g, c0.b);
			scanLineData.clr_R.Set(c2.r, c2.g, c2.b);
			scanLineData.dclr_L.Set((c1.r-c0.r)*inv_dy_L, (c1.g-c0.g)*inv_dy_L, (c1.b-c0.b)*inv_dy_L);
			scanLineData.dclr_R.Set((c1.r-c2.r)*inv_dy_R, (c1.g-c2.g)*inv_dy_R, (c1.b-c2.b)*inv_dy_R);
		}

		DrawScanLines(scanLineData, bTextured, bPerPixel, context);

		++g_env.renderer->m_frameStatics.nRenderedFace;
	}

	const static float INV_COLOR	=	1.0f / 255.0f;

	void RenderUtil::DrawScanLines( RasGouraud::SScanLineData& scanLineData, bool bTextured, bool bPerPixel, const SRenderContext& context )
	{
		//裁剪区域裁剪y
		if(scanLineData.curY < min_clip_y)
		{
			float dy = min_clip_y - scanLineData.curY;
			scanLineData.curY = min_clip_y;
			Common::Add_Vec3_By_Vec3(scanLineData.curP_L, scanLineData.curP_L, Common::Multiply_Vec3_By_K(scanLineData.dp_L, dy));
			Common::Add_Vec3_By_Vec3(scanLineData.curP_R, scanLineData.curP_R, Common::Multiply_Vec3_By_K(scanLineData.dp_R, dy));
			Common::Add_Vec3_By_Vec3(scanLineData.clr_L, scanLineData.clr_L, Common::Multiply_Vec3_By_K(scanLineData.dclr_L, dy));
			Common::Add_Vec3_By_Vec3(scanLineData.clr_R, scanLineData.clr_R, Common::Multiply_Vec3_By_K(scanLineData.dclr_R, dy));
			Common::Add_Vec2_By_Vec2(scanLineData.curUV_L, scanLineData.curUV_L, Common::Multiply_Vec2_By_K(scanLineData.duv_L, dy));
			Common::Add_Vec2_By_Vec2(scanLineData.curUV_R, scanLineData.curUV_R, Common::Multiply_Vec2_By_K(scanLineData.duv_R, dy));
			Common::Add_Vec3_By_Vec3(scanLineData.curPW_L, scanLineData.curPW_L, Common::Multiply_Vec3_By_K(scanLineData.dpw_L, dy));
			Common::Add_Vec3_By_Vec3(scanLineData.curPW_R, scanLineData.curPW_R, Common::Multiply_Vec3_By_K(scanLineData.dpw_R, dy));
			Common::Add_Vec3_By_Vec3(scanLineData.curN_L, scanLineData.curN_L, Common::Multiply_Vec3_By_K(scanLineData.dn_L, dy));
			Common::Add_Vec3_By_Vec3(scanLineData.curN_R, scanLineData.curN_R, Common::Multiply_Vec3_By_K(scanLineData.dn_R, dy));

		}
		if(scanLineData.endY > max_clip_y)
		{
			scanLineData.endY = max_clip_y;
		}

		//定位输出位置
		DWORD* vb_start = (DWORD*)g_env.renderer->m_backBuffer->GetDataPointer();
		int lpitch = g_env.renderer->m_backBuffer->GetWidth();
		DWORD* destBuffer = vb_start + scanLineData.curY * lpitch;
		float* zBuffer = (float*)g_env.renderer->m_zBuffer->GetDataPointer() + scanLineData.curY * lpitch;
		SColor pixelColor, curPixelClr;
		VEC3 curClr, curPW, curN, finalPW, finalN;
		VEC3 deltaClr, deltaPW, deltaN;
		VEC2 curUV, finalUV;
		VEC2 deltaUV;

		while (scanLineData.curY <= scanLineData.endY)
		{
			//左上填充规则:左
			int lineX0 = Ext::Ceil32_Fast(scanLineData.curP_L.x);
			int lineX1 = Ext::Ceil32_Fast(scanLineData.curP_R.x);

			if(lineX1 - lineX0 >= 0)
			{
				float invdx = 1.0f / (lineX1 - lineX0);
				deltaClr.Set((scanLineData.clr_R.x-scanLineData.clr_L.x)*invdx, (scanLineData.clr_R.y-scanLineData.clr_L.y)*invdx, (scanLineData.clr_R.z-scanLineData.clr_L.z)*invdx);
				deltaUV.Set((scanLineData.curUV_R.x-scanLineData.curUV_L.x)*invdx, (scanLineData.curUV_R.y-scanLineData.curUV_L.y)*invdx);
				deltaPW.Set((scanLineData.curPW_R.x-scanLineData.curPW_L.x)*invdx, (scanLineData.curPW_R.y-scanLineData.curPW_L.y)*invdx, (scanLineData.curPW_R.z-scanLineData.curPW_L.z)*invdx);
				deltaN.Set((scanLineData.curN_R.x-scanLineData.curN_L.x)*invdx, (scanLineData.curN_R.y-scanLineData.curN_L.y)*invdx, (scanLineData.curN_R.z-scanLineData.curN_L.z)*invdx);
				float dz = (scanLineData.curP_R.y - scanLineData.curP_L.y) * invdx;
				float dw = (scanLineData.curP_R.z - scanLineData.curP_L.z) * invdx;

				curClr = scanLineData.clr_L;
				curUV = scanLineData.curUV_L;
				curPW = scanLineData.curPW_L;
				curN = scanLineData.curN_L;
				float z = scanLineData.curP_L.y;
				float w = scanLineData.curP_L.z;

				//裁剪区域裁剪x
				if(lineX0 < min_clip_x)
				{
					const float dx = min_clip_x-lineX0;
					Common::Add_Vec3_By_Vec3(curClr, curClr, Common::Multiply_Vec3_By_K(deltaClr, dx));
					Common::Add_Vec2_By_Vec2(curUV, curUV, Common::Multiply_Vec2_By_K(deltaUV, dx));
					Common::Add_Vec3_By_Vec3(curPW, curPW, Common::Multiply_Vec3_By_K(deltaPW, dx));
					Common::Add_Vec3_By_Vec3(curN, curN, Common::Multiply_Vec3_By_K(deltaN, dx));
					lineX0 = min_clip_x;
					z += dx*dz;
					w += dx*dw;
				}
				if(lineX1 > max_clip_x)
				{
					lineX1 = max_clip_x;
				}

				//画水平直线
				for (int curX=lineX0; curX<=lineX1; ++curX)
				{
					//深度测试
					if(z < zBuffer[curX])
					{
						if(bTextured)
						{
#if USE_PERSPEC_CORRECT == 1
							//双曲插值最后一步
							float inv_w = 1 / w;
							finalUV.Set(curUV.x*inv_w, curUV.y*inv_w);
 							finalPW.Set(curPW.x*inv_w, curPW.y*inv_w, curPW.z*inv_w);
 							finalN.Set(curN.x*inv_w, curN.y*inv_w, curN.z*inv_w);
#else
							finalUV = curUV;
							finalPW = curPW;
							finalN = curN;
#endif			
							if(context.pMaterial->pDiffuseMap && context.pMaterial->bUseBilinearSampler)
							{
								context.pMaterial->pDiffuseMap->Tex2D_Bilinear(finalUV, pixelColor);
							}
							else if(context.pMaterial->pDiffuseMap)
							{
								context.pMaterial->pDiffuseMap->Tex2D_Point(finalUV, pixelColor);
							}
							else
							{
								pixelColor = SColor::WHITE;
							}

							if(bPerPixel)
							{
								g_env.renderer->m_curRas->DoPerPixelLighting(curPixelClr, finalPW, finalN, context.pMaterial);
								pixelColor.r = Ext::Ftoi32_Fast(pixelColor.r * curPixelClr.r * INV_COLOR);
								pixelColor.g = Ext::Ftoi32_Fast(pixelColor.g * curPixelClr.g * INV_COLOR);
								pixelColor.b = Ext::Ftoi32_Fast(pixelColor.b * curPixelClr.b * INV_COLOR);
							}
							else
							{
								pixelColor.r = Ext::Ftoi32_Fast(pixelColor.r * curClr.x * INV_COLOR);
								pixelColor.g = Ext::Ftoi32_Fast(pixelColor.g * curClr.y * INV_COLOR);
								pixelColor.b = Ext::Ftoi32_Fast(pixelColor.b * curClr.z * INV_COLOR);
							}
						}
						else
						{
							pixelColor.r = Ext::Ftoi32_Fast(curClr.x);
							pixelColor.g = Ext::Ftoi32_Fast(curClr.y); 
							pixelColor.b = Ext::Ftoi32_Fast(curClr.z);
						}

						// FIXME：OpenMP并行同步问题 [8/5/2013 mavaL]
						zBuffer[curX] = z;
						destBuffer[curX] = pixelColor.color;
					}

					Common::Add_Vec3_By_Vec3(curClr, curClr, deltaClr);
					Common::Add_Vec2_By_Vec2(curUV, curUV, deltaUV);
					Common::Add_Vec3_By_Vec3(curPW, curPW, deltaPW);
					Common::Add_Vec3_By_Vec3(curN, curN, deltaN);
					z += dz;
					w += dw;
				}
			}

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

			destBuffer += lpitch;
			zBuffer += lpitch;
		}
	}
}