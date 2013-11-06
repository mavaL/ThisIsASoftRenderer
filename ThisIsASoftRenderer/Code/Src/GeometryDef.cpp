#include "stdafx.h"
#include "GeometryDef.h"
#include "Profiler.h"

namespace SR
{
	SColor SColor::WHITE	=	SColor(1.0f, 1.0f, 1.0f);
	SColor SColor::BLACK	=	SColor(0.0f, 0.0f, 0.0f);
	SColor SColor::BLUE		=	SColor(0.0f, 0.0f, 1.0f);
	SColor SColor::RED		=	SColor(1.0f, 0.0f, 0.0f);
	SColor SColor::NICE_BLUE =	SColor(0.0f, 0.125f, 0.3f);


	STexture::STexture( const STexture& rhs )
		:texName("")
	{
		*this = rhs;
	}

	STexture::~STexture()
	{
		std::for_each(texData.begin(), texData.end(), std::default_delete<PixelBox>());
		texData.clear();
	}

	STexture& STexture::operator=( const STexture& rhs )
	{
		this->~STexture();
		if(!rhs.texName.empty())
			LoadTexture(rhs.texName, false);
		return *this;
	}

	void STexture::LoadTexture( const STRING& filename, bool bmipmap )
	{
		assert(texName.empty() && texData.empty());
		Gdiplus::Bitmap bm(Ext::AnsiToUnicode(filename.c_str()).c_str(), TRUE);
		HBITMAP hbm;
		bm.GetHBITMAP(Gdiplus::Color::Black, &hbm);

		CBitmap* cbm;
		BITMAP bitmap;
		cbm = CBitmap::FromHandle(hbm);
		cbm->GetBitmap(&bitmap);

		SR::PixelBox* baseLevel = new SR::PixelBox(&bitmap, true);
		texData.push_back(baseLevel);
		texName = filename;

		if (bmipmap)
		{
			GenMipMaps();
			bMipMap = true;
		}
	}

	void STexture::Tex2D_Point( const VEC2& uv, SColor& ret, int mip ) const
	{
		PixelBox* mipLevel = texData[mip];
		DWORD* pTexData = (DWORD*)mipLevel->GetDataPointer();

		VEC2 tmpUV = uv;
		//Wrap mode
		tmpUV.x -= Ext::Floor32_Fast(tmpUV.x);
		tmpUV.y -= Ext::Floor32_Fast(tmpUV.y);

		int w = mipLevel->GetWidth() - 1;
		int h = mipLevel->GetHeight() - 1;

		int x = Ext::Ftoi32_Fast(tmpUV.x * w);
		int y = Ext::Ftoi32_Fast(tmpUV.y * h);

		ret.SetAsInt(pTexData[y * mipLevel->GetWidth() + x]);

#if USE_PROFILER == 1
		if(bMipMap)
			ret *= g_env.profiler->m_vecMipColor[mip];
#endif
	}

	void STexture::Tex2D_Bilinear( const VEC2& uv, SColor& ret, int mip ) const
	{
		PixelBox* mipLevel = texData[mip];
		DWORD* pTexData = (DWORD*)mipLevel->GetDataPointer();

		VEC2 tmpUV = uv;
		//Wrap mode
		tmpUV.x -= Ext::Floor32_Fast(tmpUV.x);
		tmpUV.y -= Ext::Floor32_Fast(tmpUV.y);

		float intU = (mipLevel->GetWidth() - 1) * tmpUV.x;
		float intV = (mipLevel->GetHeight() - 1) * tmpUV.y;

		int u_l = Ext::Floor32_Fast(intU);
		int u_r = Ext::Ceil32_Fast(intU);
		int v_t = Ext::Floor32_Fast(intV);
		int v_b = Ext::Ceil32_Fast(intV);

		SColor color[4];
		color[0].SetAsInt(pTexData[v_t * mipLevel->GetWidth() + u_l]);
		color[1].SetAsInt(pTexData[v_t * mipLevel->GetWidth() + u_r]);
		color[2].SetAsInt(pTexData[v_b * mipLevel->GetWidth() + u_l]);
		color[3].SetAsInt(pTexData[v_b * mipLevel->GetWidth() + u_r]);

		float frac_u = intU - u_l;
		float frac_v = intV - v_t;

		float weight[4];
		weight[0] = (1 - frac_u)	* (1 - frac_v);
		weight[1] = frac_u			* (1 - frac_v);
		weight[2] = (1 - frac_u)	* frac_v;
		weight[3] = frac_u			* frac_v;

		ret = SColor::BLACK;
		//blend the four neighbour colors
		color[0] *= weight[0];
		color[1] *= weight[1];
		color[2] *= weight[2];
		color[3] *= weight[3];

		ret += color[0];
		ret += color[1];
		ret += color[2];
		ret += color[3];

#if USE_PROFILER == 1
		if(bMipMap)
			ret *= g_env.profiler->m_vecMipColor[mip];
#endif
	}

	void STexture::GenMipMaps()
	{
		const SR::PixelBox* baseLevel = texData[0];

		bool bPowOfTwo_W = ((baseLevel->GetWidth()-1) & baseLevel->GetWidth()) == 0;
		bool bPowOfTwo_H = ((baseLevel->GetHeight()-1) & baseLevel->GetHeight()) == 0;
		if (!bPowOfTwo_W || !bPowOfTwo_H)
		{
			MessageBoxA(nullptr, "Only support pow2 mipmap!", "Warning", MB_OK|MB_ICONWARNING);
			return;
		}

		// Generate mip of each level
		int w = baseLevel->GetWidth();
		int h = baseLevel->GetHeight();
		int preLevel = 0;
		while(w!=1 || h!=1)
		{
			w = max(1, w/2);
			h = max(1, h/2);

			PixelBox* mipLevel = new PixelBox(w, h, baseLevel->GetBytesPerPixel());
			texData.push_back(mipLevel);
			PixelBox* srcLevel = texData[preLevel++];

			// Simple box filter..
			int srcW = srcLevel->GetWidth();
			int srcH = srcLevel->GetHeight();
			int dest_i = 0;
			for (int i=0; i<srcH-1; i+=2,++dest_i)
			{
				int dest_j = 0;
				for (int j=0; j<srcW-1; j+=2,++dest_j)
				{
					SColor c1 = srcLevel->GetPixelAt(j, i);
					c1 += srcLevel->GetPixelAt(j+1, i);
					c1 += srcLevel->GetPixelAt(j, i+1);
					c1 += srcLevel->GetPixelAt(j+1, i+1);
					c1 *= 0.25f;

					mipLevel->SetPixelAt(dest_j, dest_i, c1);
				}
			}

#if USE_PROFILER == 1
			char tmp[64];
			sprintf_s(tmp, sizeof(tmp), "mipmap%d.bmp", preLevel);
			mipLevel->SaveToFile(tmp);
#endif
		}
	}

	PixelBox* STexture::GetSurface( int i )
	{
		assert(i>=0 && i<texData.size());
		return texData[i];
	}


}