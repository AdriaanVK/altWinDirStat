// globalhelpers.cpp - Implementation of global helper functions
//
// see `file_header_text.txt` for licensing & contact info. If you can't find that file, then assume you're NOT allowed to do whatever you wanted to do.

#pragma once

#include "stdafx.h"

#ifndef WDS_GLOBALHELPERS_CPP
#define WDS_GLOBALHELPERS_CPP

WDS_FILE_INCLUDE_MESSAGE

#include "globalhelpers.h"
#include "ScopeGuard.h"
#include "signum.h"
#include "stringformatting.h"

#pragma warning(3:4514) //'function': unreferenced inline function has been removed



namespace {

	const PCWSTR date_time_format_locale_name_str = LOCALE_NAME_INVARIANT;
	const DWORD GetDateFormatEx_flags             = DATE_SHORTDATE;
	const DWORD GetTimeFormatEx_flags             = 0;
	const double COLOR_MAX_VALUE = 255.0;



	void display_SYSTEMTIME_debugging_info( _In_ const SYSTEMTIME system_time ) {
		std::wstring fmt_str( L"WDS: GetDateFormatEx failed, given this SYSTEMTIME:\r\n\t" );
		fmt_str += L"year: ";
		fmt_str += std::to_wstring( static_cast< unsigned int >( system_time.wYear ) );
		fmt_str += L"\r\n\t";
		fmt_str += L"month: ";
		fmt_str += std::to_wstring( static_cast< unsigned int >( system_time.wMonth ) );
		fmt_str += L"\r\n\t";
		fmt_str += L"day of week: ";
		fmt_str += std::to_wstring( static_cast< unsigned int >( system_time.wDayOfWeek ) );
		fmt_str += L"\r\n\t";
		fmt_str += L"day: ";
		fmt_str += std::to_wstring( static_cast< unsigned int >( system_time.wDay ) );
		fmt_str += L"\r\n\t";
		fmt_str += L"hour: ";
		fmt_str += std::to_wstring( static_cast< unsigned int >( system_time.wHour ) );
		fmt_str += L"\r\n\t";
		fmt_str += L"minute: ";
		fmt_str += std::to_wstring( static_cast< unsigned int >( system_time.wMinute ) );
		fmt_str += L"\r\n\t";
		fmt_str += L"second: ";
		fmt_str += std::to_wstring( static_cast< unsigned int >( system_time.wSecond ) );
		fmt_str += L"\r\n\t";
		fmt_str += L"millisecond: ";
		fmt_str += std::to_wstring( static_cast< unsigned int >( system_time.wMilliseconds ) );
		fmt_str += L"\r\n";
		const std::wstring& fmt_str_finished = fmt_str;
		OutputDebugStringW( fmt_str_finished.c_str( ) );
		displayWindowsMsgBoxWithMessage( fmt_str_finished );
		}

	_Must_inspect_result_ 
	const bool display_GetDateFormatEx_flags( _In_ const DWORD flags ) {
		if ( ( flags bitand DATE_LONGDATE ) ) {
			displayWindowsMsgBoxWithMessage( L"GetDateFormatEx passed DATE_LONGDATE" );
			return true;
			}
		if ( ( flags bitand DATE_LTRREADING ) ) {
			displayWindowsMsgBoxWithMessage( L"GetDateFormatEx passed DATE_LTRREADING" );
			return true;
			}
		if ( ( flags bitand DATE_RTLREADING ) ) {
			displayWindowsMsgBoxWithMessage( L"GetDateFormatEx passed DATE_RTLREADING" );
			return true;
			}
		if ( ( flags bitand DATE_SHORTDATE ) ) {
			displayWindowsMsgBoxWithMessage( L"GetDateFormatEx passed DATE_SHORTDATE" );
			return true;
			}
		if ( ( flags bitand DATE_USE_ALT_CALENDAR ) ) {
			displayWindowsMsgBoxWithMessage( L"GetDateFormatEx passed DATE_USE_ALT_CALENDAR" );
			return true;
			}
		if ( ( flags bitand DATE_YEARMONTH ) ) {
			displayWindowsMsgBoxWithMessage( L"GetDateFormatEx passed DATE_YEARMONTH" );
			return true;
			}
		return false;
		}

	_Must_inspect_result_ 
	const bool display_GetTimeFormatEx_flags( _In_ const DWORD flags ) {
		if ( ( flags bitand TIME_NOMINUTESORSECONDS ) ) {
			displayWindowsMsgBoxWithMessage( L"GetTimeFormatEx passed TIME_NOMINUTESORSECONDS" );
			return true;
			}
		if ( ( flags bitand TIME_NOSECONDS ) ) {
			displayWindowsMsgBoxWithMessage( L"GetTimeFormatEx passed TIME_NOSECONDS" );
			return true;
			}
		if ( ( flags bitand TIME_NOTIMEMARKER ) ) {
			displayWindowsMsgBoxWithMessage( L"GetTimeFormatEx passed TIME_NOTIMEMARKER" );
			return true;
			}
		if ( ( flags bitand TIME_FORCE24HOURFORMAT ) ) {
			displayWindowsMsgBoxWithMessage( L"GetTimeFormatEx passed TIME_FORCE24HOURFORMAT" );
			return true;
			}
		return false;
		}

	//continue if returns true?
	//This is an error handling function, and is intended to be called rarely!
	__declspec(noinline)
	_Must_inspect_result_ _Success_( return == true ) //_In_range_ upper bound is totally arbitrary! Lower bound is enough for WRITE_BAD_FMT
	const bool get_date_format_err( _In_ const SYSTEMTIME system_time, _In_ _In_range_( 24, 2048 ) const rsize_t str_size, _In_ const DWORD flags ) {
		const auto err = GetLastError( );

		//sorry, this is ugly, isn't it?
		display_SYSTEMTIME_debugging_info( system_time );

		if ( err == ERROR_INSUFFICIENT_BUFFER ) {
			TRACE( _T( "%s\r\n" ), global_strings::get_date_format_buffer_err );
			displayWindowsMsgBoxWithMessage( global_strings::get_date_format_buffer_err );
			const std::wstring buffer_size( L"size of (the insufficient) buffer: " + std::to_wstring( str_size ) );
			displayWindowsMsgBoxWithMessage( buffer_size );
			//std::terminate( );
			return true;
			}
		if ( err == ERROR_INVALID_FLAGS ) {
			TRACE( _T( "%s\r\n" ), global_strings::get_date_format_flags_err );
			displayWindowsMsgBoxWithMessage( global_strings::get_date_format_flags_err );
			return display_GetDateFormatEx_flags( flags );
			}
		if ( err == ERROR_INVALID_PARAMETER ) {
			TRACE( _T( "%s\r\n" ), global_strings::get_date_format_param_err );
			displayWindowsMsgBoxWithMessage( global_strings::get_date_format_param_err );
			return display_GetDateFormatEx_flags( flags );
			//std::terminate( );
			}
		ASSERT( ( err == ERROR_INSUFFICIENT_BUFFER ) || ( err == ERROR_INVALID_FLAGS ) || ( err == ERROR_INVALID_PARAMETER ) );
		displayWindowsMsgBoxWithMessage( L"GetDateFormat failed, and GetLastError returned an unexpected error code!" );
		std::terminate( );
		return false;
		}

