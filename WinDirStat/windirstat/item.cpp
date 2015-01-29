// item.cpp	- Implementation of CItemBranch
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2005 Bernhard Seifert
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: bseifert@users.sourceforge.net, bseifert@daccord.net
//
// Last modified: $Date$

#include "stdafx.h"


//encourage inter-procedural optimization (and class-hierarchy analysis!)
#include "ownerdrawnlistcontrol.h"
#include "TreeListControl.h"
#include "item.h"
#include "typeview.h"


#include "globalhelpers.h"
#include "options.h"
#include "windirstat.h"

#ifdef _DEBUG
#include "dirstatdoc.h"
#endif




namespace {

	_Success_( return != UINT64_MAX )
	std::uint64_t GetCompressedFileSize_filename( const std::wstring path ) {
		ULARGE_INTEGER ret;
		ret.QuadPart = 0;//zero initializing this is critical!
		ret.LowPart = GetCompressedFileSizeW( path.c_str( ), &ret.HighPart );
	#ifdef PERF_DEBUG_SLEEP
		Sleep( 0 );
		Sleep( 10 );
	#endif
		const auto last_err = GetLastError( );
#ifdef DEBUG
		const rsize_t error_message_buffer_size = 128;
		wchar_t error_message_buffer[ error_message_buffer_size ] = { 0 };
		rsize_t chars_written = 0;
		const HRESULT fmt_res = CStyle_GetLastErrorAsFormattedMessage( error_message_buffer, error_message_buffer_size, chars_written, last_err );
#endif
		if ( ret.QuadPart == INVALID_FILE_SIZE ) {
			if ( ret.HighPart != NULL ) {
				if ( last_err != NO_ERROR ) {
#ifdef DEBUG
					if ( SUCCEEDED( fmt_res ) ) {
						TRACE( _T( "Error! Filepath: %s, Filepath length: %i, GetLastError: %s\r\n" ), path.c_str( ), path.length( ), error_message_buffer );
						}
					else {
						TRACE( _T( "Error! Filepath: %s, Filepath length: %i. Failed to get error message for error code: %u\r\n" ), path.c_str( ), path.length( ), last_err );
						}
#endif
					return UINT64_ERROR;// IN case of an error return size from CFileFind object
					}
#ifdef DEBUG
				if ( SUCCEEDED( fmt_res ) ) {
					TRACE( _T( "WTF ERROR! File path: %s, File path length: %i, GetLastError: %s\r\n" ), path.c_str( ), path.length( ), error_message_buffer );
					}
				else {
					TRACE( _T( "WTF ERROR! File path: %s, File path length: %i. Failed to get error message for error code: %u\r\n" ), path.c_str( ), path.length( ), last_err );
					}
#endif
				return UINT64_ERROR;
				}
			else {
				if ( last_err != NO_ERROR ) {
#ifdef DEBUG
					if ( SUCCEEDED( fmt_res ) ) {
						TRACE( _T( "Error! File path: %s, File path length: %i, GetLastError: %s\r\n" ), path.c_str( ), path.length( ), error_message_buffer );
						}
					else {
						TRACE( _T( "Error! File path: %s, File path length: %i. Failed to get error message for error code: %u\r\n" ), path.c_str( ), path.length( ), last_err );
						}
#endif
					return UINT64_ERROR;
					}
				return ret.QuadPart;
				}
			}
		return ret.QuadPart;
		}

	class get_size_agent : public concurrency::agent {
		concurrency::ISource<std::pair< CItemBranch*, std::wstring >>& source;
		concurrency::ITarget<std::pair< CItemBranch*, std::uint64_t>>& target;
		public:
			explicit get_size_agent( concurrency::ISource<std::pair< CItemBranch*, std::wstring >>& source_in, concurrency::ITarget<std::pair< CItemBranch*, std::uint64_t>>& target_in ) : source( source_in ), target( target_in ) { }
			get_size_agent( const get_size_agent& in ) = delete;
			get_size_agent& operator=( const get_size_agent& in ) = delete;
		protected:
			void run( ) {
				auto recv = concurrency::receive( source );
				do {
					CItemBranch* const ptr = recv.first;
					const auto path = recv.second;
					if ( !concurrency::asend( target, std::make_pair( ptr, GetCompressedFileSize_filename( std::move( path ) ) ) ) ) {
						concurrency::send( target, std::make_pair( ptr, GetCompressedFileSize_filename( std::move( path ) ) ) );
						}
					recv = concurrency::receive( source );
					} while ( recv.first != nullptr );
				concurrency::send( target, std::make_pair( static_cast<CItemBranch*>( nullptr ), 0ui64 ) );
				done( );
				}
		};

	class fixup_child_agent : public concurrency::agent {
		concurrency::ISource<std::pair< CItemBranch*, std::uint64_t>>& source;
		public:
			explicit fixup_child_agent( concurrency::ISource<std::pair< CItemBranch*, std::uint64_t>>& source_in ) : source( source_in ) { }
			fixup_child_agent( const fixup_child_agent& in ) = delete;
			fixup_child_agent& operator=( const fixup_child_agent& in ) = delete;
		protected:
			void run( ) {
				auto recv = concurrency::receive( source );
				do {
					const auto child = recv.first;
					const auto size_child = recv.second;
					if ( size_child != UINT64_MAX ) {
						ASSERT( child != NULL );
						if ( child != NULL ) {
							child->m_size = std::move( size_child );
							}
						}
					else {
						TRACE( _T( "ERROR returned by GetCompressedFileSize! file: %s\r\n" ), child->m_name.get( ) );
						child->m_attr.invalid = true;
						}
					recv = concurrency::receive( source );
					} while ( recv.first != nullptr );
				done( );
				}
		};

	class fixup_dispatcher_agent : public concurrency::agent {
		concurrency::ITarget<std::pair< CItemBranch*, std::wstring >>& target;
		concurrency::concurrent_vector<pair_of_item_and_path>* sizes;
		public:
			fixup_dispatcher_agent( concurrency::ITarget<std::pair< CItemBranch*, std::wstring >>& target_in, _In_ concurrency::concurrent_vector<pair_of_item_and_path>* sizes_in ) : target( target_in ), sizes( sizes_in ) { }
			fixup_dispatcher_agent( const fixup_dispatcher_agent& in ) = delete;
			fixup_dispatcher_agent& operator=( const fixup_dispatcher_agent& in ) = delete;
		protected:
			void run( ) {
				const auto number_sizes = sizes->size( );
				//for ( size_t i = 0; i < number_sizes; ++i ) {
				//	sizesToWorkOn_.emplace_back( std::async( compose_compressed_file_size_and_fixup_child, ( *sizes )[ i ].ptr, ( *sizes )[ i ].path ) );
				//	}
				for ( size_t i = 0; i < number_sizes; ++i ) {
					concurrency::send( target, std::make_pair( ( *sizes )[ i ].ptr, ( *sizes )[ i ].path ) );
					}
				std::wstring nullstr;
				concurrency::send( target, std::make_pair( static_cast<CItemBranch*>( nullptr ), nullstr ) );
				done( );
				}
		};

	//void compose_compressed_file_size_and_fixup_child( CItemBranch* const child, const std::wstring path ) {
	//	//concurrency::create_task( [path] ( ) -> std::uint64_t {
	//	//	return GetCompressedFileSize_filename( path );
	//	//	} ).then( [child] ( std::uint64_t size_child ) {
	//	//		if ( size_child != UINT64_MAX ) {
	//	//			ASSERT( child != NULL );
	//	//			if ( child != NULL ) {
	//	//				child->m_size = std::move( size_child );
	//	//				}
	//	//			}
	//	//		else {
	//	//			TRACE( _T( "ERROR returned by GetCompressedFileSize! file: %s\r\n" ), child->m_name.get( ) );
	//	//			child->m_attr.invalid = true;
	//	//			}
	//	//		} ).wait( );


