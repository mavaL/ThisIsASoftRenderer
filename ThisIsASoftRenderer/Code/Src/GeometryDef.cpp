#include "stdafx.h"
#include "GeometryDef.h"

namespace SR
{
	SColor SColor::WHITE	=	SColor(0xffffffff);
	SColor SColor::BLACK	=	SColor(0);

	void STexture::LoadTexture( const STRING& filename )
	{
		assert(texName.empty() && !pData);
		Gdiplus::Bitmap bm(Ext::AnsiToUnicode(filename.c_str()).c_str(), TRUE);
		HBITMAP hbm;
		bm.GetHBITMAP(Gdiplus::Color::Black, &hbm);

		CBitmap* cbm;
		BITMAP bitmap;
		cbm = CBitmap::FromHandle(hbm);
		cbm->GetBitmap(&bitmap);

		pData = new Common::PixelBox(&bitmap, true);
		texName = filename;
	}

	SR::SColor STexture::Tex2D_Point( const VEC2& uv ) const
	{
		DWORD* pTexData = (DWORD*)pData->GetDataPointer();

		int w = pData->GetWidth() - 1;
		int h = pData->GetHeight() - 1;

		int x = (int)(uv.x * w);
		int y = (int)((1 - uv.y) * h);	//.bmp¸ñÊ½!

		x = Ext::Clamp(x, 0, w);
		y = Ext::Clamp(y, 0, h);

		return std::move(SColor(pTexData[y * pData->GetWidth() + x]));
	}

	STexture::STexture( const STexture& rhs )
	:texName("")
	,pData(nullptr)
	{
		*this = rhs;
	}

	STexture& STexture::operator=( const STexture& rhs )
	{
		SAFE_DELETE(pData);
		LoadTexture(rhs.texName);
		return *this;
	}
}