	//continue if returns true?
	//This is an error handling function, and is intended to be called rarely!
	__declspec(noinline)
	_Must_inspect_result_ _Success_( return == true )
	const bool get_time_format_err( _In_ const rsize_t str_size, _In_ const DWORD flags ) {
		const auto err = GetLastError( );
		if ( err == ERROR_INSUFFICIENT_BUFFER ) {
			TRACE( _T( "%s\r\n" ), global_strings::get_time_format_buffer_err );
			displayWindowsMsgBoxWithMessage( global_strings::get_time_format_buffer_err );
			const std::wstring buffer_size( L"size of (the insufficient) buffer: " + std::to_wstring( str_size ) );
			displayWindowsMsgBoxWithMessage( buffer_size );
			return true;
			}
		if ( err == ERROR_INVALID_FLAGS ) {
			TRACE( _T( "%s\r\n" ), global_strings::get_time_format_flags_err );
			displayWindowsMsgBoxWithMessage( global_strings::get_time_format_flags_err );
			return display_GetTimeFormatEx_flags( flags );

			//std::terminate( );
			}
		if ( err == ERROR_INVALID_PARAMETER ) {
			TRACE( _T( "%s\r\n" ), global_strings::get_time_format_param_err );
			displayWindowsMsgBoxWithMessage( global_strings::get_time_format_param_err );
			return true;
			}

		if ( err == ERROR_OUTOFMEMORY ) {
			TRACE( _T( "%s\r\n" ), global_strings::get_time_format_err_OUTOFMEMORY );
			displayWindowsMsgBoxWithMessage( global_strings::get_time_format_err_OUTOFMEMORY );
			std::terminate( );
			return false;
			}

		ASSERT( ( err == ERROR_INSUFFICIENT_BUFFER ) || ( err == ERROR_INVALID_FLAGS ) || ( err == ERROR_INVALID_PARAMETER ) || ( err == ERROR_OUTOFMEMORY ) );
		displayWindowsMsgBoxWithMessage( L"FileTimeToSystemTime failed, and GetLastError returned an unexpected error code!" );
		std::terminate( );
		return false;
		}

	//The compiler will automatically inline if /Ob2 is on, so we'll ask anyways.
	_Success_( SUCCEEDED( return ) ) inline HRESULT file_time_to_system_time_err( _Out_writes_z_( strSize ) _Pre_writable_size_( strSize ) PWSTR psz_formatted_datetime, _In_range_( 128, 2048 ) const rsize_t strSize, _Out_ rsize_t& chars_written ) {
		const HRESULT err_res = CStyle_GetLastErrorAsFormattedMessage( psz_formatted_datetime, strSize, chars_written );
		ASSERT( SUCCEEDED( err_res ) );
		if ( !SUCCEEDED( err_res ) ) {
			TRACE( _T( "Error in file_time_to_system_time_err->CStyle_GetLastErrorAsFormattedMessage!!\r\n" ) );
			displayWindowsMsgBoxWithMessage( L"Error in file_time_to_system_time_err->CStyle_GetLastErrorAsFormattedMessage!!\r\n" );
			return err_res;
			}
		//return a failure, with the buffer filled with the error message (not intended to be read in by a function)
		return E_FAIL;
		}

	//_In_range_ upper bound is totally arbitrary! Lower bound is enough for WRITE_BAD_FMT
	void ensure_valid_return_date( _In_ const int gdfres, _In_ _In_ _In_range_( 24, 2048 ) const rsize_t strSize, _In_ const SYSTEMTIME system_time, _Inout_ PWSTR psz_formatted_datetime ) {
		if ( !( ( gdfres + 1 ) < static_cast< std::int64_t >( strSize ) ) ) {
			displayWindowsMsgBoxWithMessage( L"Error in ensure_valid_return_date!(aborting)" );
			std::wstring err_str( L"DEBUGGING INFO: strSize: " );
			err_str += std::to_wstring( strSize );
			err_str += L", gdfres: ";
			err_str += std::to_wstring( gdfres );
			err_str += L".";
			displayWindowsMsgBoxWithMessage( err_str.c_str( ) );
			std::terminate( );
			}
		if ( gdfres == 0 ) {
			const bool we_continue = get_date_format_err( system_time, strSize, GetDateFormatEx_flags );
			if ( we_continue ) {
				rsize_t dummy_var = 0;
				wds_fmt::write_BAD_FMT( psz_formatted_datetime, dummy_var );
				return;
				}
			std::terminate( );
			}
		std::terminate( );
		}

	void ensure_valid_return_time( const int gtfres, const rsize_t strSize, _Inout_ PWSTR psz_formatted_datetime ) {
		if ( !( ( gtfres + 1 ) < static_cast< std::int64_t >( strSize ) ) ) {
			displayWindowsMsgBoxWithMessage( L"Error in ensure_valid_return_time!(aborting)" );
			std::wstring err_str( L"DEBUGGING INFO: strSize: " );
			err_str += std::to_wstring( strSize );
			err_str += L", gtfres: ";
			err_str += std::to_wstring( gtfres );
			err_str += L".";
			displayWindowsMsgBoxWithMessage( err_str.c_str( ) );
			std::terminate( );
			}
		if ( gtfres == 0 ) {
			const bool we_continue = get_time_format_err( strSize, GetTimeFormatEx_flags );
			if ( we_continue ) {
				rsize_t dummy_var = 0;
				wds_fmt::write_BAD_FMT( psz_formatted_datetime, dummy_var );
				return;
				}
			std::terminate( );
			}
		std::terminate( );
		}



