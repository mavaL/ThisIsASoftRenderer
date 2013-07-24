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
		int x0 = Ext::Floor32_Fast(vert0->pos.x);
		int x1 = Ext::Floor32_Fast(vert1->pos.x);
		int x2 = Ext::Floor32_Fast(vert2->pos.x);
		int y0 = Ext::Floor32_Fast(vert0->pos.y);
		int y1 = Ext::Floor32_Fast(vert1->pos.y);
		int y2 = Ext::Floor32_Fast(vert2->pos.y);

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

		assert(Ext::Floor32_Fast(y1) == Ext::Floor32_Fast(y2));

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
	}

	void RenderUtil::DrawTopTri_Scanline( float x0, float y0, float x1, float y1, float x2, float y2, SColor color )
	{
		//NB: 为了填充规则,要保持x坐标有序
		if(x0 > x2)
		{
			Ext::Swap(x0, x2);
			Ext::Swap(y0, y2);
		}

		assert(Ext::Floor32_Fast(y0) == Ext::Floor32_Fast(y2));

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

	void RenderUtil::DrawTriangle_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, bool bTextured, const STexture* tex )
	{
		eTriangleShape triType;
		if(!PreDrawTriangle(vert0, vert1, vert2, triType))
			return;

		switch (triType)
		{
		case eTriangleShape_Bottom: DrawBottomTri_Scanline_V2(vert0, vert1, vert2, bTextured, tex); break;
		case eTriangleShape_Top: DrawTopTri_Scanline_V2(vert0, vert1, vert2, bTextured, tex); break;
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
				float z2 = vert2->pos.z;

				float t = (y2-y0)/(y1-y0);
				vert3.pos.x = x0 + (x1-x0)*t;
				vert3.pos.y = y2;
				vert3.pos.z = z0 + (z1-z0)*t;
				vert3.color.a = 255;
				vert3.color.r = Ext::LinearLerp(vert0->color.r, vert1->color.r, t);
				vert3.color.g = Ext::LinearLerp(vert0->color.g, vert1->color.g, t);
				vert3.color.b = Ext::LinearLerp(vert0->color.b, vert1->color.b, t);
				vert3.uv.x = Ext::LinearLerp(vert0->uv.x, vert1->uv.x, t);
				vert3.uv.y = Ext::LinearLerp(vert0->uv.y, vert1->uv.y, t);

				DrawBottomTri_Scanline_V2(vert0, &vert3, vert2, bTextured, tex);
				DrawTopTri_Scanline_V2(&vert3, vert1, vert2, bTextured, tex);
			}
			break;
		default: assert(0);
		}
	}

	void RenderUtil::DrawBottomTri_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, bool bTextured, const STexture* tex )
	{
		//NB: 为了正确插值和填充规则,要保持x坐标有序
		if(vert1->pos.x > vert2->pos.x)
			Ext::Swap(vert1, vert2);

		float x0 = vert0->pos.x;
		float x1 = vert1->pos.x;
		float x2 = vert2->pos.x;
		float y0 = vert0->pos.y;
		float y1 = vert1->pos.y;
		float y2 = vert2->pos.y;
		float z0 = vert0->pos.z;
		float z1 = vert1->pos.z;
		float z2 = vert2->pos.z;
		SColor c0 = vert0->color;
		SColor c1 = vert1->color;
		SColor c2 = vert2->color;
		float u0 = vert0->uv.x, v0 = vert0->uv.y;
		float u1 = vert1->uv.x, v1 = vert1->uv.y;
		float u2 = vert2->uv.x, v2 = vert2->uv.y;

		assert(Ext::Floor32_Fast(y1) == Ext::Floor32_Fast(y2));

		//位置坐标及增量
		float curLX = x0, curRX = x0, curLZ = z0, curRZ = z0;
		float inv_dy = 1.0f/(y1-y0);
		float left_incX = (x1-x0)*inv_dy;
		float right_incX = (x2-x0)*inv_dy;
		float left_incZ = (z1-z0)*inv_dy;
		float right_incZ = (z2-z0)*inv_dy;
		//左上填充规则
		int curY = Ext::Ceil32_Fast(y0), endY = Ext::Ceil32_Fast(y1) - 1;
		//当前两端点颜色分量及增量
		float rl = c0.r, rr = c0.r;
		float gl = c0.g, gr = c0.g;
		float bl = c0.b, br = c0.b;
		float drl = (c1.r-c0.r)*inv_dy, drr = (c2.r-c0.r)*inv_dy;
		float dgl = (c1.g-c0.g)*inv_dy, dgr = (c2.g-c0.g)*inv_dy;
		float dbl = (c1.b-c0.b)*inv_dy, dbr = (c2.b-c0.b)*inv_dy;
		//当前两端点uv分量及增量
		float curUL = u0, curVL = v0;
		float curUR = u0, curVR = v0;
		float dul = (u1-u0)*inv_dy, dvl = (v1-v0)*inv_dy;
		float dur = (u2-u0)*inv_dy, dvr = (v2-v0)*inv_dy;

		//裁剪区域裁剪y
		if(curY < min_clip_y)
		{
			float dy = min_clip_y-y0;
			curY = min_clip_y;
			curLX += left_incX*dy;
			curRX += right_incX*dy;
			curLZ += left_incZ*dy;
			curRZ += right_incZ*dy;
			rl += drl*dy; gl += dgl*dy; bl += dbl*dy;
			rr += drr*dy; gr += dgr*dy; br += dbr*dy;
			curUL += dul*dy; curVL += dvl*dy;
			curUR += dur*dy; curVR += dvr*dy;
		}
		if(endY > max_clip_y)
		{
			endY = max_clip_y;
		}

		//定位输出位置
		DWORD* vb_start = (DWORD*)g_env.renderer->m_backBuffer->GetDataPointer();
		int lpitch = g_env.renderer->m_backBuffer->GetWidth();
		DWORD* destBuffer = vb_start + curY*lpitch;
		float* zBuffer = (float*)g_env.renderer->m_zBuffer->GetDataPointer() + curY*lpitch;
		SColor pixelColor;
		float inv_texel = 1.0f/255;

		while (curY <= endY)
		{
			//左上填充规则
			int lineX0 = Ext::Ceil32_Fast(curLX);
			int lineX1 = Ext::Ceil32_Fast(curRX) - 1;

			if(lineX1 - lineX0 >= 0)
			{
				float invdx = 1/(curRX-curLX);
				float dr = (rr-rl)*invdx;
				float dg = (gr-gl)*invdx;
				float db = (br-bl)*invdx;
				float du = (curUR-curUL)*invdx;
				float dv = (curVR-curVL)*invdx;
				float dz = (curRZ-curLZ)*invdx;

				float r = rl, g = gl, b = bl;
				float u = curUL, v = curVL;
				float z = curLZ;

				//裁剪区域裁剪x
				if(lineX0 < min_clip_x)
				{
					const int dx = min_clip_x-lineX0;
					r += dx*dr;
					g += dx*dg;
					b += dx*db;
					u += dx*du;
					v += dx*dv;
					lineX0 = min_clip_x;
					z += dx*dz;
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
						if(bTextured && tex->pData)
						{
							pixelColor = tex->Tex2D_Point(VEC2(u, v));
							pixelColor.r *= r * inv_texel;
							pixelColor.g *= g * inv_texel;
							pixelColor.b *= b * inv_texel;
						}
						else
						{
							pixelColor.a = 255; 
							pixelColor.r = (BYTE)Ext::Ftoi32_Fast(r);
							pixelColor.g = (BYTE)Ext::Ftoi32_Fast(g); 
							pixelColor.b = (BYTE)Ext::Ftoi32_Fast(b);
						}

						destBuffer[curX] = pixelColor.color;
						zBuffer[curX] = z;
					}

					r += dr;
					g += dg;
					b += db;
					u += du;
					v += dv;
					z += dz;
				}
			}

			++curY;
			curLX += left_incX;
			curRX += right_incX;
			curLZ += left_incZ;
			curRZ += right_incZ;
			destBuffer += lpitch;
			zBuffer += lpitch;
			rl += drl; rr += drr;
			gl += dgl; gr += dgr;
			bl += dbl; br += dbr;
			curUL += dul; curUR += dur;
			curVL += dvl; curVR += dvr;
		}
	}

	void RenderUtil::DrawTopTri_Scanline_V2( const SVertex* vert0, const SVertex* vert1, const SVertex* vert2, bool bTextured, const STexture* tex )
	{
		//NB: 为了正确插值和填充规则,要保持x坐标有序
		if(vert0->pos.x > vert2->pos.x)
			Ext::Swap(vert0, vert2);

		float x0 = vert0->pos.x;
		float x1 = vert1->pos.x;
		float x2 = vert2->pos.x;
		float y0 = vert0->pos.y;
		float y1 = vert1->pos.y;
		float y2 = vert2->pos.y;
		float z0 = vert0->pos.z;
		float z1 = vert1->pos.z;
		float z2 = vert2->pos.z;
		SColor c0 = vert0->color;
		SColor c1 = vert1->color;
		SColor c2 = vert2->color;
		float u0 = vert0->uv.x, v0 = vert0->uv.y;
		float u1 = vert1->uv.x, v1 = vert1->uv.y;
		float u2 = vert2->uv.x, v2 = vert2->uv.y;

		assert(Ext::Floor32_Fast(y0) == Ext::Floor32_Fast(y2));

		//位置坐标及增量
		float inv_dy = 1.0f/(y1-y0);
		float curLX = x0, curRX = x2, curLZ = z0, curRZ = z2;
		float left_incX = (x1-x0)*inv_dy;
		float right_incX = (x1-x2)*inv_dy;
		float left_incZ = (z1-z0)*inv_dy;
		float right_incZ = (z1-z2)*inv_dy;
		//左上填充规则
		int curY = Ext::Ceil32_Fast(y0), endY = Ext::Ceil32_Fast(y1) - 1;
		//当前两端点颜色分量及增量
		float rl = c0.r, rr = c2.r;
		float gl = c0.g, gr = c2.g;
		float bl = c0.b, br = c2.b;
		float drl = (c1.r-c0.r)*inv_dy, drr = (c1.r-c2.r)*inv_dy;
		float dgl = (c1.g-c0.g)*inv_dy, dgr = (c1.g-c2.g)*inv_dy;
		float dbl = (c1.b-c0.b)*inv_dy, dbr = (c1.b-c2.b)*inv_dy;
		//当前两端点uv分量及增量
		float curUL = u0, curVL = v0;
		float curUR = u2, curVR = v2;
		float dul = (u1-u0)*inv_dy, dvl = (v1-v0)*inv_dy;
		float dur = (u1-u2)*inv_dy, dvr = (v1-v2)*inv_dy;

		//裁剪区域裁剪y
		if(curY < min_clip_y)
		{
			float dy = min_clip_y-y0;
			curY = min_clip_y;
			curLX += left_incX * dy;
			curRX += right_incX * dy;
			curLZ += left_incZ*dy;
			curRZ += right_incZ*dy;
			rl += drl*dy; gl += dgl*dy; bl += dbl*dy;
			rr += drr*dy; gr += dgr*dy; br += dbr*dy;
			curUL += dul*dy; curVL += dvl*dy;
			curUR += dur*dy; curVR += dvr*dy;
		}
		if(endY > max_clip_y)
		{
			endY = max_clip_y;
		}

		//定位输出位置
		DWORD* vb_start = (DWORD*)g_env.renderer->m_backBuffer->GetDataPointer();
		int lpitch = g_env.renderer->m_backBuffer->GetWidth();
		DWORD* destBuffer = vb_start + curY*lpitch;
		float* zBuffer = (float*)g_env.renderer->m_zBuffer->GetDataPointer() + curY*lpitch;
		SColor pixelColor;
		float inv_texel = 1.0f/255;

		while (curY <= endY)
		{
			//左上填充规则
			int lineX0 = Ext::Ceil32_Fast(curLX);
			int lineX1 = Ext::Ceil32_Fast(curRX) - 1;

			if(lineX1 - lineX0 >= 0)
			{
				float invdx = 1/(curRX-curLX);
				float dr = (rr-rl)*invdx;
				float dg = (gr-gl)*invdx;
				float db = (br-bl)*invdx;
				float du = (curUR-curUL)*invdx;
				float dv = (curVR-curVL)*invdx;
				float dz = (curRZ-curLZ)*invdx;

				float r = rl, g = gl, b = bl;
				float u = curUL, v = curVL;
				float z = curLZ;

				//裁剪区域裁剪x
				if(lineX0 < min_clip_x)
				{
					const int dx = min_clip_x-lineX0;
					r += dx*dr;
					g += dx*dg;
					b += dx*db;
					u += dx*du;
					v += dx*dv;
					lineX0 = min_clip_x;
					z += dx*dz;
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
						if(bTextured && tex->pData)
						{
							pixelColor = tex->Tex2D_Point(VEC2(u, v));
							pixelColor.r *= r * inv_texel;
							pixelColor.g *= g * inv_texel;
							pixelColor.b *= b * inv_texel;
						}
						else
						{
							pixelColor.r = (BYTE)Ext::Ftoi32_Fast(r);
							pixelColor.g = (BYTE)Ext::Ftoi32_Fast(g); 
							pixelColor.b = (BYTE)Ext::Ftoi32_Fast(b);
						}

						destBuffer[curX] = pixelColor.color;
						zBuffer[curX] = z;
					}

					r += dr;
					g += dg;
					b += db;
					u += du;
					v += dv;
					z += dz;
				}
			}

			++curY;
			curLX += left_incX;
			curRX += right_incX;
			curLZ += left_incZ;
			curRZ += right_incZ;
			destBuffer += lpitch;
			zBuffer += lpitch;
			rl += drl;
			rr += drr;
			gl += dgl;
			gr += dgr;
			bl += dbl;
			br += dbr;
			curUL += dul; curUR += dur;
			curVL += dvl; curVR += dvr;
		}
	}
}