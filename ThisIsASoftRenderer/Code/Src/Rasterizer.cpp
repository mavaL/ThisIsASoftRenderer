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
		const VEC3 p0(v0->pos.x, v0->pos.z, v0->pos.w);
		const VEC3 p1(v1->pos.x, v1->pos.z, v1->pos.w);
		const VEC3 p2(v2->pos.x, v2->pos.z, v2->pos.w);

		float y0 = v0->pos.y;
		float y1 = v1->pos.y;
		float y2 = v2->pos.y;

		if (type == eTriangleShape_Bottom)
		{
			//左上填充规则:上
			rasData.curY = Ext::Ceil32_Fast(y0), rasData.endY = Ext::Ceil32_Fast(y1) - 1;
			//位置坐标及增量
			rasData.inv_dy_L = 1.0f / (y1 - y0);
			rasData.inv_dy_R = 1.0f / (y2 - y0);
			rasData.curP_L = p0;
			rasData.curP_R = p0;
			rasData.dp_L = Common::Sub_Vec3_By_Vec3(p1, p0);
			rasData.dp_R = Common::Sub_Vec3_By_Vec3(p2, p0);
		}
		else
		{
			//左上填充规则:上
			rasData.curY = Ext::Ceil32_Fast(y0), rasData.endY = Ext::Ceil32_Fast(y1) - 1;
			//屏幕坐标及增量
			rasData.inv_dy_L = 1.0f / (y1 - y0);
			rasData.inv_dy_R = 1.0f / (y1 - y2);
			rasData.curP_L = p0;
			rasData.curP_R = p2;
			rasData.dp_L = Common::Sub_Vec3_By_Vec3(p1, p0);
			rasData.dp_R = Common::Sub_Vec3_By_Vec3(p1, p2);
		}
		Common::Multiply_Vec3_By_K(rasData.dp_L, rasData.dp_L, rasData.inv_dy_L);
		Common::Multiply_Vec3_By_K(rasData.dp_R, rasData.dp_R, rasData.inv_dy_R);

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
		scanLine.dzdw.Set(rasData.curP_R.y-rasData.curP_L.y, rasData.curP_R.z-rasData.curP_L.z);
		scanLine.zw.Set(rasData.curP_L.y, rasData.curP_L.z);
		Common::Multiply_Vec2_By_K(scanLine.dzdw, scanLine.dzdw, scanLine.inv_dx);

		//裁剪区域裁剪x
		if(scanLine.lineX0 < min_clip_x)
		{
			scanLine.bClipX = true;
			scanLine.clip_dx = min_clip_x-scanLine.lineX0;
			scanLine.lineX0 = min_clip_x;			
			Common::Add_Vec2_By_Vec2(scanLine.zw, scanLine.zw, Common::Multiply_Vec2_By_K(scanLine.dzdw, scanLine.clip_dx));
		}
		else
		{
			scanLine.clip_dx = false;
		}
	}

	bool Rasterizer::DoZTest( float z, float zbuffer1, float zbuffer2, SMaterial* pMaterial )
	{
		bool bZPassed = false;
		eZFunc zfunc = g_env.renderer->GetCurZFunc();

		switch (zfunc)
		{
		case eZFunc_Less:		bZPassed = z < zbuffer1; break;
		case eZFunc_Greater:	bZPassed = z > zbuffer1; break;
		case eZFunc_Always:		bZPassed = true; break;
		default: assert(0); return false;
		}

#if USE_OIT == 0
		return bZPassed;
#endif

		if(!bZPassed || !pMaterial->bTransparent)
			return bZPassed;

		// Transparent material
		zfunc = g_env.renderer->GetAnotherZFunc();

		switch (zfunc)
		{
		case eZFunc_Less:		return z < zbuffer2; break;
		case eZFunc_Greater:	return z > zbuffer2; break;
		case eZFunc_Always:		return true;
		default: assert(0); return false;
		}
	}
}