	//	//const auto size_child = GetCompressedFileSize_filename( path );
	//	//if ( size_child != UINT64_MAX ) {
	//	//	ASSERT( child != NULL );
	//	//	if ( child != NULL ) {
	//	//		child->m_size = std::move( size_child );
	//	//		}
	//	//	}
	//	//else {
	//	//	TRACE( _T( "ERROR returned by GetCompressedFileSize! file: %s\r\n" ), child->m_name.get( ) );
	//	//	child->m_attr.invalid = true;
	//	//	}
	//	}

	//void process_vector_of_compressed_file_futures( std::vector<std::pair<CItemBranch*, std::future<std::uint64_t>>>& vector_of_compressed_file_futures ) {
	//	const auto sizesToWorkOnCount = vector_of_compressed_file_futures.size( );
	//	for ( size_t i = 0; i < sizesToWorkOnCount; ++i ) {
	//	
	//		const auto sizeValue = vector_of_compressed_file_futures[ i ].second.get( );
	//		auto child = vector_of_compressed_file_futures[ i ].first;
	//		if ( sizeValue != UINT64_MAX ) {
	//		
	//			ASSERT( child != NULL );
	//			if ( child != NULL ) {
	//				child->m_size = std::move( sizeValue );
	//				}
	//			}
	//		else {
	//			TRACE( _T( "ERROR returned by GetCompressedFileSize! file: %s\r\n" ), child->m_name );
	//			child->m_attr.invalid = true;
	//			}
	//		}
	//	}

	//sizes_to_work_on_in NEEDS to be passed as a pointer, else bad things happen!
	std::vector<std::future<void>> start_workers( std::vector<std::pair<CItemBranch*, std::wstring>> dirs_to_work_on, _In_ const CDirstatApp* app, concurrency::concurrent_vector<pair_of_item_and_path>* sizes_to_work_on_in ) {
		const auto dirsToWorkOnCount = dirs_to_work_on.size( );
		std::vector<std::future<void>> workers;
		workers.reserve( dirsToWorkOnCount );
		for ( size_t i = 0; i < dirsToWorkOnCount; ++i ) {
			ASSERT( dirs_to_work_on[ i ].second.length( ) > 1 );
			ASSERT( dirs_to_work_on[ i ].second.back( ) != L'\\' );
			ASSERT( dirs_to_work_on[ i ].second.back( ) != L'*' );
			//TODO: investigate task_group
	#ifdef PERF_DEBUG_SLEEP
			Sleep( 0 );
			Sleep( 10 );
	#endif
			//using std::launch::async ( instead of the default, std::launch::any ) causes WDS to hang!
			workers.emplace_back( std::async( DoSomeWork, std::move( dirs_to_work_on[ i ].first ), std::move( dirs_to_work_on[ i ].second ), app, sizes_to_work_on_in, std::move( false ) ) );
			}
		return workers;
		}

	//sizes_to_work_on_in NEEDS to be passed as a pointer, else bad things happen!
	void size_workers( _In_ concurrency::concurrent_vector<pair_of_item_and_path>* sizes ) {
		//std::vector<std::pair<CItemBranch*, std::future<std::uint64_t>>> sizesToWorkOn_;

		concurrency::unbounded_buffer < std::pair< CItemBranch*, std::wstring > > buffer;
		
		concurrency::unbounded_buffer < std::pair< CItemBranch*, std::uint64_t > > half_completed;

		concurrency::Context::Oversubscribe( true );

		fixup_dispatcher_agent dispatcher( buffer, sizes );
		get_size_agent get_sizes_agent( buffer, half_completed );
		fixup_child_agent fixup_children_agent( half_completed );

		dispatcher.start( );

		get_sizes_agent.start( );
		
		fixup_children_agent.start( );

		concurrency::agent::wait( &dispatcher );
		concurrency::agent::wait( &get_sizes_agent );
		concurrency::agent::wait( &fixup_children_agent );

		concurrency::Context::Oversubscribe( false );
		//std::vector<std::future<void>> sizesToWorkOn_;
		//TRACE( _T( "need to get the compressed size for %I64u files!\r\n" ), std::uint64_t( sizes->size( ) ) );
		//sizesToWorkOn_.reserve( sizes->size( ) + 1 );
		//const auto number_sizes = sizes->size( );
		//for ( size_t i = 0; i < number_sizes; ++i ) {
		//	sizesToWorkOn_.emplace_back( std::async( compose_compressed_file_size_and_fixup_child, ( *sizes )[ i ].ptr, ( *sizes )[ i ].path ) );
		//	}
		//
		//for ( size_t i = 0; i < number_sizes; ++i ) {
		//	sizesToWorkOn_[ i ].get( );
		//	}
		
		//process_vector_of_compressed_file_futures( sizesToWorkOn_ );
		}
	}

void FindFilesLoop( _Inout_ std::vector<FILEINFO>& files, _Inout_ std::vector<DIRINFO>& directories, const std::wstring path ) {
	ASSERT( path.back( ) == L'*' );
	WIN32_FIND_DATA fData;
	HANDLE fDataHand = NULL;
	fDataHand = FindFirstFileExW( path.c_str( ), FindExInfoBasic, &fData, FindExSearchNameMatch, NULL, 0 );
	//FILETIME t;
	FILEINFO fi;
	//zeroFILEINFO( fi );
	//memset_zero_struct( fi );
	fi.reset( );
	BOOL findNextFileRes = TRUE;
	while ( ( fDataHand != INVALID_HANDLE_VALUE ) && ( findNextFileRes != 0 ) ) {
		auto scmpVal  = wcscmp( fData.cFileName, L".." );
		auto scmpVal2 = wcscmp( fData.cFileName, L"." );
		if ( ( scmpVal == 0 ) || ( scmpVal2 == 0 ) ) {//This branches on the return of IsDirectory, then checks characters 0,1, & 2//IsDirectory calls MatchesMask, which bitwise-ANDs dwFileAttributes with FILE_ATTRIBUTE_DIRECTORY
			findNextFileRes = FindNextFileW( fDataHand, &fData );
			continue;//No point in operating on ourselves!
			}
		else if ( fData.dwFileAttributes bitand FILE_ATTRIBUTE_DIRECTORY ) {
			ASSERT( path.substr( path.length( ) - 3, 3 ).compare( L"*.*" ) == 0 );
			const auto alt_path_this_dir( path.substr( 0, path.length( ) - 3 ) + fData.cFileName );
			directories.emplace_back( DIRINFO { 0,
												std::move( fData.ftLastWriteTime ),
												std::move( fData.dwFileAttributes ),
												fData.cFileName,
												std::move( alt_path_this_dir )
											  }
									);
			}
		else {
			files.emplace_back( FILEINFO {  std::move( ( static_cast<std::uint64_t>( fData.nFileSizeHigh ) * ( static_cast<std::uint64_t>( MAXDWORD ) + 1 ) ) + static_cast<std::uint64_t>( fData.nFileSizeLow ) ), 
											fData.ftLastWriteTime,
											fData.dwFileAttributes,
											fData.cFileName
										 }
							  );
			}
		findNextFileRes = FindNextFileW( fDataHand, &fData );
		}
	if ( fDataHand == INVALID_HANDLE_VALUE ) {
		return;
		}
	VERIFY( FindClose( fDataHand ) );
	}