	void convert_number_to_string_failed_display_debugging_info( _In_range_( 19, 128 ) const rsize_t bufSize, _In_ const std::int64_t number ) {
		const std::wstring err_str( L"DEBUGGING INFO: bufSize: " + std::to_wstring( bufSize ) + L", number: " + std::to_wstring( number ) );
		displayWindowsMsgBoxWithMessage( err_str.c_str( ) );
		std::terminate( );
		}

	void convert_number_to_string_failed( _In_range_( 19, 128 ) const rsize_t bufSize, _In_ const std::int64_t number, _In_ const HRESULT strsafe_printf_res ) {
		auto guard = WDS_SCOPEGUARD_INSTANCE( [ &] { convert_number_to_string_failed_display_debugging_info( bufSize, number ); } );
		
		if ( strsafe_printf_res == STRSAFE_E_INSUFFICIENT_BUFFER ) {
			displayWindowsMsgBoxWithMessage( L"STRSAFE_E_INSUFFICIENT_BUFFER in CStyle_GetNumberFormatted!(aborting)" );
			return;
			}
		if ( strsafe_printf_res == STRSAFE_E_END_OF_FILE ) {
			//this doesn't make any sense.
			displayWindowsMsgBoxWithMessage( L"STRSAFE_E_END_OF_FILE in CStyle_GetNumberFormatted!(aborting)" );
			return;
			}
		if ( strsafe_printf_res == STRSAFE_E_INVALID_PARAMETER ) {
			displayWindowsMsgBoxWithMessage( L"STRSAFE_E_INVALID_PARAMETER in CStyle_GetNumberFormatted!(aborting)" );
			return;
			}
		displayWindowsMsgBoxWithMessage( L"Unknown error in CStyle_GetNumberFormatted!(aborting)" );
		return;
		}

	void convert_number_to_string( _In_range_( 19, 128 ) const rsize_t bufSize, _Pre_writable_size_( bufSize ) _Post_z_ PWSTR number_str_buffer, _In_ const std::int64_t number ) {
		rsize_t chars_remaining = 0;

		const HRESULT strsafe_printf_res = StringCchPrintfExW( number_str_buffer, bufSize, NULL, &chars_remaining, 0, L"%I64d", number );
		if ( SUCCEEDED( strsafe_printf_res ) ) {
			return;
			}
		WDS_ASSERT_EXPECTED_STRING_FORMAT_FAILURE_HRESULT( strsafe_printf_res );
		WDS_STRSAFE_E_INVALID_PARAMETER_HANDLER( strsafe_printf_res, "StringCchPrintfExW" );
		
		convert_number_to_string_failed( bufSize, number, strsafe_printf_res );
		}

	inline void DistributeFirst( _Inout_ _Out_range_(0, 255) INT& first, _Inout_ _Out_range_(0, 255) INT& second, _Inout_ _Out_range_(0, 255) INT& third ) {
		const INT h = ( first - 255 ) / 2;
		first = 255;
		second += h;
		third += h;

		if ( second > 255 ) {
			const auto h2 = second - 255;
			second = 255;
			third += h2;
			}
		else if ( third > 255 ) {
			const auto h3 = third - 255;
			third = 255;
			second += h3;
			}
		ASSERT( second <= 255 );
		ASSERT( third <= 255 );
		}




}

_Success_( SUCCEEDED( return ) )
const HRESULT WriteToStackBuffer_do_nothing( WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written ) {
	if ( strSize > 1 ) {
		psz_text[ 0 ] = 0;
		chars_written = 0;
		ASSERT( chars_written == wcslen( psz_text ) );
		return S_OK;
		}
	//do nothing
	sizeBuffNeed = 6u; //you've got to be kidding me if you've passed a buffer that too small.
	return STRSAFE_E_INSUFFICIENT_BUFFER;
	//return StringCchPrintfExW( psz_text, strSize, NULL, &chars_remaining, 0, L"" );
	}


QPC_timer::QPC_timer( ) : m_frequency( help_QueryPerformanceFrequency( ).QuadPart ), m_start( 0 ), m_end( 0 ) {
	ASSERT( m_frequency > 0 );
	}

void QPC_timer::begin( ) {
	m_start = help_QueryPerformanceCounter( ).QuadPart;
	}

void QPC_timer::end( ) {
	m_end = help_QueryPerformanceCounter( ).QuadPart;
	}

const double QPC_timer::total_time_elapsed( ) const {
	ASSERT( m_end > m_start );
	static_assert( std::is_same<std::int64_t, LONGLONG>::value, "difference is wrong!" );
	const auto difference = ( m_end - m_start );
	const DOUBLE adjustedTimingFrequency = ( static_cast< DOUBLE >( 1.00 ) ) / static_cast< DOUBLE >( m_frequency );
	const auto total_time = ( difference * adjustedTimingFrequency );
	return total_time;
	}


void InitializeCriticalSection_wrapper( _Pre_invalid_ _Post_valid_ _Out_ CRITICAL_SECTION& cs ) {
	InitializeCriticalSection( &cs );
	}

void DeleteCriticalSection_wrapper( _Pre_valid_ _Post_invalid_ CRITICAL_SECTION& cs ) {
	DeleteCriticalSection( &cs );
	}


void error_getting_pointer_to( _In_z_ PCWSTR const function_name ) {
	std::wstring message;
	message.reserve( 75 );
	message += ( L"Failed to get pointer to " );
	message += function_name;
	message += L'!';
	TRACE( L"%s\r\n", message.c_str( ) );
	displayWindowsMsgBoxWithMessage( std::move( message ) );
	}

void test_if_null_funcptr( void* func_ptr, _In_z_ PCWSTR const function_name ) {
	if ( func_ptr == NULL ) {
		error_getting_pointer_to( function_name );
		}
	}

_Pre_satisfies_( rect.left > rect.right ) _Post_satisfies_( rect.left <= rect.right )
void normalize_RECT_left_right( _Inout_ RECT& rect ) {
	ASSERT( rect.left > rect.right );
	const auto temp = rect.left;
	rect.left = rect.right;
	rect.right = temp;
	ASSERT( rect.left <= rect.right );
	}

_Pre_satisfies_( rect.top > rect.bottom ) _Post_satisfies_( rect.top <= rect.bottom )
void normalize_RECT_top_bottom( _Inout_ RECT& rect ) {
	ASSERT( rect.top > rect.bottom );
	const auto temp = rect.top;
	rect.top = rect.bottom;
	rect.bottom = temp;
	ASSERT( rect.top <= rect.bottom );
	}

