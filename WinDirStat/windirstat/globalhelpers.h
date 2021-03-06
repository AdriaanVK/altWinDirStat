// globalhelpers.h	- Declaration of global helper functions
//
// see `file_header_text.txt` for licensing & contact info. If you can't find that file, then assume you're NOT allowed to do whatever you wanted to do.

#pragma once

#include "stdafx.h"

#ifndef WDS_GLOBALHELPERS_H
#define WDS_GLOBALHELPERS_H

WDS_FILE_INCLUDE_MESSAGE

#include "datastructures.h"
#include "macros_that_scare_small_children.h"

//must be inline, else compiler bitches about ODR!
//should really just be a void* and a size_t?
template<typename type_struct_to_memset>
inline void memset_zero_struct( type_struct_to_memset& the_struct ) {
	static_assert( std::is_pod<type_struct_to_memset>::value, "can't memset a non-pod struct!" );
	static_assert( !std::is_polymorphic<type_struct_to_memset>::value, "can't memset a polymorphic type!" );
	static_assert( std::is_standard_layout<type_struct_to_memset>::value, "can't memset a non-standard layout struct!" );
	memset( &the_struct, 0, sizeof( the_struct ) );
	}

//must be inline, else compiler bitches about ODR!
template<typename type_struct_to_init>
inline type_struct_to_init zero_init_struct( ) {
	static_assert( std::is_pod<type_struct_to_init>::value, "can't memset a non-pod struct!" );
	static_assert( !std::is_polymorphic<type_struct_to_init>::value, "can't memset a polymorphic type!" );
	static_assert( std::is_standard_layout<type_struct_to_init>::value, "can't memset a non-standard layout struct!" );
	static_assert( std::is_trivially_default_constructible<type_struct_to_init>::value, "can't memset a struct that isn't trivially default constructable!" );
	static_assert( std::is_trivially_copyable<type_struct_to_init>::value, "might have trouble returning a non-trivially-copyable item by value. You've been warned!" );
	type_struct_to_init the_struct;
	//memset( &the_struct, 0, sizeof( the_struct ) );
	memset_zero_struct( the_struct );
	return the_struct;
	}



_Success_( SUCCEEDED( return ) )
const HRESULT WriteToStackBuffer_do_nothing( WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written );

static_assert( sizeof( short ) == sizeof( std::int16_t ), "y'all ought to check SRECT" );
struct SRECT final {
	/*
	  short-based RECT, saves 8 bytes compared to tagRECT
	  */
	SRECT( );

	SRECT( std::int16_t iLeft, std::int16_t iTop, std::int16_t iRight, std::int16_t iBottom );

	SRECT( const RECT& in );


	const int Width( ) const;

	const int Height( ) const;

	std::int16_t left;
	std::int16_t top;
	std::int16_t right;
	std::int16_t bottom;
	};




struct QPC_timer final {
	QPC_timer( );
	void begin( );
	void end( );
	const double total_time_elapsed( ) const;
	QPC_timer& operator=( const QPC_timer& in ) = delete;

	const std::int64_t m_frequency;
	std::int64_t m_start;
	std::int64_t m_end;
	};

_Pre_satisfies_( handle != INVALID_HANDLE_VALUE )
_At_( handle, _Post_invalid_ )
_At_( handle, _Pre_valid_ )
void close_handle( const HANDLE handle );

_Pre_satisfies_( rect.left > rect.right ) _Post_satisfies_( rect.left <= rect.right )
inline void normalize_RECT_left_right( _Inout_ RECT& rect );

_Pre_satisfies_( rect.top > rect.bottom ) _Post_satisfies_( rect.top <= rect.bottom )
inline void normalize_RECT_top_bottom( _Inout_ RECT& rect );

_Post_satisfies_( rect.left <= rect.right ) _Post_satisfies_( rect.top <= rect.bottom )
void normalize_RECT( _Inout_ RECT& rect );