std::vector<std::pair<CItemBranch*, std::wstring>> addFiles_returnSizesToWorkOn( _In_ CItemBranch* const ThisCItem, std::vector<FILEINFO>& vecFiles, const std::wstring& path ) {
	std::vector<std::pair<CItemBranch*, std::wstring>> sizesToWorkOn_;
	std::sort( vecFiles.begin( ), vecFiles.end( ) );
	sizesToWorkOn_.reserve( vecFiles.size( ) );

	ASSERT( path.back( ) != _T( '\\' ) );
	for ( const auto& aFile : vecFiles ) {
		if ( ( aFile.attributes bitand FILE_ATTRIBUTE_COMPRESSED ) != 0 ) {
			const auto new_name_length = aFile.name.length( );
			ASSERT( new_name_length < UINT16_MAX );

			PWSTR new_name_ptr = nullptr;
			const HRESULT copy_res = allocate_and_copy_name_str( new_name_ptr, new_name_length, aFile.name );
			if ( !SUCCEEDED( copy_res ) ) {
				displayWindowsMsgBoxWithMessage( L"Failed to allocate & copy (compressed) name str! (addFiles_returnSizesToWorkOn)(aborting!)" );
				displayWindowsMsgBoxWithMessage( aFile.name.c_str( ) );
				}
			else {
				//                                                                                            IT_FILE
				auto newChild = ::new ( &( ThisCItem->m_children[ ThisCItem->m_childCount ] ) ) CItemBranch { std::move( aFile.length ), std::move( aFile.lastWriteTime ), std::move( aFile.attributes ), true, ThisCItem, new_name_ptr, static_cast< std::uint16_t >( new_name_length ) };
				//using std::launch::async ( instead of the default, std::launch::any ) causes WDS to hang!
				//sizesToWorkOn_.emplace_back( std::move( newChild ), std::move( std::async( GetCompressedFileSize_filename, std::move( path + _T( '\\' ) + aFile.name  ) ) ) );
				sizesToWorkOn_.emplace_back( std::move( newChild ), std::move( path + _T( '\\' ) + aFile.name  ) );
				}

#ifdef PERF_DEBUG_SLEEP
		Sleep( 0 );
		Sleep( 10 );
#endif
			}
		else {
			const auto new_name_length = aFile.name.length( );
			ASSERT( new_name_length < UINT16_MAX );
			PWSTR new_name_ptr = nullptr;
			const HRESULT copy_res = allocate_and_copy_name_str( new_name_ptr, new_name_length, aFile.name );
			if ( !SUCCEEDED( copy_res ) ) {
				displayWindowsMsgBoxWithMessage( L"Failed to allocate & copy (uncompressed) name str! (addFiles_returnSizesToWorkOn)(aborting!)" );
				displayWindowsMsgBoxWithMessage( aFile.name.c_str( ) );
				}
			else {
				//                                                                            IT_FILE
				::new ( &( ThisCItem->m_children[ ThisCItem->m_childCount ] ) ) CItemBranch { std::move( aFile.length ), std::move( aFile.lastWriteTime ), std::move( aFile.attributes ), true, ThisCItem, new_name_ptr, static_cast< std::uint16_t >( new_name_length ) };
				}
			}
		//detect overflows. highly unlikely.
		ASSERT( ThisCItem->m_childCount < 4294967290 );
		++( ThisCItem->m_childCount );
		}
	return sizesToWorkOn_;
	}

_Pre_satisfies_( !ThisCItem->m_attr.m_done ) std::pair<std::vector<std::pair<CItemBranch*, std::wstring>>,std::vector<std::pair<CItemBranch*, std::wstring>>> readJobNotDoneWork( _In_ CItemBranch* const ThisCItem, std::wstring path, _In_ const CDirstatApp* app ) {
	std::vector<FILEINFO> vecFiles;
	std::vector<DIRINFO>  vecDirs;

	vecFiles.reserve( 50 );//pseudo-arbitrary number

	FindFilesLoop( vecFiles, vecDirs, std::move( path + _T( "\\*.*" ) ) );

	//std::sort( vecFiles.begin( ), vecFiles.end( ) );

	const auto fileCount = vecFiles.size( );
	const auto dirCount  = vecDirs.size( );
	
	const auto total_count = ( fileCount + dirCount );

	ASSERT( ThisCItem->m_childCount == 0 );
	if ( total_count > 0 ) {
		ThisCItem->m_children.reset( new CItemBranch[ total_count ] );
		}
	////true for 2 means DIR

	//ASSERT( path.back( ) != _T( '\\' ) );
	//sizesToWorkOn_ CANNOT BE CONST!!
	auto sizesToWorkOn_ = addFiles_returnSizesToWorkOn( ThisCItem, vecFiles, path );
	std::vector<std::pair<CItemBranch*, std::wstring>> dirsToWorkOn;
	dirsToWorkOn.reserve( dirCount );
	const auto thisOptions = GetOptions( );

	//TODO IsJunctionPoint calls IsMountPoint deep in IsJunctionPoint's bowels. This means triplicated calls.
	for ( const auto& dir : vecDirs ) {
		const bool dontFollow = ( app->m_mountPoints.IsJunctionPoint( dir.path, dir.attributes ) && !thisOptions->m_followJunctionPoints ) || ( app->m_mountPoints.IsMountPoint( dir.path ) && !thisOptions->m_followMountPoints );
		const auto new_name_length = dir.name.length( );
		ASSERT( new_name_length < UINT16_MAX );

		PWSTR new_name_ptr = nullptr;
		const HRESULT copy_res = allocate_and_copy_name_str( new_name_ptr, new_name_length, dir.name );
		if ( !SUCCEEDED( copy_res ) ) {
			displayWindowsMsgBoxWithMessage( L"Failed to allocate & copy (directory) name str! (readJobNotDoneWork)(aborting!)" );
			displayWindowsMsgBoxWithMessage( dir.name.c_str( ) );
			}
		else {
			//                                                                                               IT_DIRECTORY
			const auto newitem = new ( &( ThisCItem->m_children[ ThisCItem->m_childCount ] ) ) CItemBranch { static_cast< std::uint64_t >( 0u ), std::move( dir.lastWriteTime ), std::move( dir.attributes ), dontFollow, ThisCItem, new_name_ptr, static_cast< std::uint16_t >( new_name_length ) };

			//detect overflows. highly unlikely.
			ASSERT( ThisCItem->m_childCount < 4294967290 );

			++( ThisCItem->m_childCount );
			//ThisCItem->m_children_vector.emplace_back( newitem );

			if ( !newitem->m_attr.m_done ) {
				ASSERT( !dontFollow );
				dirsToWorkOn.emplace_back( std::move( std::make_pair( std::move( newitem ), std::move( dir.path ) ) ) );
				}
			else {
				ASSERT( dontFollow );
				}
			}
		}
	ASSERT( ( fileCount + dirCount ) == ThisCItem->m_childCount );
	//ThisCItem->m_children_vector.shrink_to_fit( );
	return std::make_pair( std::move( dirsToWorkOn ), std::move( sizesToWorkOn_ ) );
	}

_Pre_satisfies_( this->m_parent == NULL ) void CItemBranch::AddChildren( ) {
	ASSERT( GetDocument( )->IsRootDone( ) );
	ASSERT( m_attr.m_done );
	if ( m_parent == NULL ) {
		GetTreeListControl( )->OnChildAdded( NULL, this, false );
		}
	}

