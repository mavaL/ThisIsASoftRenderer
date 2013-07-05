#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"

namespace SR
{
	void RasWireFrame::RasterizeTriangleList( const TriangleList& triList )
	{
		//each triangle
		for (size_t iTri=0; iTri<triList.size(); ++iTri)
		{
			const STriangle& triangle = triList[iTri];
			const Common::SVector3& p0 = triangle.vert[0].pos;
			const Common::SVector3& p1 = triangle.vert[1].pos;
			const Common::SVector3& p2 = triangle.vert[2].pos;

			//each line
			RenderUtil::DrawClipLine_DDA(p0.x, p0.y, p1.x, p1.y, 0xffffffff);
			RenderUtil::DrawClipLine_DDA(p1.x, p1.y, p2.x, p2.y, 0xffffffff);
			RenderUtil::DrawClipLine_DDA(p0.x, p0.y, p2.x, p2.y, 0xffffffff);
		}
	}
 }