void error_getting_pointer_to( _In_z_ PCWSTR const function_name );
void test_if_null_funcptr( void* func_ptr, _In_z_ PCWSTR const function_name );

void InitializeCriticalSection_wrapper( _Pre_invalid_ _Post_valid_ _Out_ CRITICAL_SECTION& cs );

void DeleteCriticalSection_wrapper( _Pre_valid_ _Post_invalid_ CRITICAL_SECTION& cs );

//On returning E_FAIL, call GetLastError for details. That's not my idea!
_Success_( SUCCEEDED( return ) ) HRESULT CStyle_GetLastErrorAsFormattedMessage( WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_formatted_error, _In_range_( 128, 32767 ) const rsize_t strSize, _Out_ rsize_t& chars_written, const DWORD error = GetLastError( ) );

void unexpected_strsafe_invalid_parameter_handler( _In_z_ PCSTR const strsafe_func_name, _In_z_ PCSTR const file_name_in, _In_z_ PCSTR const func_name_in, _In_ _In_range_( 0, INT_MAX ) const int line_number_in );

void handle_stack_insufficient_buffer( _In_ const rsize_t str_size, _In_ const rsize_t generic_size_needed, _Out_ rsize_t& size_buff_need, _Out_ rsize_t& chars_written );


//WinDirStat string-formatting functions
namespace wds_fmt {



	_Success_( SUCCEEDED( return ) ) HRESULT CStyle_FormatFileTime  ( _In_ const FILETIME t,    _Out_writes_z_( strSize ) _Pre_writable_size_( strSize ) PWSTR psz_formatted_datetime, _In_range_( 128, 2048 ) const rsize_t strSize, _Out_ rsize_t& chars_written );


	_Success_( SUCCEEDED( return ) ) HRESULT CStyle_GetNumberFormatted( const std::int64_t number, _Pre_writable_size_( strSize ) PWSTR psz_formatted_number, _In_range_( 21, 64 ) const rsize_t strSize, _Out_ rsize_t& chars_written );

	

	void write_MEM_INFO_ERR ( _Out_writes_z_( 13 ) _Pre_writable_size_( 13 ) PWSTR psz_formatted_usage );

	void write_RAM_USAGE    ( _Out_writes_z_( 12 ) _Pre_writable_size_( 13 ) PWSTR psz_ram_usage );
	
	void write_bad_fmt_msg  ( _Out_writes_z_( 41 ) _Pre_writable_size_( 42 ) _Post_readable_size_( chars_written ) PWSTR psz_fmt_msg, _Out_ rsize_t& chars_written );
	
	
	}


_Success_( SUCCEEDED( return ) ) const HRESULT allocate_and_copy_name_str( _Pre_invalid_ _Post_z_ _Post_readable_size_( new_name_length ) wchar_t*& new_name_ptr, _In_ _In_range_( 0, UINT16_MAX ) const rsize_t& new_name_length, const std::wstring& name );

void displayWindowsMsgBoxWithError  ( const DWORD error = GetLastError( ) );

void displayWindowsMsgBoxWithMessage( const std::wstring message );
void displayWindowsMsgBoxWithMessage( const std::string message );
void displayWindowsMsgBoxWithMessage( PCWSTR const message );


const LARGE_INTEGER help_QueryPerformanceCounter( );
const LARGE_INTEGER help_QueryPerformanceFrequency( );

RECT BuildRECT( const SRECT& in );

_Pre_satisfies_( min_val < max_val )
_Post_satisfies_( min_val <= val )
_Post_satisfies_( val <= max_val )
void CheckMinMax( _Inout_ LONG& val, _In_ const LONG min_val, _In_ const LONG max_val );

_Pre_satisfies_( min_val < max_val )
_Post_satisfies_( min_val <= val )
_Post_satisfies_( val <= max_val )
void CheckMinMax( _Inout_ LONG& val, _In_ const INT min_val, _In_ const INT max_val );