DOUBLE DoSomeWorkShim( _In_ CItemBranch* const ThisCItem, std::wstring path, _In_ const CDirstatApp* app, const bool isRootRecurse ) {
	//some sync primitive
	//http://msdn.microsoft.com/en-us/library/ff398050.aspx
	ASSERT( ThisCItem->m_childCount == 0 );
	ASSERT( ThisCItem->m_children.get( ) == nullptr );
	auto strcmp_path = path.compare( 0, 4, L"\\\\?\\", 0, 4 );
	ASSERT( strcmp_path == 0 );
	if ( strcmp_path != 0 ) {
		auto fixedPath( L"\\\\?\\" + path );
		TRACE( _T( "path fixed as: %s\r\n" ), fixedPath.c_str( ) );
		path = fixedPath;
		}
	if ( path.back( ) == L'\\' ) {
		path.pop_back( );
		}
	concurrency::concurrent_vector<pair_of_item_and_path> sizes_to_work_on;

	const auto qpc_1 = help_QueryPerformanceCounter( );
	
	DoSomeWork( std::move( ThisCItem ), std::move( path ), app, &sizes_to_work_on, std::move( isRootRecurse ) );
	
	const auto qpc_2 = help_QueryPerformanceCounter( );
	const auto qpf = help_QueryPerformanceFrequency( );

	const auto timing = ( static_cast<double>( qpc_2.QuadPart - qpc_1.QuadPart ) * ( static_cast<double>( 1.0 ) / static_cast<double>( qpf.QuadPart ) ) );
	const rsize_t debug_buf_size = 96;
	wchar_t debug_buf[ debug_buf_size ] = { 0 };
	const auto debug_buf_res_1 = _snwprintf_s( debug_buf, debug_buf_size, _TRUNCATE, L"WDS: enum timing: %f\r\n", timing );
	ASSERT( debug_buf_res_1 != -1 );
	if ( debug_buf_res_1 == -1 ) {
		OutputDebugStringW( global_strings::output_dbg_string_error );
		//std::terminate( );
		}

	OutputDebugStringW( debug_buf );


	const auto qpc_3 = help_QueryPerformanceCounter( );

	size_workers( &sizes_to_work_on );

	const auto qpc_4 = help_QueryPerformanceCounter( );
	const auto qpf_2 = help_QueryPerformanceFrequency( );
	const auto timing_2 = ( static_cast<double>( qpc_4.QuadPart - qpc_3.QuadPart ) * ( static_cast<double>( 1.0 ) / static_cast<double>( qpf_2.QuadPart ) ) );

	
	wchar_t debug_buf_2[ debug_buf_size ] = { 0 };
	const auto debug_buf_res_2 = _snwprintf_s( debug_buf_2, debug_buf_size, _TRUNCATE, L"WDS: compressed file timing: %f\r\n", timing_2 );
	ASSERT( debug_buf_res_2 != -1 );
	if ( debug_buf_res_2 == -1 ) {
		OutputDebugStringW( global_strings::output_dbg_string_error );
		//std::terminate( );
		}

	OutputDebugStringW( debug_buf_2 );

	return timing_2;
	//wait for sync?
	}

//sizes_to_work_on_in NEEDS to be passed as a pointer, else bad things happen!
void DoSomeWork( _In_ CItemBranch* const ThisCItem, std::wstring path, _In_ const CDirstatApp* app, concurrency::concurrent_vector<pair_of_item_and_path>* sizes_to_work_on_in, const bool isRootRecurse ) {
	//This is temporary.
	UNREFERENCED_PARAMETER( isRootRecurse );

	ASSERT( wcscmp( L"\\\\?\\", path.substr( 0, 4 ).c_str( ) ) == 0 );
	auto strcmp_path = path.compare( 0, 4, L"\\\\?\\", 0, 4 );
	if ( strcmp_path != 0 ) {
		auto fixedPath( L"\\\\?\\" + path );
		TRACE( _T( "path fixed as: %s\r\n" ), fixedPath.c_str( ) );
		path = fixedPath;
		}
	auto itemsToWorkOn = readJobNotDoneWork( ThisCItem, std::move( path ), app );
	if ( ThisCItem->m_childCount == 0 ) {
		ASSERT( itemsToWorkOn.first.size( ) == 0 );
		ASSERT( itemsToWorkOn.second.size( ) == 0 );
		ThisCItem->m_attr.m_done = true;
		return;
		}

	//std::vector<std::pair<CItemBranch*, std::wstring>>& dirs_to_work_on = itemsToWorkOn.first;

	//auto workers = start_workers( std::move( dirs_to_work_on ), app, sizes_to_work_on_in );

	auto workers = start_workers( std::move( itemsToWorkOn.first ), app, sizes_to_work_on_in );

	//std::vector<std::pair<CItemBranch*, std::future<std::uint64_t>>>& vector_of_compressed_file_futures = itemsToWorkOn.second;

	//process_vector_of_compressed_file_futures( vector_of_compressed_file_futures );


	std::vector<std::pair<CItemBranch*, std::wstring>>& vector_of_compressed_file_futures = itemsToWorkOn.second;

	//Not vectorized: 1304, loop includes assignments of different sizes
	for ( auto& a_pair : vector_of_compressed_file_futures ) {
		pair_of_item_and_path the_pair;
		the_pair.ptr  = a_pair.first;
		the_pair.path = std::move( a_pair.second );
		sizes_to_work_on_in->push_back( the_pair );
		}

	for ( auto& worker : workers ) {
#ifdef PERF_DEBUG_SLEEP
		Sleep( 0 );
		Sleep( 10 );
#endif
		worker.get( );
		}

	ThisCItem->m_attr.m_done = true;
	return;
	}

//

CItemBranch::CItemBranch( std::uint64_t size, FILETIME time, DWORD attr, bool done, _In_ CItemBranch* parent, _In_z_ _Readable_elements_( length ) PCWSTR name, const std::uint16_t length ) : m_size( size ), m_rect( 0, 0, 0, 0 ), m_lastChange( std::move( time ) ), m_childCount( 0 ), m_children( nullptr ), CTreeListItem( std::move( name ), std::move( length ), std::move( parent ) ) {
	//m_vi( nullptr );
	SetAttributes( attr );
	m_attr.m_done = done;
	//m_name = std::move( name );
	}

//CItemBranch::~CItemBranch( ) {
//	//delete[ ] m_children;
//	//m_children = nullptr;
//	//m_childCount = 0;
//	//m_children_vector.clear( );
//	}

//_Pre_satisfies_( subitem == column::COL_NAME ) _Success_( SUCCEEDED( return ) )
//HRESULT CItemBranch::WriteToStackBuffer_COL_NAME( RANGE_ENUM_COL const column::ENUM_COL subitem, WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, _Out_ _On_failure_( _Post_valid_ ) rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written ) const {
//#ifndef DEBUG
//	UNREFERENCED_PARAMETER( subitem );
//#endif
//	ASSERT( subitem == column::COL_NAME );
//	size_t chars_remaining = 0;
//	const auto res = StringCchCopyExW( psz_text, strSize, m_name.get( ), NULL, &chars_remaining, 0 );
//		
//	chars_written = m_name_length;
//	if ( res == STRSAFE_E_INSUFFICIENT_BUFFER ) {
//		chars_written = strSize;
//		sizeBuffNeed = static_cast<rsize_t>( m_name_length + 2u );
//		}
//	else if ( ( res != STRSAFE_E_INSUFFICIENT_BUFFER ) && ( FAILED( res ) ) ) {
//		chars_written = 0;
//		sizeBuffNeed = static_cast<rsize_t>( m_name_length + 2u );
//		}
//	else {
//		ASSERT( SUCCEEDED( res ) );
//		if ( SUCCEEDED( res ) ) {
//			chars_written = ( strSize - chars_remaining );
//			sizeBuffNeed = SIZE_T_ERROR;
//			}
//		else {
//			sizeBuffNeed = static_cast< rsize_t >( m_name_length + 2u );
//			}
//		}
//	return res;
//	}

