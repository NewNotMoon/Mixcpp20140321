/*!
 * @file	NotMoon\Windows\Result.hpp
 * 
 * @brief	HRESULT 処理
 * 
 * Copyright (c) 2014 新々月. All rights reserved.
 */
#ifndef NOTMOON_WINDOWS_RESULT_HPP
#define NOTMOON_WINDOWS_RESULT_HPP

#include <Windows.h>
#include <sstream>

namespace NotMoon
{
	namespace Windows
	{
		/*!
		 * @fn outputResult
		 * @param result Result.
		 */
		static HRESULT outputResult( HRESULT result, const wchar_t* message, const wchar_t* data, const wchar_t* file, const wchar_t* func, int line )
		{
			static size_t count = 0;
			std::stringstream s;

			auto format = []( HRESULT& h )
				->std::string
			{
				LPVOID s;
				auto l = FormatMessageA
					(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					h,
					MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
					(LPSTR)&s,
					0,
					NULL
					);
				std::string r( (LPCSTR)s, l );
				LocalFree( s );
				auto p = r.find_last_not_of( '\n' );
				return r.substr( 0, p );
			};

			if( FAILED( result ) )
			{
				++count;
				s << "[失敗] ";							OutputDebugStringA( s.str().c_str() );	s.str( "" );	OutputDebugStringW( message );
				s << "\n\tDesc: " << format( result );	OutputDebugStringA( s.str().c_str() );	s.str( "" );
				s << "\n\tDeta: ";						OutputDebugStringA( s.str().c_str() );	s.str( "" );	OutputDebugStringW( data );
				s << "\n\tFile: ";						OutputDebugStringA( s.str().c_str() );	s.str( "" );	OutputDebugStringW( file );
				s << "\n\tLine: " << line;				OutputDebugStringA( s.str().c_str() );	s.str( "" );
				s << "\n\tFunc: ";						OutputDebugStringA( s.str().c_str() );	s.str( "" );	OutputDebugStringW( func );
				OutputDebugStringA( "\n" );

				return result;
			}
			else
			{
				return result;
			}
		};
	}
}

//#ifdef _DEBUG
#	define	UNICODE_TEXT_IMPL( x )					L##x
#	define	UNICODE_TEXT( x )						UNICODE_TEXT_IMPL(x)
#	define	RESULT( message, result )				NotMoon::Windows::outputResult( result, UNICODE_TEXT(message), L"", UNICODE_TEXT(__FILE__), UNICODE_TEXT(__FUNCTION__), __LINE__ )
#	define	RESULT_EXPR( message, result, data )	NotMoon::Windows::outputResult( result, L##message, ( std::stringstream{} << data ).str(), _T(__FILE__), _T(__FUNCTION__), __LINE__ )
//#else
//#	define	RESULT( message, func )				func
//#	define	RESULT_EXPR( message, func, data )	func
//#endif

#endif//NOTMOON_WINDOWS_RESULT_HPP