_Pre_satisfies_( min_val < max_val )
_Post_satisfies_( min_val <= val )
_Post_satisfies_( val <= max_val )
void CheckMinMax( _Inout_ INT& val,  _In_ const INT min_val, _In_ const INT max_val );

bool Compare_FILETIME_eq   ( const FILETIME& lhs, const FILETIME& rhs ) ;

void NormalizeColor( _Inout_ _Out_range_( 0, 255 ) INT& red, _Inout_ _Out_range_( 0, 255 ) INT& green, _Inout_ _Out_range_( 0, 255 ) INT& blue ) ;

namespace CColorSpace {

	// Gives a color a defined brightness.
	//static COLORREF MakeBrightColor( _In_ const COLORREF color, _In_ _In_range_(0, 1) const DOUBLE brightness );
	COLORREF MakeBrightColor( _In_ const COLORREF color, _In_ _In_range_(0, 1) const DOUBLE brightness );
	}

#ifdef DEBUG

#ifdef COLOR_DEBUGGING
void trace_m_stripe_color_make_bright_color( _In_ const COLORREF m_windowColor, _In_ const DOUBLE b );
void trace_m_stripeColor( _In_ const COLORREF m_stripeColor );
#endif

void trace_on_destroy( _In_z_ PCWSTR const m_persistent_name );






void trace_full_path( _In_z_ PCWSTR const path );
#endif

// Collection of all treemap options.
struct Treemap_Options final {
	                                Treemap_STYLE style;        // Squarification method
									//C4820: 'Treemap_Options' : '3' bytes padding added after data member 'Treemap_Options::grid'
	                                bool          grid;         // Whether or not to draw grid lines
									//C4820: 'Treemap_Options' : '4' bytes padding added after data member 'Treemap_Options::gridColor'
	                                COLORREF      gridColor;    // Color of grid lines
	_Field_range_(  0, 1          ) DOUBLE        brightness;   // (default = 0.84)
	_Field_range_(  0, UINT64_MAX ) DOUBLE        height;       // (default = 0.40)  Factor "H (really range should be 0...std::numeric_limits<double>::max/100"
	_Field_range_(  0, 1          ) DOUBLE        scaleFactor;  // (default = 0.90)  Factor "F"
	_Field_range_(  0, 1          ) DOUBLE        ambientLight; // (default = 0.15)  Factor "Ia"
	_Field_range_( -4, 4          ) DOUBLE        lightSourceX; // (default = -1.0), negative = left
	_Field_range_( -4, 4          ) DOUBLE        lightSourceY; // (default = -1.0), negative = top

	_Ret_range_( 0, 100 ) INT    GetBrightnessPercent  ( ) const;
	_Ret_range_( 0, 100 ) INT    GetHeightPercent      ( ) const;
	_Ret_range_( 0, 100 ) INT    GetScaleFactorPercent ( ) const;
	_Ret_range_( 0, 100 ) INT    GetAmbientLightPercent( ) const;
	_Ret_range_( 0, 100 ) INT    GetLightSourceXPercent( ) const;
	_Ret_range_( 0, 100 ) INT    GetLightSourceYPercent( ) const;
		                  POINT  GetLightSourcePoint   ( ) const;

	_Ret_range_( 0, 100 ) INT    RoundDouble ( const DOUBLE d ) const;

	void SetBrightnessPercent  ( const INT    n   );
	void SetHeightPercent      ( const INT    n   );
	void SetScaleFactorPercent ( const INT    n   );
	void SetAmbientLightPercent( const INT    n   );
	void SetLightSourceXPercent( const INT    n   );
	void SetLightSourceYPercent( const INT    n   );
	void SetLightSourcePoint   ( const POINT  pt  );
	};



static const Treemap_Options _defaultOptions = { Treemap_STYLE::KDirStatStyle, false, RGB( 0, 0, 0 ), 0.88, 0.38, 0.91, 0.13, -1.0, -1.0 };

#endif