_Pre_satisfies_( subitem == column::COL_PERCENTAGE ) _Success_( SUCCEEDED( return ) )
HRESULT CItemBranch::WriteToStackBuffer_COL_PERCENTAGE( RANGE_ENUM_COL const column::ENUM_COL subitem, WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written ) const {
	//auto res = StringCchPrintfW( psz_text, strSize, L"%.1f%%", ( GetFraction( ) * static_cast<DOUBLE>( 100 ) ) );
#ifndef DEBUG
	UNREFERENCED_PARAMETER( subitem );
#endif
	ASSERT( subitem == column::COL_PERCENTAGE );
	size_t chars_remaining = 0;
	const auto percentage = ( GetFraction( ) * static_cast< DOUBLE >( 100 ) );
	ASSERT( percentage <= 100.00 );
	const HRESULT res = StringCchPrintfExW( psz_text, strSize, NULL, &chars_remaining, 0, L"%.1f%%", percentage );
	if ( res == STRSAFE_E_INSUFFICIENT_BUFFER ) {
		chars_written = strSize;
		sizeBuffNeed = 64;//Generic size needed.
		}
	else if ( ( res != STRSAFE_E_INSUFFICIENT_BUFFER ) && ( FAILED( res ) ) ) {
		chars_written = 0;
		}
	else {
		ASSERT( SUCCEEDED( res ) );
		if ( SUCCEEDED( res ) ) {
			chars_written = ( strSize - chars_remaining );
			}
		}
	ASSERT( SUCCEEDED( res ) );
	ASSERT( chars_written == wcslen( psz_text ) );
	return res;
	}

_Pre_satisfies_( subitem == column::COL_SUBTREETOTAL ) _Success_( SUCCEEDED( return ) )
HRESULT CItemBranch::WriteToStackBuffer_COL_SUBTREETOTAL( RANGE_ENUM_COL const column::ENUM_COL subitem, WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written ) const {
#ifndef DEBUG
	UNREFERENCED_PARAMETER( subitem );
#endif
	ASSERT( subitem == column::COL_SUBTREETOTAL );
	auto res = FormatBytes( size_recurse( ), psz_text, strSize, chars_written );
	if ( res == STRSAFE_E_INSUFFICIENT_BUFFER ) {
		chars_written = strSize;
		sizeBuffNeed = 64;//Generic size needed.
		}
	else if ( ( res != STRSAFE_E_INSUFFICIENT_BUFFER ) && ( FAILED( res ) ) ) {
		chars_written = 0;
		}
	ASSERT( SUCCEEDED( res ) );
	ASSERT( chars_written == wcslen( psz_text ) );
	return res;
	}

_Pre_satisfies_( ( subitem == column::COL_FILES ) || ( subitem == column::COL_ITEMS ) ) _Success_( SUCCEEDED( return ) )
HRESULT CItemBranch::WriteToStackBuffer_COL_FILES( RANGE_ENUM_COL const column::ENUM_COL subitem, WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written ) const {
#ifndef DEBUG
	UNREFERENCED_PARAMETER( subitem );
#endif
	ASSERT( ( subitem == column::COL_FILES ) || ( subitem == column::COL_ITEMS ) );
	const auto number_to_format = files_recurse( );
	const HRESULT num_fmt_Res = CStyle_GetNumberFormatted( number_to_format, psz_text, strSize, chars_written );

	if ( SUCCEEDED( num_fmt_Res ) ) {
		return num_fmt_Res;
		}
	sizeBuffNeed = ( ( strSize > 64 ) ? ( strSize * 2 ) : 128 );
	return num_fmt_Res;
	}

_Pre_satisfies_( subitem == column::COL_LASTCHANGE ) _Success_( SUCCEEDED( return ) )
HRESULT CItemBranch::WriteToStackBuffer_COL_LASTCHANGE( RANGE_ENUM_COL const column::ENUM_COL subitem, WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, _Out_ _On_failure_( _Post_valid_ ) rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written ) const {
#ifndef DEBUG
	UNREFERENCED_PARAMETER( subitem );
#endif
	ASSERT( subitem == column::COL_LASTCHANGE );
	const HRESULT res = CStyle_FormatFileTime( FILETIME_recurse( ), psz_text, strSize, chars_written );
	if ( SUCCEEDED( res ) ) {
		sizeBuffNeed = SIZE_T_ERROR;
		return S_OK;
		}
	ASSERT( SUCCEEDED( res ) );
	chars_written = { 0u };
	sizeBuffNeed = { 48u };

	//_CrtDbgBreak( );//not handled yet.
	return STRSAFE_E_INVALID_PARAMETER;
	}

_Pre_satisfies_( subitem == column::COL_ATTRIBUTES ) _Success_( SUCCEEDED( return ) )
HRESULT CItemBranch::WriteToStackBuffer_COL_ATTRIBUTES( RANGE_ENUM_COL const column::ENUM_COL subitem, WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written ) const {
#ifndef DEBUG
	UNREFERENCED_PARAMETER( subitem );
#endif
	ASSERT( subitem == column::COL_ATTRIBUTES );
	const HRESULT res = CStyle_FormatAttributes( m_attr, psz_text, strSize, chars_written );
	ASSERT( SUCCEEDED( res ) );
	if ( !SUCCEEDED( res ) ) {
		sizeBuffNeed = { 8u };//Generic size needed, overkill;
		chars_written = { 0u };
		//_CrtDbgBreak( );//not handled yet.
		return res;
		}
	ASSERT( chars_written == wcslen( psz_text ) );
	return res;
	}

_Success_( SUCCEEDED( return ) )
HRESULT CItemBranch::WriteToStackBuffer_default( WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written ) const {
	ASSERT( strSize > 8 );
	sizeBuffNeed = SIZE_T_ERROR;
	//auto res = StringCchPrintfW( psz_text, strSize, L"BAD GetText_WriteToStackBuffer - subitem" );
	size_t chars_remaining = 0;
	const HRESULT res = StringCchPrintfExW( psz_text, strSize, NULL, &chars_remaining, 0, L"BAD GetText_WriteToStackBuffer - subitem" );
	if ( res == STRSAFE_E_INSUFFICIENT_BUFFER ) {
		if ( strSize > 8 ) {
			write_BAD_FMT( psz_text, chars_written );
			}
		else {
			chars_written = strSize;
			displayWindowsMsgBoxWithMessage( std::wstring( L"CItemBranch::" ) + std::wstring( global_strings::write_to_stackbuffer_err ) );
			}
		}
	else if ( ( res != STRSAFE_E_INSUFFICIENT_BUFFER ) && ( FAILED( res ) ) ) {
		chars_written = 0;
		}

	if ( SUCCEEDED( res ) ) {
		chars_written = ( strSize - chars_remaining );
		}

	
	ASSERT( SUCCEEDED( res ) );
	ASSERT( chars_written == wcslen( psz_text ) );
	return res;
	}


_Must_inspect_result_ _Success_( SUCCEEDED( return ) )
HRESULT CItemBranch::Text_WriteToStackBuffer( RANGE_ENUM_COL const column::ENUM_COL subitem, WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, _Out_ _On_failure_( _Post_valid_ ) rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written ) const {
	switch ( subitem )
	{
			
			case column::COL_PERCENTAGE:
				return WriteToStackBuffer_COL_PERCENTAGE( subitem, psz_text, strSize, sizeBuffNeed, chars_written );
			case column::COL_SUBTREETOTAL:
				return WriteToStackBuffer_COL_SUBTREETOTAL( subitem, psz_text, strSize, sizeBuffNeed, chars_written );
			case column::COL_ITEMS:
			case column::COL_FILES:
				return WriteToStackBuffer_COL_FILES( subitem, psz_text, strSize, sizeBuffNeed, chars_written );
			case column::COL_LASTCHANGE:
				return WriteToStackBuffer_COL_LASTCHANGE( subitem, psz_text, strSize, sizeBuffNeed, chars_written );
			case column::COL_ATTRIBUTES:
				return WriteToStackBuffer_COL_ATTRIBUTES( subitem, psz_text, strSize, sizeBuffNeed, chars_written );
			case column::COL_NAME:
			default:
				return WriteToStackBuffer_default( psz_text, strSize, sizeBuffNeed, chars_written );
	}
	}