_Post_satisfies_( rect.left <= rect.right ) _Post_satisfies_( rect.top <= rect.bottom )
void normalize_RECT( _Inout_ RECT& rect ) {
	if ( rect.left > rect.right ) {
		normalize_RECT_left_right( rect );
		}
	if ( rect.top > rect.bottom ) {
		normalize_RECT_top_bottom( rect );
		}
	}





//TODO: mark to only return STRSAFE_E_INSUFFICIENT_BUFFER, E_FAIL, or S_OK.
_Success_( SUCCEEDED( return ) ) HRESULT wds_fmt::CStyle_FormatFileTime( _In_ const FILETIME t, _Out_writes_z_( strSize ) _Pre_writable_size_( strSize ) PWSTR psz_formatted_datetime, _In_range_( 128, 2048 ) const rsize_t strSize, _Out_ rsize_t& chars_written ) {
	ASSERT( &t != NULL );
	SYSTEMTIME st;
	if ( !FileTimeToSystemTime( &t, &st ) ) {
		return file_time_to_system_time_err( psz_formatted_datetime, strSize, chars_written );
		}
	
	//const LCID lcid = MAKELCID( GetUserDefaultLangID( ), SORT_DEFAULT );
	//const int gdfres = GetDateFormatW( lcid, DATE_SHORTDATE, &st, NULL, psz_formatted_datetime, static_cast<int>( strSize ) );
	//GRR DATE_AUTOLAYOUT doesn't work, because we're not targeting a Windows 7 minimum!!
	//const int gdfres = GetDateFormatEx( LOCALE_NAME_INVARIANT, DATE_SHORTDATE bitor DATE_AUTOLAYOUT, )

	const int gdfres = GetDateFormatEx( date_time_format_locale_name_str, GetDateFormatEx_flags, &st, NULL, psz_formatted_datetime, static_cast< int >( strSize ), NULL );

	ensure_valid_return_date( gdfres, strSize, st, psz_formatted_datetime );
	chars_written = static_cast<rsize_t>( gdfres );

	//if we have room for two spaces and a null:
	if ( ( gdfres + 3 ) < static_cast<int>( strSize ) ) {
		psz_formatted_datetime[ gdfres - 1 ] = L' ';
		psz_formatted_datetime[ gdfres     ] = L' ';
		psz_formatted_datetime[ gdfres + 1 ] = 0;
		chars_written = static_cast<rsize_t>( gdfres + 1 );
		}
	else {
		return STRSAFE_E_INSUFFICIENT_BUFFER;
		}

	//const int gtfres = GetTimeFormatW( lcid, 0, &st, NULL, ( psz_formatted_datetime + chars_written ), static_cast<int>( static_cast<int>( strSize ) - static_cast<int>( chars_written ) ) );
	const int gtfres = GetTimeFormatEx( date_time_format_locale_name_str, GetTimeFormatEx_flags, &st, NULL, ( psz_formatted_datetime + chars_written ), static_cast<int>( static_cast<int>( strSize ) - static_cast<int>( chars_written ) ) );

	ensure_valid_return_time( gtfres, strSize, psz_formatted_datetime );

	chars_written += gtfres;
	chars_written -= 1;
	/*
	This function returns 0 if it does not succeed. To get extended error information, the application can call GetLastError, which can return one of the following error codes:
		ERROR_INSUFFICIENT_BUFFER. A supplied buffer size was not large enough, or it was incorrectly set to NULL.
		ERROR_INVALID_FLAGS.       The values supplied for flags were not valid.
		ERROR_INVALID_PARAMETER.   Any of the parameter values was invalid.	
	*/
	return S_OK;
	}



//
_Success_( SUCCEEDED( return ) ) HRESULT wds_fmt::CStyle_GetNumberFormatted( const std::int64_t number, _Pre_writable_size_( strSize ) PWSTR psz_formatted_number, _In_range_( 21, 64 ) const rsize_t strSize, _Out_ rsize_t& chars_written ) {
	// Returns formatted number like "123.456.789".
	// 18446744073709551615 is max ( for std::uint64_t )
	//                     ^ 20 characters
	// 18,446,744,073,709,551,615
	//                           ^26 characters
	//                            26 + null terminator = 27
	// 9223372036854775807 is max ( for 64 bit int, INT64_MAX )
	//                    ^ 19 characters
	//-9223372036854775807 is min ( for 64 bit int, INT64_MIN ) //<-- Ok, (-9223372036854775807i64 - 1) is. But I'll ignore for now.
	//                    ^ 20 characters
	//-9,223,372,036,854,775,807
	//                          ^26 characters
	//                           26 + null terminator = 27

	//Our plan:
	//	Convert number to string, unformatted
	//	Pass THAT string to GetNumberFormatEx

	const rsize_t bufSize = 66;
	_Null_terminated_ wchar_t number_str_buffer[ bufSize ] = { 0 };
	
	convert_number_to_string( bufSize, number_str_buffer, number );
	NUMBERFMT format_struct = { 0, 0, 3, L".", L",", 1 };

	//0 indicates failure! http://msdn.microsoft.com/en-us/library/windows/desktop/dd318113.aspx
	const auto get_number_fmt_ex_res = GetNumberFormatEx( NULL, 0, number_str_buffer, &format_struct, psz_formatted_number, static_cast<int>( strSize ) );
	if ( get_number_fmt_ex_res != 0 ) {
		ASSERT( get_number_fmt_ex_res > 0 );
		chars_written = static_cast<rsize_t>( get_number_fmt_ex_res - 1u );
		ASSERT( chars_written == wcslen( psz_formatted_number ) );
		return S_OK;
		}
	ASSERT( get_number_fmt_ex_res == 0 );
	const DWORD last_err = GetLastError( );
	ASSERT( ( last_err == ERROR_INSUFFICIENT_BUFFER ) || ( last_err == ERROR_INVALID_FLAGS ) || ( last_err == ERROR_INVALID_PARAMETER ) || ( last_err == ERROR_OUTOFMEMORY ) );
	switch ( last_err ) 
		{
		case ERROR_INSUFFICIENT_BUFFER:
			return STRSAFE_E_INSUFFICIENT_BUFFER;
		case ERROR_INVALID_FLAGS:
		case ERROR_INVALID_PARAMETER:
			return STRSAFE_E_INVALID_PARAMETER;
		case ERROR_OUTOFMEMORY:
			return STRSAFE_E_END_OF_FILE;
		default:
			ASSERT( false );
			displayWindowsMsgBoxWithMessage( L"Unexpected error in CStyle_GetNumberFormatted, after GetNumberFormatEx!(aborting)" );
			displayWindowsMsgBoxWithError( last_err );
			std::terminate( );
		}
	ASSERT( false );
	displayWindowsMsgBoxWithMessage( L"Unintended execution in CStyle_GetNumberFormatted, after GetNumberFormatEx!(aborting!)" );
	std::terminate( );
	static_assert( !SUCCEEDED( E_FAIL ), "bad error return type!" );
	return E_FAIL;
	}

