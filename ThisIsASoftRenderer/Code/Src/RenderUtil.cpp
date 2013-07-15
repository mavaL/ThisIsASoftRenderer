#include "stdafx.h"
#include "Renderer.h"
#include "PixelBox.h"

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

		UCHAR* vb_start = (UCHAR*)g_renderer.m_backBuffer->GetDataPointer();
		int bytesPerPixel = g_renderer.m_backBuffer->GetBytesPerPixel();
		int lpitch = g_renderer.m_backBuffer->GetWidth() * bytesPerPixel;

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

		UCHAR* vb_start = (UCHAR*)g_renderer.m_backBuffer->GetDataPointer();
		int bytesPerPixel = g_renderer.m_backBuffer->GetBytesPerPixel();
		int lpitch = g_renderer.m_backBuffer->GetWidth() * bytesPerPixel;

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

	void RenderUtil::DrawText( int x, int y, const STRING& text, const COLORREF color )
	{
		//TODO.. 输出摄像机坐标
		HDC dc = GetDC(g_renderer.m_hwnd);
		SetBkMode(dc, TRANSPARENT);
		SetTextColor(dc, color);
		TextOut(dc, x, y, text.c_str(), text.length());
		ReleaseDC(g_renderer.m_hwnd, dc);
	}

	float RenderUtil::ComputeBoundingRadius( const VertexBuffer& verts )
	{
		float maxSqRadius = 0;

		for (size_t i=0; i<verts.size(); ++i)
		{
			const VEC4& pos = verts[i].pos;
			float SqRadius = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z;
			if(SqRadius > maxSqRadius)
				maxSqRadius = SqRadius;
		}

		return std::sqrt(maxSqRadius);
	}

	bool RenderUtil::PreDrawTriangle( const SVertex*& vert0, const SVertex*& vert1, const SVertex*& vert2, eTriangleShape& retType )
	{
		int x0 = (int)vert0->pos.x;
		int x1 = (int)vert1->pos.x;
		int x2 = (int)vert2->pos.x;
		int y0 = (int)vert0->pos.y;
		int y1 = (int)vert1->pos.y;
		int y2 = (int)vert2->pos.y;

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
		if(vert0->pos.y > vert1->pos.y)
		{
			Ext::Swap(vert0, vert1);
		}
		if(vert0->pos.y > vert2->pos.y)
		{
			Ext::Swap(vert0, vert2);
		}
		if(vert2->pos.y > vert1->pos.y)
		{
			Ext::Swap(vert2, vert1);
		}

		/* 2.若三角面是平底三角形
					p0
					/\
				   /  \
				  /____\
				 p1		p2
		*/
		if(vert1->pos.y == vert2->pos.y)
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
		else if(vert0->pos.y == vert2->pos.y)
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
		assert(y1 == y2);

		float curLX = x0, curRX = x0, curY = y0;
		float left_incX = (x1-x0)/(y1-y0);
		float right_incX = (x2-x0)/(y2-y0);

		while (curY <= y1)
		{
			int lineX0 = floor(curLX + 0.5f);
			int lineX1 = floor(curRX + 0.5f);

			RenderUtil::DrawLine_Bresenahams(lineX0, curY, lineX1, curY, color, true);

			++curY;
			curLX += left_incX;
			curRX += right_incX;
		}
	}

	void RenderUtil::DrawTopTri_Scanline( float x0, float y0, float x1, float y1, float x2, float y2, SColor color )
	{
		assert(y0 == y2);

		float curLX = x0, curRX = x2, curY = y0;
		float left_incX = (x1-x0)/(y1-y0);
		float right_incX = (x1-x2)/(y1-y2);

		while (curY <= y1)
		{
			int lineX0 = floor(curLX + 0.5f);
			int lineX1 = floor(curRX + 0.5f);

			RenderUtil::DrawLine_Bresenahams(lineX0, curY, lineX1, curY, color, true);

			++curY;
			curLX += left_incX;
			curRX += right_incX;
		}
	}

	void RenderUtil::SortTris_PainterAlgorithm( const VertexBuffer& verts, FaceList& faces )
	{
		//用画家算法对所有面进行排序(根据三角面3个顶点的平均z值)
		//NB: 该算法在某些面重叠的情况下是不正确的,见<<3D编程大师技巧>>
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

	void RenderUtil::DrawTriangle_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2 )
	{
		eTriangleShape triType;
		if(!PreDrawTriangle(vert0, vert1, vert2, triType))
			return;

		switch (triType)
		{
		case eTriangleShape_Bottom: DrawBottomTri_Scanline_V2(vert0, vert1, vert2); break;
		case eTriangleShape_Top: DrawTopTri_Scanline_V2(vert0, vert1, vert2); break;
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
				vert3.pos.x = x0 + (y2-y0) * (x1-x0) / (y1-y0);
				vert3.pos.y = y2;
				vert3.color.a = 255;
				float t = (y2-y0)/(y1-y0);
				vert3.color.r = Ext::LinearLerp(vert0->color.r, vert1->color.r, t);
				vert3.color.g = Ext::LinearLerp(vert0->color.g, vert1->color.g, t);
				vert3.color.b = Ext::LinearLerp(vert0->color.b, vert1->color.b, t);

				DrawBottomTri_Scanline_V2(vert0, &vert3, vert2);
				DrawTopTri_Scanline_V2(&vert3, vert1, vert2);
			}
			break;
		default: assert(0);
		}
	}

	void RenderUtil::DrawBottomTri_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2 )
	{
		//NB: 为了正确插值颜色和UV,要保持x坐标有序
		if(vert1->pos.x > vert2->pos.x)
			Ext::Swap(vert1, vert2);

		float x0 = vert0->pos.x;
		float x1 = vert1->pos.x;
		float x2 = vert2->pos.x;
		float y0 = vert0->pos.y;
		float y1 = vert1->pos.y;
		float y2 = vert2->pos.y;
		SColor c0 = vert0->color;
		SColor c1 = vert1->color;
		SColor c2 = vert2->color;

		if(y0 < min_clip_y)
		{
			int i = 0;
		}

		assert(y1 == y2);

		//位置坐标及增量
		float curLX = x0, curRX = x0, curY = y0;
		float left_incX = (x1-x0)/(y1-y0);
		float right_incX = (x2-x0)/(y2-y0);
		//当前两端点颜色分量及增量
		float rl = c0.r, rr = c0.r;
		float gl = c0.g, gr = c0.g;
		float bl = c0.b, br = c0.b;
		float drl = (c1.r-c0.r)/(y1-y0), drr = (c2.r-c0.r)/(y2-y0);
		float dgl = (c1.g-c0.g)/(y1-y0), dgr = (c2.g-c0.g)/(y2-y0);
		float dbl = (c1.b-c0.b)/(y1-y0), dbr = (c2.b-c0.b)/(y2-y0);

		//定位输出位置
		DWORD* vb_start = (DWORD*)g_renderer.m_backBuffer->GetDataPointer();
		int lpitch = g_renderer.m_backBuffer->GetWidth();
		DWORD* destBuffer = vb_start + (int)curY*lpitch;

		{
			//单独处理第一行,不然下面除0错误
			destBuffer[(int)(curLX + 0.5f)] = c0.color;

			++curY;
			curLX += left_incX;
			curRX += right_incX;
			destBuffer += lpitch;
			rl += drl;
			rr += drr;
			gl += dgl;
			gr += dgr;
			bl += dbl;
			br += dbr;
		}

		while (curY <= y1)
		{
			int lineX0 = (int)(curLX + 0.5f);
			int lineX1 = (int)(curRX + 0.5f);
			int lineY = (int)curY;

			float invdx = 1/((float)lineX1-lineX0); // <-除0
			float dr = (rr-rl)*invdx;
			float dg = (gr-gl)*invdx;
			float db = (br-bl)*invdx;

			float r = rl, g = gl, b = bl;
			
			//画水平直线
			for (int curX=lineX0; curX<=lineX1; ++curX)
			{
				SColor clr;
				clr.a = 255; clr.r = (BYTE)r; clr.g = (BYTE)g; clr.b = (BYTE)b;

				destBuffer[curX] = clr.color;

				r += dr;
				g += dg;
				b += db;
			}

			++curY;
			curLX += left_incX;
			curRX += right_incX;
			destBuffer += lpitch;
			rl += drl;
			rr += drr;
			gl += dgl;
			gr += dgr;
			bl += dbl;
			br += dbr;
		}
	}

	void RenderUtil::DrawTopTri_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2 )
	{
		//NB: 为了正确插值颜色和UV,要保持x坐标有序
		if(vert0->pos.x > vert2->pos.x)
			Ext::Swap(vert0, vert2);

		float x0 = vert0->pos.x;
		float x1 = vert1->pos.x;
		float x2 = vert2->pos.x;
		float y0 = vert0->pos.y;
		float y1 = vert1->pos.y;
		float y2 = vert2->pos.y;
		SColor c0 = vert0->color;
		SColor c1 = vert1->color;
		SColor c2 = vert2->color;

		assert(y0 == y2);

		//位置坐标及增量
		float curLX = x0, curRX = x2, curY = y0;
		float left_incX = (x1-x0)/(y1-y0);
		float right_incX = (x1-x2)/(y1-y2);
		//当前两端点颜色分量及增量
		float rl = c0.r, rr = c2.r;
		float gl = c0.g, gr = c2.g;
		float bl = c0.b, br = c2.b;
		float drl = (c1.r-c0.r)/(y1-y0), drr = (c1.r-c2.r)/(y1-y2);
		float dgl = (c1.g-c0.g)/(y1-y0), dgr = (c1.g-c2.g)/(y1-y2);
		float dbl = (c1.b-c0.b)/(y1-y0), dbr = (c1.b-c2.b)/(y1-y2);

		//定位输出位置
		DWORD* vb_start = (DWORD*)g_renderer.m_backBuffer->GetDataPointer();
		int lpitch = g_renderer.m_backBuffer->GetWidth();
		DWORD* destBuffer = vb_start + (int)curY*lpitch;

		while (curY <= y1)
		{
			int lineX0 = (int)(curLX + 0.5f);
			int lineX1 = (int)(curRX + 0.5f);
			int lineY = (int)curY;

			float invdx = 1/((float)lineX1-lineX0);
			float dr = (rr-rl)*invdx;
			float dg = (gr-gl)*invdx;
			float db = (br-bl)*invdx;

			float r = rl, g = gl, b = bl;

			//画水平直线
			for (int curX=lineX0; curX<=lineX1; ++curX)
			{
				SColor clr;
				clr.a = 255; clr.r = (BYTE)r; clr.g = (BYTE)g; clr.b = (BYTE)b;

				destBuffer[curX] = clr.color;

				r += dr;
				g += dg;
				b += db;
			}

			++curY;
			curLX += left_incX;
			curRX += right_incX;
			destBuffer += lpitch;
			rl += drl;
			rr += drr;
			gl += dgl;
			gr += dgr;
			bl += dbl;
			br += dbr;
		}
	}
}