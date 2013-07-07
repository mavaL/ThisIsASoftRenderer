#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"

namespace SR
{
	void RasWireFrame::RasterizeTriangleList( const SRenderList& renderList )
	{
		//each triangle
		int nFace = renderList.indexes.size() / 3;

		for (int iFace=0; iFace<nFace; ++iFace)
		{
			const Index idx1 = renderList.indexes[iFace * 3 + 0];
			const Index idx2 = renderList.indexes[iFace * 3 + 1];
			const Index idx3 = renderList.indexes[iFace * 3 + 2];

			const VEC4& p0 = renderList.verts[idx1].pos;
			const VEC4& p1 = renderList.verts[idx2].pos;
			const VEC4& p2 = renderList.verts[idx3].pos;

			//each line
			RenderUtil::DrawLine_Bresenahams(p0.x, p0.y, p1.x, p1.y, 0xffffffff, true);
			RenderUtil::DrawLine_Bresenahams(p1.x, p1.y, p2.x, p2.y, 0xffffffff, true);
			RenderUtil::DrawLine_Bresenahams(p0.x, p0.y, p2.x, p2.y, 0xffffffff, true);
		}
	}
 }