_Success_( SUCCEEDED( return ) )
const HRESULT allocate_and_copy_name_str( _Pre_invalid_ _Post_z_ _Post_readable_size_( new_name_length ) wchar_t*& new_name_ptr, _In_ _In_range_( 0, UINT16_MAX ) const rsize_t& new_name_length, const std::wstring& name ) {
	ASSERT( new_name_length < UINT16_MAX );
	new_name_ptr = new wchar_t[ new_name_length + 2u ];
	PWSTR pszend = NULL;
	rsize_t chars_remaining = new_name_length;
	const HRESULT res = StringCchCopyExW( new_name_ptr, ( new_name_length + 1u ), name.c_str( ), &pszend, &chars_remaining, 0 );
	ASSERT( SUCCEEDED( res ) );
	if ( SUCCEEDED( res ) ) {
#ifdef DEBUG
		ASSERT( wcslen( new_name_ptr ) == new_name_length );
		ASSERT( wcscmp( new_name_ptr, name.c_str( ) ) == 0 );
		const auto da_ptrdiff = ( std::ptrdiff_t( pszend ) - std::ptrdiff_t( new_name_ptr ) );
		ASSERT( ( da_ptrdiff / sizeof( wchar_t ) ) == new_name_length );
#endif
		return res;
		}
	displayWindowsMsgBoxWithMessage( L"Copy of name_str failed!!!" );
	std::terminate( );
	return res;
	}


//This is an error handling function, and is intended to be called rarely!
__declspec(noinline)
void unexpected_strsafe_invalid_parameter_handler( _In_z_ PCSTR const strsafe_func_name, _In_z_ PCSTR const file_name_in, _In_z_ PCSTR const func_name_in, _In_ _In_range_( 0, INT_MAX ) const int line_number_in ) {
	std::string err_str( strsafe_func_name );
	err_str += " returned STRSAFE_E_INVALID_PARAMETER, in: file `";
	err_str += file_name_in;
	err_str += "`, function: `";
	err_str += func_name_in;
	err_str += "` line: `";
	err_str += std::to_string( line_number_in );
	err_str += "`! This (near universally) means an issue where incorrect compile-time constants were passed to a strsafe function. Thus it's probably not recoverable. We'll abort. Sorry!";
	displayWindowsMsgBoxWithMessage( err_str );
	std::terminate( );
	}

//this function is only called in the rare/error path, so NON-inline code is faster, and smaller.
__declspec(noinline)
void handle_stack_insufficient_buffer( _In_ const rsize_t str_size, _In_ const rsize_t generic_size_needed, _Out_ rsize_t& size_buff_need, _Out_ rsize_t& chars_written ) {
	chars_written = str_size;
	if ( str_size < generic_size_needed ) {
		size_buff_need = generic_size_needed;
		return;
		}
	size_buff_need = ( str_size * 2 );
	return;
	}


const LARGE_INTEGER help_QueryPerformanceCounter( ) {
	LARGE_INTEGER doneTime;
	const BOOL behavedWell = QueryPerformanceCounter( &doneTime );
	ASSERT( behavedWell );
	if ( !behavedWell ) {
		WTL::AtlMessageBox( NULL, L"QueryPerformanceCounter failed!!", L"ERROR!", MB_OK );
		doneTime.QuadPart = -1;
		}
	return doneTime;
	}

const LARGE_INTEGER help_QueryPerformanceFrequency( ) {
	LARGE_INTEGER doneTime;
	const BOOL behavedWell = QueryPerformanceFrequency( &doneTime );
	ASSERT( behavedWell );
	if ( !behavedWell ) {
		WTL::AtlMessageBox( NULL, L"QueryPerformanceFrequency failed!!", L"ERROR!", MB_OK );
		doneTime.QuadPart = -1;
		}
	return doneTime;
	}


static_assert( !SUCCEEDED( E_FAIL ), "CStyle_GetLastErrorAsFormattedMessage doesn't return a valid error code!" );
static_assert( SUCCEEDED( S_OK ), "CStyle_GetLastErrorAsFormattedMessage doesn't return a valid success code!" );
//On returning E_FAIL, call GetLastError for details. That's not my idea! //TODO: mark as only returning S_OK, E_FAIL
_Success_( SUCCEEDED( return ) ) HRESULT CStyle_GetLastErrorAsFormattedMessage( WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_formatted_error, _In_range_( 128, 32767 ) const rsize_t strSize, _Out_ rsize_t& chars_written, const DWORD error ) {
	//const auto err = GetLastError( );
	const auto err = error;
	const auto ret = FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), psz_formatted_error, static_cast<DWORD>( strSize ), NULL );
	if ( ret != 0 ) {
		chars_written = ret;
		return S_OK;
		}
	const DWORD error_err = GetLastError( );
	TRACE( _T( "FormatMessageW failed with error code: `%lu`!!\r\n" ), error_err );
	
	const rsize_t err_msg_buff_size = 512;
	_Null_terminated_ char err_msg_buff[ err_msg_buff_size ] = { 0 };
	const HRESULT output_error_message_format_result = StringCchPrintfA( err_msg_buff, err_msg_buff_size, "WDS: FormatMessageW failed with error code: `%lu`!!\r\n", error_err );
	if ( SUCCEEDED( output_error_message_format_result ) ) {
		OutputDebugStringA( err_msg_buff );
		}
	else {
		WDS_ASSERT_EXPECTED_STRING_FORMAT_FAILURE_HRESULT( output_error_message_format_result );
		WDS_STRSAFE_E_INVALID_PARAMETER_HANDLER( output_error_message_format_result, "StringCchPrintfA" );
		OutputDebugStringA( "WDS: FormatMessageW failed, and THEN formatting the error message for FormatMessageW failed!\r\n" );
		}
	if ( strSize > 41 ) {
		wds_fmt::write_bad_fmt_msg( psz_formatted_error, chars_written );
		return E_FAIL;
		}
	if ( strSize > 8 ) {
		wds_fmt::write_BAD_FMT( psz_formatted_error, chars_written );
		return E_FAIL;
		}
	chars_written = 0;
	return E_FAIL;
	}

