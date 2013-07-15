#include "stdafx.h"
#include "PixelBox.h"

namespace Common
{
	PixelBox::PixelBox( int width, int height, int bytesPerPixel )
	:m_width(width)
	,m_height(height)
	,m_bytesPerPixel(bytesPerPixel)
	,m_pitch(m_width * m_bytesPerPixel)
	{
		m_data.resize(width * height * bytesPerPixel);
	}

	PixelBox::PixelBox( BITMAP* bm )
	{
		m_width = bm->bmWidth;
		m_height = bm->bmHeight;
		m_bytesPerPixel = bm->bmBitsPixel / 8;
		m_pitch = bm->bmWidthBytes;
		assert(m_bytesPerPixel == 4 && "Error, currently only support 32bit texture!");

		m_data.resize(m_width * m_height * m_bytesPerPixel);
		memcpy(&m_data[0], bm->bmBits, m_width * m_height * m_bytesPerPixel);
	}


	void* PixelBox::GetDataPointer()
	{
		return &m_data[0];
	}
}