COLORREF CItemBranch::ItemTextColor( ) const {
	if ( m_attr.invalid ) {
		//return GetItemTextColor( true );
		return RGB( 0xFF, 0x00, 0x00 );
		}
	if ( m_attr.compressed ) {
		return RGB( 0x00, 0x00, 0xFF );
		}
	else if ( m_attr.encrypted ) {
		return GetApp( )->m_altEncryptionColor;
		}
	//ASSERT( GetItemTextColor( true ) == default_item_text_color( ) );
	//return GetItemTextColor( true ); // The rest is not colored
	return default_item_text_color( ); // The rest is not colored
	}

INT CItemBranch::CompareSibling( _In_ const CTreeListItem* const tlib, _In_ _In_range_( 0, INT32_MAX ) const column::ENUM_COL subitem ) const {
	auto other = static_cast< const CItemBranch* >( tlib );
	switch ( subitem ) {
			case column::COL_NAME:
				return signum( wcscmp( m_name.get( ), other->m_name.get( ) ) );
			case column::COL_PERCENTAGE:
				return signum( GetFraction( ) - other->GetFraction( ) );
			case column::COL_SUBTREETOTAL:
				return signum( static_cast<std::int64_t>( size_recurse( ) ) - static_cast<std::int64_t>( other->size_recurse( ) ) );
			case column::COL_ITEMS:
			case column::COL_FILES:
				return signum( static_cast<std::int64_t>( files_recurse( ) ) - static_cast<std::int64_t>( other->files_recurse( ) ) );
			case column::COL_LASTCHANGE:
				return Compare_FILETIME( FILETIME_recurse( ), other->FILETIME_recurse( ) );
			case column::COL_ATTRIBUTES:
				return signum( GetSortAttributes( ) - other->GetSortAttributes( ) );
			default:
				ASSERT( false );
				return 666;
		}
	}

std::vector<CTreeListItem*> CItemBranch::size_sorted_vector_of_children( ) const {
	std::vector<CTreeListItem*> children;
	const auto child_count = m_childCount;
	children.reserve( child_count );
	const auto local_m_children = m_children.get( );
	if ( m_children != nullptr ) {
		//Not vectorized: 1200, loop contains data dependencies
		for ( size_t i = 0; i < child_count; ++i ) {
			children.emplace_back( local_m_children + i );
			}
		}
#ifdef DEBUG
	else {
		ASSERT( m_childCount == 0 );
		}
#endif
	qsort( children.data( ), static_cast< const size_t >( children.size( ) ), sizeof( CTreeListItem* ), &CItem_compareBySize );
	//std::sort( children.begin( ), children.end( ), [] ( const CTreeListItem* const lhs, const CTreeListItem* const rhs ) { return static_cast< const CItemBranch* >( lhs )->size_recurse( ) < static_cast< const CItemBranch* >( rhs )->size_recurse( ); } );
	return children;
	}

//Encodes the attributes to fit (in) 1 byte
void CItemBranch::SetAttributes( _In_ const DWORD attr ) {
	if ( attr == INVALID_FILE_ATTRIBUTES ) {
		m_attr.invalid = true;
		return;
		}
	m_attr.readonly   = ( ( attr bitand FILE_ATTRIBUTE_READONLY      ) != 0 );
	m_attr.hidden     = ( ( attr bitand FILE_ATTRIBUTE_HIDDEN        ) != 0 );
	m_attr.system     = ( ( attr bitand FILE_ATTRIBUTE_SYSTEM        ) != 0 );
	//m_attr.archive    = ( ( attr bitand FILE_ATTRIBUTE_ARCHIVE       ) != 0 );
	m_attr.compressed = ( ( attr bitand FILE_ATTRIBUTE_COMPRESSED    ) != 0 );
	m_attr.encrypted  = ( ( attr bitand FILE_ATTRIBUTE_ENCRYPTED     ) != 0 );
	m_attr.reparse    = ( ( attr bitand FILE_ATTRIBUTE_REPARSE_POINT ) != 0 );
	m_attr.invalid    = false;
	}

INT CItemBranch::GetSortAttributes( ) const {
	DWORD ret = 0;

	// We want to enforce the order RHSACE with R being the highest priority attribute and E being the lowest priority attribute.
	ret += ( m_attr.readonly   ) ? 1000000 : 0; // R
	ret += ( m_attr.hidden     ) ? 100000 : 0; // H
	ret += ( m_attr.system     ) ? 10000 : 0; // S
	//ret += ( m_attr.archive    ) ? 1000 : 0; // A
	ret += ( m_attr.compressed ) ? 100 : 0; // C
	ret += ( m_attr.encrypted  ) ? 10 : 0; // E

	return static_cast< INT >( ( m_attr.invalid ) ? 0 : ret );
	}

DOUBLE CItemBranch::GetFraction( ) const {
	const auto myParent = GetParentItem( );
	if ( myParent == NULL ) {
		return static_cast<DOUBLE>( 1.0 );//root item? must be whole!
		}
	const auto parentSize = myParent->size_recurse( );
	if ( parentSize == 0 ) {//root item?
		return static_cast<DOUBLE>( 1.0 );
		}
	const auto my_size = size_recurse( );
	ASSERT( my_size != UINT64_ERROR );
	ASSERT( my_size <= parentSize );
	return static_cast<DOUBLE>( my_size ) / static_cast<DOUBLE>( parentSize );
	}

std::wstring CItemBranch::GetPath( ) const {
	std::wstring pathBuf;
	pathBuf.reserve( MAX_PATH );
	UpwardGetPathWithoutBackslash( pathBuf );
	ASSERT( wcslen( m_name.get( ) ) == m_name_length );
	ASSERT( wcslen( m_name.get( ) ) < 33000 );
	ASSERT( pathBuf.length( ) < 33000 );
	return pathBuf;
	}

void CItemBranch::UpwardGetPathWithoutBackslash( std::wstring& pathBuf ) const {
	auto myParent = GetParentItem( );
	if ( myParent != NULL ) {
		myParent->UpwardGetPathWithoutBackslash( pathBuf );
		}
	ASSERT( wcslen( m_name.get( ) ) == m_name_length );
	ASSERT( wcslen( m_name.get( ) ) < 33000 );
	if ( m_children == nullptr ) {
		//ASSERT( m_parent != NULL );
		if ( m_parent != NULL ) {
			if ( m_parent->m_parent != NULL ) {
				pathBuf += L'\\';
				ASSERT( wcslen( m_name.get( ) ) == m_name_length );
				pathBuf += m_name.get( );
				}
			else {
				pathBuf += m_name.get( );
				}
			return;
			}
		ASSERT( pathBuf.empty( ) );
		ASSERT( wcslen( m_name.get( ) ) == m_name_length );
		pathBuf = m_name.get( );
		return;
		//ASSERT( false );
		//return;
		}
	if ( !pathBuf.empty( ) ) {
		if ( pathBuf.back( ) != L'\\' ) {//if pathBuf is empty, it's because we don't have a parent ( we're the root ), so we already have a "\\"
			pathBuf += L'\\';
			}
		}
	pathBuf += m_name.get( );
	return;

	}

CRect CItemBranch::TmiGetRectangle( ) const {
	return BuildCRect( m_rect );
	}