//This is an error handling function, and is intended to be called rarely!
__declspec(noinline)
void wds_fmt::write_bad_fmt_msg( _Out_writes_z_( 41 ) _Pre_writable_size_( 42 ) _Post_readable_size_( chars_written ) PWSTR psz_fmt_msg, _Out_ rsize_t& chars_written ) {
	psz_fmt_msg[  0 ] = L'F';
	psz_fmt_msg[  1 ] = L'o';
	psz_fmt_msg[  2 ] = L'r';
	psz_fmt_msg[  3 ] = L'm';
	psz_fmt_msg[  4 ] = L'a';
	psz_fmt_msg[  5 ] = L't';
	psz_fmt_msg[  6 ] = L'M';
	psz_fmt_msg[  7 ] = L'e';
	psz_fmt_msg[  8 ] = L's';
	psz_fmt_msg[  9 ] = L's';
	psz_fmt_msg[ 10 ] = L'a';
	psz_fmt_msg[ 11 ] = L'g';
	psz_fmt_msg[ 12 ] = L'e';
	psz_fmt_msg[ 13 ] = L' ';
	psz_fmt_msg[ 14 ] = L'f';
	psz_fmt_msg[ 15 ] = L'a';
	psz_fmt_msg[ 16 ] = L'i';
	psz_fmt_msg[ 17 ] = L'l';
	psz_fmt_msg[ 18 ] = L'e';
	psz_fmt_msg[ 19 ] = L'd';
	psz_fmt_msg[ 20 ] = L' ';
	psz_fmt_msg[ 21 ] = L't';
	psz_fmt_msg[ 22 ] = L'o';
	psz_fmt_msg[ 23 ] = L' ';
	psz_fmt_msg[ 24 ] = L'f';
	psz_fmt_msg[ 25 ] = L'o';
	psz_fmt_msg[ 26 ] = L'r';
	psz_fmt_msg[ 27 ] = L'm';
	psz_fmt_msg[ 28 ] = L'a';
	psz_fmt_msg[ 29 ] = L't';
	psz_fmt_msg[ 30 ] = L' ';
	psz_fmt_msg[ 31 ] = L'a';
	psz_fmt_msg[ 32 ] = L'n';
	psz_fmt_msg[ 33 ] = L' ';
	psz_fmt_msg[ 34 ] = L'e';
	psz_fmt_msg[ 35 ] = L'r';
	psz_fmt_msg[ 36 ] = L'r';
	psz_fmt_msg[ 37 ] = L'o';
	psz_fmt_msg[ 38 ] = L'r';
	psz_fmt_msg[ 39 ] = L'!';
	psz_fmt_msg[ 40 ] = 0;
	chars_written = 41;
	ASSERT( wcslen( psz_fmt_msg ) == chars_written );
	}

//This is an error handling function, and is intended to be called rarely!
__declspec(noinline)
void displayWindowsMsgBoxWithError( const DWORD error ) {
	const rsize_t err_msg_size = 1024;
	_Null_terminated_ wchar_t err_msg[ err_msg_size ] = { 0 };
	rsize_t chars_written = 0;

	const HRESULT err_res = CStyle_GetLastErrorAsFormattedMessage( err_msg, err_msg_size, chars_written, error );
	ASSERT( SUCCEEDED( err_res ) );
	if ( SUCCEEDED( err_res ) ) {
		WTL::AtlMessageBox( NULL, err_msg, TEXT( "Error" ), MB_OK );
		TRACE( _T( "Error: %s\r\n" ), err_msg );
		return;
		}
	TRACE( _T( "First attempt to get last error as a formatted message FAILED!\r\n" ) );

	const rsize_t err_msg_size_2 = 4096;
	_Null_terminated_ wchar_t err_msg_2[ err_msg_size_2 ] = { 0 };
	rsize_t chars_written_2 = 0;
	const HRESULT err_res_2 = CStyle_GetLastErrorAsFormattedMessage( err_msg_2, err_msg_size_2, chars_written_2, error );
	ASSERT( SUCCEEDED( err_res_2 ) );
	if ( SUCCEEDED( err_res_2 ) ) {
		WTL::AtlMessageBox( NULL, err_msg_2, TEXT( "Error" ), MB_OK );
		TRACE( _T( "Error: %s\r\n" ), err_msg_2 );
		return;
		}
	TRACE( _T( "Error while getting error message!\r\n" ), err_msg_2 );
	WTL::AtlMessageBox( NULL, _T( "Error while getting error message!\r\n" ), TEXT( "Error" ), MB_OK );
	}

//This is an error handling function, and is intended to be called rarely!
__declspec(noinline)
void displayWindowsMsgBoxWithMessage( const std::wstring message ) {
	displayWindowsMsgBoxWithMessage( message.c_str( ) );
	}

//This is an error handling function, and is intended to be called rarely!
__declspec(noinline)
void displayWindowsMsgBoxWithMessage( const std::string message ) {
	//MessageBoxW( NULL, message.c_str( ), TEXT( "Error" ), MB_OK );
	
	//see: https://code.google.com/p/hadesmem/source/browse/trunk/Include/Common/HadesCommon/I18n.hpp?r=1163
	auto convert_obj = stdext::cvt::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t>( );

	const auto new_wide_str = convert_obj.from_bytes( message );
	WTL::AtlMessageBox( NULL, new_wide_str.c_str( ), L"Error", MB_OK | MB_ICONINFORMATION );
	TRACE( _T( "Error: %s\r\n" ), new_wide_str.c_str( ) );
	}

//This is an error handling function, and is intended to be called rarely!
__declspec(noinline)
void displayWindowsMsgBoxWithMessage( PCWSTR const message ) {
	WTL::AtlMessageBox( NULL, message, TEXT( "Error" ), MB_OK );
	TRACE( _T( "Error: %s\r\n" ), message );
	}

