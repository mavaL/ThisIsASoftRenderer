#include "stdafx.h"
#include "Utility.h"

namespace Ext
{
	std::wstring AnsiToUnicode( const char* src )
	{
		WCHAR dest[MAX_PATH];
		MultiByteToWideChar( CP_ACP, 0, src,
			strlen(src)+1, dest, sizeof(dest)/sizeof(dest[0]) );

		return std::wstring(dest);
	}

	STRING UnicodeToEngine( const WCHAR* src )
	{
		char dest[MAX_PATH];
		WideCharToMultiByte( CP_ACP, 0, src, -1,
			dest, MAX_PATH, NULL, NULL );

		return std::string(dest);
	}

}