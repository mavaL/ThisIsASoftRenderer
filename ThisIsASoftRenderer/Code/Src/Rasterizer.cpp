#include "stdafx.h"
#include "Rasterizer.h"
#include "MathDef.h"
#include "Renderer.h"
#include "RenderUtil.h"
#include "RenderObject.h"
#include "Profiler.h"

namespace SR
{
	void Rasterizer::RasterizeTriangleList( SRenderContext& context )
	{
		FaceList& faces = context.faces;
		VertexBuffer& verts = context.verts;

		//each triangle
		size_t nFace = faces.size();

		for (size_t iFace=0; iFace<nFace; ++iFace)
		{
			const SFace& face = faces[iFace];
			if(face.IsBackface || face.bCulled)
				continue;

			const Index idx1 = faces[iFace].index1;
			const Index idx2 = faces[iFace].index2;
			const Index idx3 = faces[iFace].index3;

			const SVertex& vert0 = verts[idx1];
			const SVertex& vert1 = verts[idx2];
			const SVertex& vert2 = verts[idx3];

			assert(vert0.bActive && vert1.bActive && vert2.bActive && "Shit, this can't be true!");

			// Determine mip level of this triangle
			const SMaterial* pMat = context.pMaterial;
			if (pMat->pDiffuseMap && pMat->pDiffuseMap->bMipMap)
			{
				const float areaScreen = Ext::CalcAreaOfTriangle(
					VEC2(vert0.pos.x, vert0.pos.y),
					VEC2(vert1.pos.x, vert1.pos.y),
					VEC2(vert2.pos.x, vert2.pos.y));

				float ratio = face.texArea / areaScreen;
				context.texLod = Ext::Ftoi32_Fast(log(ratio) / log(4.0f) + pMat->pDiffuseMap->lodBias);
				//clamp it
				if(context.texLod < 0)
					context.texLod = 0;
				if(context.texLod >= pMat->pDiffuseMap->GetMipCount())
					context.texLod = pMat->pDiffuseMap->GetMipCount() - 1;
			}

			_RasterizeTriangle(vert0, vert1, vert2, face, context);
		}
	}

	void Rasterizer::RasTriangleSetup(SScanLinesData& rasData, const SVertex* v0, const SVertex* v1, const SVertex* v2, eTriangleShape type)
	{
		const VEC4& p0 = v0->pos;
		const VEC4& p1 = v1->pos;
		const VEC4& p2 = v2->pos;

		if (type == eTriangleShape_Bottom)
		{
			//左上填充规则:上
			rasData.curY = Ext::Ceil32_Fast(p0.y), rasData.endY = Ext::Ceil32_Fast(p1.y) - 1;
			//位置坐标及增量
			rasData.inv_dy_L = 1.0f / (p1.y - p0.y);
			rasData.inv_dy_R = 1.0f / (p2.y - p0.y);
			rasData.curP_L.Set(p0.x, p0.z, p0.w);
			rasData.curP_R.Set(p0.x, p0.z, p0.w);
			rasData.dp_L.Set((p1.x-p0.x)*rasData.inv_dy_L, (p1.z-p0.z)*rasData.inv_dy_L, (p1.w-p0.w)*rasData.inv_dy_L);
			rasData.dp_R.Set((p2.x-p0.x)*rasData.inv_dy_R, (p2.z-p0.z)*rasData.inv_dy_R, (p2.w-p0.w)*rasData.inv_dy_R);
		}
		else
		{
			//左上填充规则:上
			rasData.curY = Ext::Ceil32_Fast(p0.y), rasData.endY = Ext::Ceil32_Fast(p1.y) - 1;
			//屏幕坐标及增量
			rasData.inv_dy_L = 1.0f / (p1.y - p0.y);
			rasData.inv_dy_R = 1.0f / (p1.y - p2.y);
			rasData.curP_L.Set(p0.x, p0.z, p0.w);
			rasData.curP_R.Set(p2.x, p2.z, p2.w);
			rasData.dp_L.Set((p1.x-p0.x)*rasData.inv_dy_L, (p1.z-p0.z)*rasData.inv_dy_L, (p1.w-p0.w)*rasData.inv_dy_L);
			rasData.dp_R.Set((p1.x-p2.x)*rasData.inv_dy_R, (p1.z-p2.z)*rasData.inv_dy_R, (p1.w-p2.w)*rasData.inv_dy_R);
		}

		//裁剪区域裁剪y
		if(rasData.curY < min_clip_y)
		{
			rasData.clip_dy = min_clip_y - rasData.curY;
			rasData.curY = min_clip_y;
			Common::Add_Vec3_By_Vec3(rasData.curP_L, rasData.curP_L, Common::Multiply_Vec3_By_K(rasData.dp_L, rasData.clip_dy));
			Common::Add_Vec3_By_Vec3(rasData.curP_R, rasData.curP_R, Common::Multiply_Vec3_By_K(rasData.dp_R, rasData.clip_dy));
			rasData.bClipY = true;
		}
		else
		{
			rasData.bClipY = false;
		}
		if(rasData.endY > max_clip_y)
		{
			rasData.endY = max_clip_y;
		}
	}

	void Rasterizer::RasLineSetup( SScanLine& scanLine, const SScanLinesData& rasData )
	{
		scanLine.inv_dx = 1.0f / (scanLine.lineX1 - scanLine.lineX0);
		scanLine.dz = (rasData.curP_R.y - rasData.curP_L.y) * scanLine.inv_dx;
		scanLine.dw = (rasData.curP_R.z - rasData.curP_L.z) * scanLine.inv_dx;
		scanLine.z = rasData.curP_L.y;
		scanLine.w = rasData.curP_L.z;

		//裁剪区域裁剪x
		if(scanLine.lineX0 < min_clip_x)
		{
			scanLine.bClipX = true;
			scanLine.clip_dx = min_clip_x-scanLine.lineX0;
			scanLine.lineX0 = min_clip_x;			
			scanLine.z += scanLine.clip_dx*scanLine.dz;
			scanLine.w += scanLine.clip_dx*scanLine.dw;
		}
		else
		{
			scanLine.clip_dx = false;
		}
	}
}