RECT BuildRECT( const SRECT& in ) {
	//ASSERT( ( in.left != -1 ) && ( in.top != -1 ) && ( in.right != -1 ) && ( in.bottom != -1 ) );
	ASSERT( ( in.right + 1 ) >= in.left );
	ASSERT( in.bottom >= in.top );
	RECT out;
	out.left   = static_cast<LONG>( in.left );
	out.top    = static_cast<LONG>( in.top );
	out.right  = static_cast<LONG>( in.right );
	out.bottom = static_cast<LONG>( in.bottom );
	ASSERT( out.left == in.left );
	ASSERT( out.top == in.top );
	ASSERT( out.right == in.right );
	ASSERT( out.bottom == in.bottom );
	
	/*
inline void CRect::NormalizeRect() throw()
{
	int nTemp;
	if (left > right)
	{
		nTemp = left;
		left = right;
		right = nTemp;
	}
	if (top > bottom)
	{
		nTemp = top;
		top = bottom;
		bottom = nTemp;
	}
}
*/

	//if ( out.left > out.right ) {
	//	normalize_RECT_left_right( out );
	//	}

	//if ( out.top > out.bottom ) {
	//	normalize_RECT_top_bottom( out );
	//	}

	normalize_RECT( out );

	//out.NormalizeRect( );
	ASSERT( out.right >= out.left );
	ASSERT( out.bottom >= out.top );
	return out;
	}




//This is an error handling function, and is intended to be called rarely!
__declspec(noinline)
void wds_fmt::write_MEM_INFO_ERR( _Out_writes_z_( 13 ) _Pre_writable_size_( 13 ) PWSTR psz_formatted_usage ) {
	psz_formatted_usage[ 0  ] = 'M';
	psz_formatted_usage[ 1  ] = 'E';
	psz_formatted_usage[ 2  ] = 'M';
	psz_formatted_usage[ 3  ] = '_';
	psz_formatted_usage[ 4  ] = 'I';
	psz_formatted_usage[ 5  ] = 'N';
	psz_formatted_usage[ 6  ] = 'F';
	psz_formatted_usage[ 7  ] = 'O';
	psz_formatted_usage[ 8  ] = '_';
	psz_formatted_usage[ 9  ] = 'E';
	psz_formatted_usage[ 10 ] = 'R';
	psz_formatted_usage[ 11 ] = 'R';
	psz_formatted_usage[ 12 ] =  0;
	}

//This is an error handling function, and is intended to be called rarely!
__declspec(noinline)
void wds_fmt::write_RAM_USAGE( _Out_writes_z_( 12 ) _Pre_writable_size_( 13 ) PWSTR psz_ram_usage ) {
	psz_ram_usage[ 0  ] = 'R';
	psz_ram_usage[ 1  ] = 'A';
	psz_ram_usage[ 2  ] = 'M';
	psz_ram_usage[ 3  ] = ' ';
	psz_ram_usage[ 4  ] = 'U';
	psz_ram_usage[ 5  ] = 's';
	psz_ram_usage[ 6  ] = 'a';
	psz_ram_usage[ 7  ] = 'g';
	psz_ram_usage[ 8  ] = 'e';
	psz_ram_usage[ 9  ] = ':';
	psz_ram_usage[ 10 ] = ' ';
	psz_ram_usage[ 11 ] = 0;

	}

_Pre_satisfies_( min_val < max_val )
_Post_satisfies_( min_val <= val )
_Post_satisfies_( val <= max_val )
void CheckMinMax( _Inout_ LONG& val, _In_ const LONG min_val, _In_ const LONG max_val ) {
	ASSERT( min_val <= max_val );
	if ( val < min_val ) {
		val = min_val;
		}
	if ( val > max_val ) {
		val = max_val;
		}
	ASSERT( val <= max_val );
	ASSERT( min_val <= val );
	}

_Pre_satisfies_( min_val < max_val )
_Post_satisfies_( min_val <= val )
_Post_satisfies_( val <= max_val )
void CheckMinMax( _Inout_ LONG& val, _In_ const INT min_val, _In_ const INT max_val ) {
	ASSERT( min_val <= max_val );

	if ( val < static_cast<LONG>( min_val ) ) {
		val = static_cast<LONG>( min_val );
		}
	if ( val > static_cast<LONG>( max_val ) ) {
		val = static_cast<LONG>( max_val );
		}
	ASSERT( val <= static_cast<LONG>( max_val ) );
	ASSERT( static_cast<LONG>( min_val ) <= val );
	}

_Pre_satisfies_( min_val < max_val )
_Post_satisfies_( min_val <= val )
_Post_satisfies_( val <= max_val )
void CheckMinMax( _Inout_ INT& val, _In_ const INT min_val, _In_ const INT max_val ) {
	ASSERT( min_val <= max_val );

	if ( val < min_val ) {
		val = min_val;
		}
	if ( val > max_val ) {
		val = max_val;
		}
	ASSERT( val <= max_val );
	ASSERT( min_val <= val );
	}

bool Compare_FILETIME_eq( const FILETIME& t1, const FILETIME& t2 ) {
	//CompareFileTime returns 0 when first FILETIME is equal to the second FILETIME
	//Therefore: we can 'emulate' the `==` operator, by checking if ( CompareFileTime( &t1, &t2 ) == ( 0 ) );
	return ( CompareFileTime( &t1, &t2 ) == ( 0 ) );
	}







void NormalizeColor( _Inout_ _Out_range_(0, 255) INT& red, _Inout_ _Out_range_(0, 255) INT& green, _Inout_ _Out_range_(0, 255) INT& blue ) {
	ASSERT( red + green + blue <= 3 * COLOR_MAX_VALUE );
	if ( red > 255 ) {
#ifdef COLOR_DEBUGGING
		TRACE( _T( "Distributing red...\r\n" ) );
#endif
		DistributeFirst( red, green, blue );
		}
	else if ( green > 255 ) {
#ifdef COLOR_DEBUGGING
		TRACE( _T( "Distributing green...\r\n" ) );
#endif
		DistributeFirst( green, red, blue );
		}
	else if ( blue > 255 ) {
#ifdef COLOR_DEBUGGING
		TRACE( _T( "Distributing blue...\r\n" ) );
#endif
		DistributeFirst( blue, red, green );
		}
	}



