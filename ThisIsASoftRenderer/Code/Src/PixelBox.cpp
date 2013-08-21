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
}

