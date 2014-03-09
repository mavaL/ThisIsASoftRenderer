#include "stdafx.h"
#include "PixelBox.h"
#include "Utility.h"

namespace SR
{
	PixelBox::PixelBox( int width, int height, int bytesPerPixel )
	:m_width(width)
	,m_height(height)
	,m_bytesPerPixel(bytesPerPixel)
	,m_pitch(width * bytesPerPixel)
	{
		m_data = new char[width * height * bytesPerPixel];
		ZeroMemory(m_data, width * height * bytesPerPixel);
		m_ownData = true;
	}

	PixelBox::PixelBox( BITMAP* bm, bool bCopyData )
	{
		m_width = bm->bmWidth;
		m_height = bm->bmHeight;
		m_bytesPerPixel = bm->bmBitsPixel / 8;
		m_pitch = bm->bmWidthBytes;

		if(bCopyData)
		{
			m_data = new char[m_width * m_height * m_bytesPerPixel];
			memcpy(&m_data[0], bm->bmBits, m_width * m_height * m_bytesPerPixel);
			m_ownData = true;
		}
		else
		{
			m_data = (char*)bm->bmBits;
			m_ownData = false;
		}
	}

	PixelBox::~PixelBox()
	{
		if(m_ownData)
			SAFE_DELETE_ARRAY(m_data);
	}

	SR::SColor PixelBox::GetPixelAt( int x, int y ) const
	{
		assert(x >=0 && x < m_width);
		assert(y >=0 && x < m_height);

		SColor result;
		switch (m_bytesPerPixel)
		{
		case 3:
			{
				char* pData = m_data + y * m_pitch + x * 3;
				result.b = (float)pData[0];
				result.g = (float)pData[1];
				result.r = (float)pData[2];
			}
			break;

		case 4:
			{
				DWORD* pData = (DWORD*)m_data;
				DWORD color = pData[y*m_width+x];
				result.SetAsInt(color);
			}
			break;

		default: assert(0); break;
		}

		return std::move(result);
	}

	void PixelBox::SetPixelAt( int x, int y, SColor p )
	{
		assert(x >=0 && x < m_width);
		assert(y >=0 && x < m_height);

		switch (m_bytesPerPixel)
		{
		case 3:
			{
				char* pData = m_data + y * m_pitch + x * 3;
				pData[0] = Ext::Ftoi32_Fast(p.b);
				pData[1] = Ext::Ftoi32_Fast(p.g);
				pData[2] = Ext::Ftoi32_Fast(p.r);
			}
			break;

		case 4:
			{
				DWORD* pData = (DWORD*)m_data;
				pData[y*m_width+x] = p.GetAsInt();
			}
			break;

		default: assert(0); break;
		}
	}

	//@brief 获取图片文件的编码方式，支持bmp、jpg、jpeg、gif、tiff和png等格式图片
	//@date   1-13-2009  
	//@param [in]  format 图片格式 值可以为以下几种
	//@"image/bmp"
	//@"image/jpeg"
	//@"image/gif"
	//@"image/tiff"
	//@"image/png"
	//@param [in]  pClsid
	//@return  成功则返回值 >= 0，失败则返回值为-1
	static int GetEncoderClsid(const WCHAR* format, CLSID *pClsid)
	{
		using namespace Gdiplus;

		int nRet = -1;
		ImageCodecInfo* pCodecInfo = NULL;
		UINT nNum = 0,nSize = 0;
		GetImageEncodersSize(&nNum,&nSize);

		if (nSize<0)
		{
			return nRet;
		}

		pCodecInfo= new ImageCodecInfo[nSize];

		if (pCodecInfo==NULL)
		{
			return nRet;
		}

		GetImageEncoders(nNum,nSize,pCodecInfo);

		for (UINT i=0; i<nNum;i++)
		{
			if (wcscmp(pCodecInfo[i].MimeType,format)==0)
			{
				*pClsid= pCodecInfo[i].Clsid;
				nRet = i;
				delete[] pCodecInfo;
				return nRet;
			}
			else
			{
				continue;
			}
		}

		delete[] pCodecInfo;
		return nRet;
	}

	void PixelBox::SaveToFile( const STRING& filename )
	{
		Gdiplus::Bitmap bm(m_width, m_height, m_pitch, PixelFormat32bppRGB, (BYTE*)m_data); 
		CLSID clsid;
		GetEncoderClsid(Ext::AnsiToUnicode("image/bmp").c_str(), &clsid);
		bm.Save(Ext::AnsiToUnicode(filename.c_str()).c_str(), &clsid);
	}

}