_Success_( return < SIZE_T_MAX )
size_t CItemBranch::findItemInChildren( const CItemBranch* const theItem ) const {
	const auto childrenSize = m_childCount;
	for ( size_t i = 0; i < childrenSize; ++i ) {
		if ( ( ( m_children.get( ) + i ) ) == theItem ) {
			return i;
			}
		}
	return SIZE_T_MAX;
	}


void CItemBranch::refresh_sizeCache( ) const {
	//if ( m_type == IT_FILE ) {
	if ( m_children == nullptr ) {
		return;
		}
	if ( m_vi != nullptr ) {
		if ( m_vi->sizeCache != UINT64_ERROR ) {
			m_vi->sizeCache = UINT64_ERROR;
			m_vi->sizeCache = size_recurse( );
			}
		}
	}



_Ret_range_( 0, UINT64_MAX )
std::uint64_t CItemBranch::size_recurse( ) const {
	//if ( m_type == IT_FILE ) {
	if ( !m_children ) {
		ASSERT( m_childCount == 0 );
		return m_size;
		}
	if ( m_vi != nullptr ) {
		if ( m_vi->sizeCache != UINT64_ERROR ) {
			return m_vi->sizeCache;
			}
		}

	std::uint64_t total = m_size;

	const auto childCount = m_childCount;
	const auto child_array = m_children.get( );
	const rsize_t stack_alloc_threshold = 128;
	if ( childCount < stack_alloc_threshold ) {
		std::uint64_t child_totals[ stack_alloc_threshold ];
		for ( size_t i = 0; i < childCount; ++i ) {
			child_totals[ i ] = ( child_array + i )->size_recurse( );
			}
		//loop vectorized!
		for ( size_t i = 0; i < childCount; ++i ) {
			total += child_totals[ i ];
			}
		}
	else {
		//Not vectorized: 1200, loop contains data dependencies
		for ( size_t i = 0; i < childCount; ++i ) {
			total += ( child_array + i )->size_recurse( );
			}
		}
	if ( m_vi != nullptr ) {
		if ( m_vi->sizeCache == UINT64_ERROR ) {
			m_vi->sizeCache = total;
			//if ( total != 0 ) {
			//	ASSERT( total < ( UINT64_ERROR / 4 ) );
			//	m_vi->sizeCache = total;
			//	}
			}
		}
	ASSERT( total < ( UINT64_ERROR / 4 ) );
	return total;
	}


//4,294,967,295  (4294967295 ) is the maximum number of files in an NTFS filesystem according to http://technet.microsoft.com/en-us/library/cc781134(v=ws.10).aspx
_Ret_range_( 0, 4294967295 )
std::uint32_t CItemBranch::files_recurse( ) const {
	if ( m_children == nullptr ) {
		return 1;
		}
	if ( m_vi != nullptr ) {
		if ( m_vi->files_cache != UINT32_ERROR ) {
			return m_vi->files_cache;
			}
		}
	std::uint32_t total = 0;
	const auto childCount = m_childCount;
	const auto my_m_children = m_children.get( );
	const rsize_t stack_alloc_threshold = 128;
	if ( childCount < stack_alloc_threshold ) {
		std::uint32_t child_totals[ stack_alloc_threshold ];
		for ( size_t i = 0; i < childCount; ++i ) {
			child_totals[ i ] = ( my_m_children + i )->files_recurse( );
			}
		//loop vectorized!
		for ( size_t i = 0; i < childCount; ++i ) {
			total += child_totals[ i ];
			}
		}
	else {
		//Not vectorized: 1304, loop includes assignments of different sizes
		for ( size_t i = 0; i < childCount; ++i ) {
			total += ( my_m_children + i )->files_recurse( );
			}
		}
	total += 1;
	if ( m_vi != nullptr ) {
		if ( m_vi->files_cache == UINT32_ERROR ) {
			m_vi->files_cache = total;
			}
		}
	return total;
	}




FILETIME CItemBranch::FILETIME_recurse( ) const {
	if ( m_children == nullptr ) {
		return m_lastChange;
		}
	if ( m_vi != nullptr ) {
		if ( ( m_vi->filetime_cache.dwHighDateTime != DWORD_ERROR ) && ( m_vi->filetime_cache.dwLowDateTime != DWORD_ERROR ) ) {
			return m_vi->filetime_cache;
			}
		}
	//auto ft = zeroInitFILETIME( );
	auto ft = zero_init_struct<FILETIME>( );
	if ( Compare_FILETIME_cast( ft, m_lastChange ) ) {
		ft = m_lastChange;
		}
	
	const auto childCount = m_childCount;
	const auto my_m_children = m_children.get( );
	//Not vectorized: 1304, loop includes assignments of different sizes
	for ( size_t i = 0; i < childCount; ++i ) {
		const auto ft_child = ( my_m_children + i )->FILETIME_recurse( );
		if ( Compare_FILETIME_cast( ft, ft_child ) ) {
			ft = ft_child;
			}
		}
	if ( m_vi != nullptr ) {
		if ( ( m_vi->filetime_cache.dwHighDateTime == DWORD_ERROR ) && ( m_vi->filetime_cache.dwLowDateTime == DWORD_ERROR ) ) {
			ASSERT( ( ft.dwHighDateTime != DWORD_ERROR ) && ( ft.dwLowDateTime != DWORD_ERROR ) );
			m_vi->filetime_cache = ft;
			}
		}
	return ft;
	}



//Sometimes I just need to COMPARE the extension with a string. So, instead of copying/screwing with string internals, I'll just return a pointer to the substring.
//_Pre_satisfies_( this->m_type == IT_FILE )
_Pre_satisfies_( this->m_children._Myptr == nullptr ) 
PCWSTR CItemBranch::CStyle_GetExtensionStrPtr( ) const {
	ASSERT( m_name_length < ( MAX_PATH + 1 ) );

	PCWSTR resultPtrStr = PathFindExtensionW( m_name.get( ) );
	ASSERT( resultPtrStr != '\0' );
	return resultPtrStr;
	}

//_Pre_satisfies_( this->m_type == IT_FILE )
_Pre_satisfies_( this->m_children._Myptr == nullptr )
_Success_( SUCCEEDED( return ) )
HRESULT CItemBranch::CStyle_GetExtension( WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_extension, const rsize_t strSize, _Out_ rsize_t& chars_written ) const {
	psz_extension[ 0 ] = 0;

	PWSTR resultPtrStr = PathFindExtensionW( m_name.get( ) );
	ASSERT( resultPtrStr != '\0' );
	if ( resultPtrStr != '\0' ) {
		size_t extLen = 0;
		const HRESULT res_1 = StringCchLengthW( resultPtrStr, MAX_PATH, &extLen );
		if ( FAILED( res_1 ) ) {
			psz_extension[ 0 ] = 0;
			chars_written = 0;
			return res_1;
			}
		if ( extLen > ( strSize ) ) {
			psz_extension[ 0 ] = 0;
			chars_written = 0;
			return STRSAFE_E_INSUFFICIENT_BUFFER;
			}
		const HRESULT res_2 = StringCchCopyW( psz_extension, strSize, resultPtrStr );
		if ( SUCCEEDED( res_2 ) ){
			chars_written = extLen;
			}
#ifdef DEBUG
		if ( SUCCEEDED( res_2 ) ) {
			ASSERT( GetExtension( ).compare( psz_extension ) == 0 );
			}
#endif
		return res_2;
		}

	psz_extension[ 0 ] = 0;
	chars_written = 0;
	return STRSAFE_E_INVALID_PARAMETER;//some generic error
	}