COLORREF CColorSpace::MakeBrightColor( _In_ const COLORREF color, _In_ _In_range_( 0, 1 ) const DOUBLE brightness ) {
	ASSERT( brightness >= 0.0 );
	ASSERT( brightness <= 1.0 );

	DOUBLE dred   = GetRValue( color ) / COLOR_MAX_VALUE;
	DOUBLE dgreen = GetGValue( color ) / COLOR_MAX_VALUE;
	DOUBLE dblue  = GetBValue( color ) / COLOR_MAX_VALUE;
#ifdef COLOR_DEBUGGING
	TRACE( _T( "passed brightness: %.3f, red: %.3f, green: %.3f, blue: %.3f\r\n" ), brightness, dred, dgreen, dblue );
#endif

	const DOUBLE f = 3.0 * brightness / ( dred + dgreen + dblue );
	dred   *= f;
	dgreen *= f;
	dblue  *= f;

#ifdef COLOR_DEBUGGING
	TRACE( _T( "Intermediate colors,     red: %.3f, green: %.3f, blue: %.3f\r\n" ), dred, dgreen, dblue );
#endif


	ASSERT( ( std::lrint( dred * int( COLOR_MAX_VALUE ) ) ) == ( std::lrint( dred * COLOR_MAX_VALUE ) ) );

	INT red   = std::lrint( dred   * COLOR_MAX_VALUE );
	INT green = std::lrint( dgreen * COLOR_MAX_VALUE );
	INT blue  = std::lrint( dblue  * COLOR_MAX_VALUE );
	
	NormalizeColor( red, green, blue );
	ASSERT( RGB( red, green, blue ) != 0 );
#ifdef COLOR_DEBUGGING
	TRACE( _T( "MakeBrightColor returning red: %i, green: %i, blue: %i\r\n" ), red, green, blue );
#endif

	return RGB( red, green, blue );
	}

_Pre_satisfies_( handle != INVALID_HANDLE_VALUE )
_At_( handle, _Post_invalid_ )
_At_( handle, _Pre_valid_ )
void close_handle( const HANDLE handle ) {
	//If [CloseHandle] succeeds, the return value is nonzero.
	const BOOL res = CloseHandle( handle );
	ASSERT( res != 0 );
	if ( !res ) {
		TRACE( _T( "Closing handle failed!\r\n" ) );
		}
	
	}

#ifdef DEBUG

#ifdef COLOR_DEBUGGING
//this function exists for the singular purpose of tracing to console, as doing so from a .cpp is cleaner.
void trace_m_stripe_color_make_bright_color( _In_ const COLORREF m_windowColor, _In_ const DOUBLE b ) {
	TRACE( _T( "m_stripeColor = MakeBrightColor( %ld, %f )\r\n" ), m_windowColor, b );
	}

//this function exists for the singular purpose of tracing to console, as doing so from a .cpp is cleaner.
void trace_m_stripeColor( _In_ const COLORREF m_stripeColor ) {
	TRACE( _T( "m_stripeColor: %ld\r\n" ), m_stripeColor );
	}
#endif

//this function exists for the singular purpose of tracing to console, as doing so from a .cpp is cleaner.
void trace_on_destroy( _In_z_ PCWSTR const m_persistent_name ) {
	TRACE( _T( "%s received OnDestroy!\r\n" ), m_persistent_name );
	}

//this function exists for the singular purpose of tracing to console, as doing so from a .cpp is cleaner.
void trace_full_path( _In_z_ PCWSTR const path ) {
	TRACE( _T( "MyGetFullPathName( m_folder_name_heap ): %s\r\n" ), path );
	}

#endif


_Ret_range_( 0, 100 ) INT Treemap_Options::GetBrightnessPercent( ) const {
	return RoundDouble( brightness   * 100 );
	}


_Ret_range_( 0, 100 ) INT Treemap_Options::GetHeightPercent( ) const {
	return RoundDouble( height       * 100 );
	}

_Ret_range_( 0, 100 ) INT Treemap_Options::GetScaleFactorPercent( ) const {
	return RoundDouble( scaleFactor  * 100 );
	}


_Ret_range_( 0, 100 ) INT Treemap_Options::GetAmbientLightPercent( ) const {
	return RoundDouble( ambientLight * 100 );
	}

_Ret_range_( 0, 100 ) INT Treemap_Options::GetLightSourceXPercent( ) const {
	return RoundDouble( lightSourceX * 100 );
	}

_Ret_range_( 0, 100 ) INT Treemap_Options::GetLightSourceYPercent( ) const {
	return RoundDouble( lightSourceY * 100 );
	}

POINT Treemap_Options::GetLightSourcePoint( ) const {
	return POINT { GetLightSourceXPercent( ), GetLightSourceYPercent( ) };
	}

_Ret_range_( 0, 100 ) INT Treemap_Options::RoundDouble ( const DOUBLE d ) const {
	return signum( d ) * static_cast<INT>( abs( d ) + 0.5 );
	}

void Treemap_Options::SetBrightnessPercent( const INT    n   ) {
	brightness   = n / 100.0;
	}

void Treemap_Options::SetHeightPercent( const INT    n   ) {
	height       = n / 100.0;
	}

void Treemap_Options::SetScaleFactorPercent( const INT    n   ) {
	scaleFactor  = n / 100.0;
	}

void Treemap_Options::SetAmbientLightPercent( const INT    n   ) {
	ambientLight = n / 100.0;
	}

void Treemap_Options::SetLightSourceXPercent( const INT    n   ) {
	lightSourceX = n / 100.0; 
	}

void Treemap_Options::SetLightSourceYPercent( const INT    n   ) {
	lightSourceY = n / 100.0;
	}


void Treemap_Options::SetLightSourcePoint   ( const POINT  pt  ) {
	SetLightSourceXPercent( pt.x );
	SetLightSourceYPercent( pt.y );
	}

SRECT::SRECT( ) : left( 0 ), top( 0 ), right( 0 ), bottom( 0 ) { }
SRECT::SRECT( std::int16_t iLeft, std::int16_t iTop, std::int16_t iRight, std::int16_t iBottom ) : left { iLeft }, top { iTop }, right { iRight }, bottom { iBottom } { }

SRECT::SRECT( const RECT& in ) {
	left   = static_cast<std::int16_t>( in.right );
	top    = static_cast<std::int16_t>( in.top );
	right  = static_cast<std::int16_t>( in.right );
	bottom = static_cast<std::int16_t>( in.bottom );
	}


const int SRECT::Width( ) const {
	return right - left;
	}

const int SRECT::Height( ) const {
	return bottom - top;
	}

#else

#endif