//_Pre_satisfies_( this->m_type == IT_FILE )
_Pre_satisfies_( this->m_children._Myptr == nullptr ) 
const std::wstring CItemBranch::GetExtension( ) const {
	//if ( m_type == IT_FILE ) {
	if ( m_children == nullptr ) {
		PWSTR resultPtrStr = PathFindExtensionW( m_name.get( ) );
		ASSERT( resultPtrStr != 0 );
		if ( resultPtrStr != '\0' ) {
			return resultPtrStr;
			}
		const PCWSTR i = wcsrchr( m_name.get( ), L'.' );

		if ( i == NULL ) {
			return _T( "." );
			}
		return std::wstring( i );
		}
	return std::wstring( L"" );
	}


void CItemBranch::TmiSetRectangle( _In_ const CRect& rc ) const {
	ASSERT( ( rc.right + 1 ) >= rc.left );
	ASSERT( rc.bottom >= rc.top );
	m_rect.left   = static_cast<short>( rc.left );
	m_rect.top    = static_cast<short>( rc.top );
	m_rect.right  = static_cast<short>( rc.right );
	m_rect.bottom = static_cast<short>( rc.bottom );
	}


_Ret_range_( 0, 33000 ) DOUBLE CItemBranch::averageNameLength( ) const {
	const auto myLength = static_cast<DOUBLE>( m_name_length );
	DOUBLE childrenTotal = 0;
	
	//if ( m_type != IT_FILE ) {
	if ( m_children != nullptr ) {
		const auto childCount = m_childCount;
		const auto my_m_children = m_children.get( );
		const rsize_t stack_alloc_threshold = 128;
		if ( childCount < stack_alloc_threshold ) {
			DOUBLE children_totals[ stack_alloc_threshold ] = { 0 };
			for ( size_t i = 0; i < childCount; ++i ) {
				children_totals[ i ] = ( my_m_children + i )->averageNameLength( );
				}
			for ( size_t i = 0; i < childCount; ++i ) {
				childrenTotal += children_totals[ i ];
				}
			}
		else {
			//Not vectorized: 1200, loop contains data dependencies
			for ( size_t i = 0; i < childCount; ++i ) {
				childrenTotal += ( my_m_children + i )->averageNameLength( );
				}
			}
		return ( childrenTotal + myLength ) / static_cast<DOUBLE>( childCount + 1u );
		}
	ASSERT( m_childCount == 0 );
	return myLength;
	}

//_Pre_satisfies_( this->m_type == IT_FILE )
_Pre_satisfies_( this->m_children._Myptr == nullptr ) 
void CItemBranch::stdRecurseCollectExtensionData_FILE( _Inout_ std::unordered_map<std::wstring, SExtensionRecord>& extensionMap ) const {
	const size_t extensionPsz_size = 48;
	wchar_t extensionPsz[ extensionPsz_size ] = { 0 };
	rsize_t chars_written = 0;
	HRESULT res = CStyle_GetExtension( extensionPsz, extensionPsz_size, chars_written );
	if ( SUCCEEDED( res ) ) {
		auto& value = extensionMap[ extensionPsz ];
		if ( value.files == 0 ) {
			value.ext = extensionPsz;
			value.ext.shrink_to_fit( );
			}
		++( value.files );
		value.bytes += m_size;
		}
	else {
		//use an underscore to avoid name conflict with _DEBUG build
		auto ext_ = GetExtension( );
		ext_.shrink_to_fit( );
		TRACE( _T( "Extension len: %i ( bigger than buffer! )\r\n\toffending extension:\r\n %s\r\n" ), ext_.length( ), ext_.c_str( ) );
		auto& value = extensionMap[ ext_ ];
		if ( value.files == 0 ) {
			value.ext = ext_;
			value.ext.shrink_to_fit( );
			}
		++( value.files );
		extensionMap[ ext_ ].bytes += m_size;
		}
	}


void CItemBranch::stdRecurseCollectExtensionData( _Inout_ std::unordered_map<std::wstring, SExtensionRecord>& extensionMap ) const {
	//if ( m_type == IT_FILE ) {
	if ( m_children == nullptr ) {
		stdRecurseCollectExtensionData_FILE( extensionMap );
		}
	else {
		const auto childCount = m_childCount;
		const auto local_m_children = m_children.get( );
		//Not vectorized: 1200, loop contains data dependencies
		for ( size_t i = 0; i < childCount; ++i ) {
			( local_m_children + i )->stdRecurseCollectExtensionData( extensionMap );
			}

		}
	}

INT __cdecl CItem_compareBySize( _In_ _Points_to_data_ const void* const p1, _In_ _Points_to_data_ const void* const p2 ) {
	const auto size1 = ( *( reinterpret_cast< const CItemBranch * const* const >( p1 ) ) )->size_recurse( );
	const auto size2 = ( *( reinterpret_cast< const CItemBranch * const* const >( p2 ) ) )->size_recurse( );
	return signum( static_cast<std::int64_t>( size2 ) - static_cast<std::int64_t>( size1 ) ); // biggest first// TODO: Use 2nd sort column (as set in our TreeListView?)
	}


// $Log$
// Revision 1.27  2005/04/10 16:49:30  assarbad
// - Some smaller fixes including moving the resource string version into the rc2 files
//
// Revision 1.26  2004/12/31 16:01:42  bseifert
// Bugfixes. See changelog 2004-12-31.
//
// Revision 1.25  2004/12/12 08:34:59  bseifert
// Aboutbox: added Authors-Tab. Removed license.txt from resource dlls (saves 16 KB per dll).
//
// Revision 1.24  2004/11/29 07:07:47  bseifert
// Introduced SRECT. Saves 8 Bytes in sizeof(CItem). Formatting changes.
//
// Revision 1.23  2004/11/28 19:20:46  assarbad
// - Fixing strange behavior of logical operators by rearranging code in
//   CItem::SetAttributes() and CItem::GetAttributes()
//
// Revision 1.22  2004/11/28 15:38:42  assarbad
// - Possible sorting implementation (using bit-order in m_attributes)
//
// Revision 1.21  2004/11/28 14:40:06  assarbad
// - Extended CFileFindWDS to replace a global function
// - Now packing/unpacking the file attributes. This even spares a call to find encrypted/compressed files.
//
// Revision 1.20  2004/11/25 23:07:23  assarbad
// - Derived CFileFindWDS from CFileFind to correct a problem of the ANSI version
//
// Revision 1.19  2004/11/25 21:13:38  assarbad
// - Implemented "attributes" column in the treelist
// - Adopted width in German dialog
// - Provided German, Russian and English version of IDS_TREECOL_ATTRIBUTES
//
// Revision 1.18  2004/11/25 11:58:52  assarbad
// - Minor fixes (odd behavior of coloring in ANSI version, caching of the GetCompressedFileSize API)
//   for details see the changelog.txt
//
// Revision 1.17  2004/11/12 22:14:16  bseifert
// Eliminated CLR_NONE. Minor corrections.
//
// Revision 1.16  2004/11/12 00:47:42  assarbad
// - Fixed the code for coloring of compressed/encrypted items. Now the coloring spans the full row!
//
// Revision 1.15  2004/11/10 01:03:00  assarbad
// - Style cleaning of the alternative coloring code for compressed/encrypted items
//
// Revision 1.14  2004/11/08 00:46:26  assarbad
// - Added feature to distinguish compressed and encrypted files/folders by color as in the Windows 2000/XP explorer.
//   Same rules apply. (Green = encrypted / Blue = compressed)
//
// Revision 1.13  2004/11/07 20:14:30  assarbad
// - Added wrapper for GetCompressedFileSize() so that by default the compressed file size will be shown.
//
// Revision 1.12  2004/11/05 16:53:07  assarbad
// Added Date and History tag where appropriate.
//
