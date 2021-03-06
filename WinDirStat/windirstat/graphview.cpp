// graphview.cpp: Implementation of CGraphView
//
// see `file_header_text.txt` for licensing & contact info. If you can't find that file, then assume you're NOT allowed to do whatever you wanted to do.

#pragma once

#include "stdafx.h"

#ifndef WDS_GRAPHVIEW_CPP
#define WDS_GRAPHVIEW_CPP

WDS_FILE_INCLUDE_MESSAGE

//TODO: too many headers?
#include "graphview.h"
#include "macros_that_scare_small_children.h"
#include "ScopeGuard.h"
#include "mainframe.h"
#include "windirstat.h"
#include "dirstatview.h"
#include "options.h"
#include "mainframe.h"
#include "dirstatdoc.h"

namespace {

	//TweakSizeOfRectangleForHightlight is called once, unconditionally.
	inline void TweakSizeOfRectangleForHightlight( _Inout_ RECT& rc, _Inout_ RECT& rcClient, _In_ const bool grid ) {
		if ( grid ) {
			rc.right++;
			rc.bottom++;
			}
		if ( rcClient.left < rc.left ) {
			rc.left--;
			}
		if ( rcClient.top < rc.top ) {
			rc.top--;
			}
		if ( rc.right < rcClient.right ) {
			rc.right++;
			}
		if ( rc.bottom < rcClient.bottom ) {
			rc.bottom++;
			}
		}

	}

	/*
BEGIN_MESSAGE_MAP( CGraphView, CView )
	ON_WM_SIZE( )
	ON_WM_LBUTTONDOWN( )
	ON_WM_SETFOCUS( )
	ON_WM_CONTEXTMENU( )
	ON_WM_MOUSEMOVE( )
	ON_WM_DESTROY( )
	ON_WM_TIMER( )
	//ON_COMMAND(ID_POPUP_CANCEL, OnPopupCancel)
END_MESSAGE_MAP( )
	*/

CGraphView::CGraphView( ) : m_recalculationSuspended( false ), m_showTreemap( true ), m_timer( 0 ), m_frameptr( GetMainFrame( ) ), m_appptr( GetApp( ) ) {
	m_size.cx = 0;
	m_size.cy = 0;
	m_dimmedSize.cx = 0;
	m_dimmedSize.cy = 0;
	m_showTreemap = CPersistence::GetShowTreemap( );
	}


AFX_COMDAT const CRuntimeClass CGraphView::classCGraphView = { "CGraphView", sizeof( class CGraphView ), 0xFFFF, &( CGraphView::CreateObject ), const_cast<CRuntimeClass*>( &CView::classCView ), NULL, NULL };

const AFX_MSGMAP* PASCAL CGraphView::GetThisMessageMap( ) {
	typedef CGraphView ThisClass;
	typedef CView TheBaseClass;
	static const AFX_MSGMAP_ENTRY _messageEntries[ ] = {
		{ 
			WM_SIZE, 0, 0, 0,
			AfxSig_vwii,
			(AFX_PMSG)(AFX_PMSGW) (static_cast<void (AFX_MSG_CALL CWnd::*)(UINT, int, int)>( &ThisClass::OnSize))
		},
		{ 
			WM_LBUTTONDOWN, 0, 0, 0,
			AfxSig_vwp,
			(AFX_PMSG)(AFX_PMSGW) (static_cast<void (AFX_MSG_CALL CWnd::*)(UINT, CPoint)>( &ThisClass::OnLButtonDown))
		},
		{ 
			WM_SETFOCUS, 0, 0, 0,
			AfxSig_vW,
			(AFX_PMSG)(AFX_PMSGW)(static_cast<void (AFX_MSG_CALL CWnd::*)(CWnd*)>( &ThisClass::OnSetFocus))
		},
		{
			WM_CONTEXTMENU, 0, 0, 0,
			AfxSig_vWp,
			(AFX_PMSG)(AFX_PMSGW)(static_cast<void (AFX_MSG_CALL CWnd::*)(CWnd*, CPoint)>( &ThisClass::OnContextMenu))
		},
		{
			WM_MOUSEMOVE, 0, 0, 0,
			AfxSig_vwp,
			(AFX_PMSG)(AFX_PMSGW)(static_cast<void (AFX_MSG_CALL CWnd::*)(UINT, CPoint)>( &ThisClass::OnMouseMove))
		},
		{
			WM_DESTROY, 0, 0, 0,
			AfxSig_vv,
			(AFX_PMSG)(AFX_PMSGW)(static_cast<void (AFX_MSG_CALL CWnd::*)(void)>( &ThisClass::OnDestroy))
		},
		{
			WM_TIMER, 0, 0, 0,
			AfxSig_v_up_v,
			(AFX_PMSG)(AFX_PMSGW)(static_cast<void (AFX_MSG_CALL CWnd::*)(UINT_PTR)>( &ThisClass::OnTimer))
		},
		{
			0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0
		}
	};
	static const AFX_MSGMAP messageMap = { &TheBaseClass::GetThisMessageMap, &_messageEntries[ 0 ] };
	return &messageMap;
}

void CGraphView::RecurseHighlightExtension( _In_ CDC& pdc, _In_ const CTreeListItem& item, _In_ const std::wstring& ext ) const {
	const auto rc = item.m_rect;
	if ( ( rc.right - rc.left ) <= 0 || ( rc.bottom - rc.top ) <= 0 ) {
		return;
		}

	//if ( item.m_type == IT_FILE ) {
	if ( item.m_child_info.m_child_info_ptr == nullptr ) {
		const auto extensionStrPtr = item.CStyle_GetExtensionStrPtr( );
		const auto scmp = wcscmp( extensionStrPtr, ext.c_str( ) );
		if ( scmp == 0 ) {
			auto rcc = item.TmiGetRectangle( );
			return RenderHighlightRectangle( pdc, rcc );
			}
		return;
		}

	RecurseHighlightChildren( pdc, item, ext );
	}

void CGraphView::DrawSelection( _In_ CDC& pdc ) const {
	//auto Document = static_cast< CDirstatDoc* >( m_pDocument );
	const auto Document = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	ASSERT( Document != NULL );
	if ( Document == NULL ) {
		return;
		}
	const auto item = Document->m_selectedItem;
	if ( item == NULL ) {//no selection to draw.
		return;
		}
	RECT rcClient;

	ASSERT( ::IsWindow( m_hWnd ) );
	//::GetClientRect(m_hWnd, lpRect);
	VERIFY( ::GetClientRect( m_hWnd, &rcClient ) );
	//GetClientRect( &rcClient );


	RECT rc = item->TmiGetRectangle( );

	TweakSizeOfRectangleForHightlight( rc, rcClient, m_treemap.m_options.grid );

	CSelectStockObject sobrush( pdc, NULL_BRUSH );
	const auto Options = GetOptions( );
	CPen pen( PS_SOLID, 1, Options->m_treemapHighlightColor );
	CSelectObject sopen( pdc, pen );

	RenderHighlightRectangle( pdc, rc );
	}


void CGraphView::DoDraw( _In_ CDC& pDC, _In_ CDC& offscreen_buffer, _In_ RECT& rc ) {
	WTL::CWaitCursor wc;

	VERIFY( m_bitmap.CreateCompatibleBitmap( &pDC, m_size.cx, m_size.cy ) );
	auto guard = WDS_SCOPEGUARD_INSTANCE( [&] { CGraphView::cause_OnIdle_to_be_called_once( ); } );

	CSelectObject sobmp( offscreen_buffer, m_bitmap );
	const auto Document = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	ASSERT( Document != NULL );
	if ( Document == NULL ) {
		//cause_OnIdle_to_be_called_once( );
		return;
		}
	const auto Options = GetOptions( );
	const auto rootItem = Document->m_rootItem.get( );
	ASSERT( rootItem != NULL );
	if ( rootItem == NULL ) {
		//cause_OnIdle_to_be_called_once( );
		return;
		}
	m_treemap.DrawTreemap( offscreen_buffer, rc, rootItem, Options->m_treemapOptions );
#ifdef _DEBUG
	m_treemap.RecurseCheckTree( rootItem );
#endif
	//cause_OnIdle_to_be_called_once( );
	}

//only called from one place
_Pre_satisfies_( item.m_child_info.m_child_info_ptr != NULL )
inline void CGraphView::RecurseHighlightChildren( _In_ CDC& pdc, _In_ const CTreeListItem& item, _In_ const std::wstring& ext ) const {
	ASSERT( item.m_child_info.m_child_info_ptr != nullptr );
	//ASSERT( item.m_childCount == item.m_child_info->m_childCount );
	const auto childCount = item.m_child_info.m_child_info_ptr->m_childCount;
	const auto item_m_children = item.m_child_info.m_child_info_ptr->m_children.get( );

	//Not vectorized: 1200, loop contains data dependencies
	for ( size_t i = 0; i < childCount; ++i ) {
		RecurseHighlightExtension( pdc, *( item_m_children + i ), ext );
		}
	}

void CGraphView::OnDraw( CDC* pScreen_Device_Context ) {
	ASSERT_VALID( pScreen_Device_Context );
	const auto aDocument = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	ASSERT( aDocument != NULL );
	if ( aDocument == NULL ) {
		return;
		}
	const auto root = aDocument->m_rootItem.get( );
	if ( root == NULL ) {
		return;
		}
	ASSERT( root->m_attr.m_done );

	if ( !( root->m_attr.m_done ) ) {
		displayWindowsMsgBoxWithMessage( L"CGraphView::OnLButtonDown: root item is NOT done! This should never happen!" );
		std::terminate( );
		}
	if ( m_recalculationSuspended || ( !m_showTreemap ) ) {
		// TODO: draw something interesting, e.g. outline of the first level.
		return DrawEmptyView( *pScreen_Device_Context );
		}
	DrawViewNotEmpty( *pScreen_Device_Context );
	}


void CGraphView::OnMouseMove( UINT /*nFlags*/, CPoint point ) {
	const auto Document = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	//Perhaps surprisingly, Document == NULL CAN be a valid condition. We don't have to set the message to anything if there's no document.
	auto guard = WDS_SCOPEGUARD_INSTANCE( [&]{ reset_timer_if_zero( ); } );
	if ( Document == NULL ) {
		return;
		}
	const auto root = Document->m_rootItem.get( );
	if ( root == NULL ) {
		TRACE( _T( "FindItemByPoint CANNOT find a point when given a NULL root! So let's not try.\r\n" ) );
		return;
		}

	ASSERT( root->m_attr.m_done );

	if ( !( root->m_attr.m_done ) ) {
		displayWindowsMsgBoxWithMessage( L"CGraphView::OnMouseMove: root item is NOT done! This should never happen!" );
		std::terminate( );
		}

	if ( !( CGraphView::IsDrawn( ) ) ) {
		return;
		}
	/*
	void CView::OnPaint()
	{
		// standard paint routine
		CPaintDC dc(this);
		OnPrepareDC(&dc);
		OnDraw(&dc);
	}
	*/

	const auto item = static_cast< const CTreeListItem* >( m_treemap.FindItemByPoint( root, point, NULL ) );
	if ( item == NULL ) {
		TRACE( _T( "There's nothing with a path, therefore nothing for which we can set the message text.\r\n" ) );
		return;
		}
	ASSERT( m_frameptr != NULL );
	if ( m_frameptr == NULL ) {
		return;
		}
#ifdef DEBUG
	trace_focused_mouspos( point.x, point.y, item->GetPath( ).c_str( ) );
#endif
	//TRACE( _T( "focused & Mouse on tree map!(x: %ld, y: %ld), %s\r\n" ), point.x, point.y, item->GetPath( ).c_str( ) );
	m_frameptr->SetMessageText( item->GetPath( ).c_str( ) );

	reset_timer_if_zero( );
	}


void CGraphView::OnSetFocus( CWnd* /*pOldWnd*/ ) {
	ASSERT( m_frameptr != NULL );
	if ( m_frameptr == NULL ) {
		return;
		}
	const auto DirstatView = m_frameptr->GetDirstatView( );
	ASSERT( DirstatView != NULL );
	if ( DirstatView == NULL ) {
		return;
		}

	//TODO: BUGBUG: WTF IS THIS??!?
	auto junk = DirstatView->SetFocus( );
	if ( junk != NULL ) {
		junk = { NULL };//Don't use return CWnd* right now.
		}
	else if ( junk == NULL ) {
		TRACE( _T( "I'm told I set focus to NULL. That's weird.\r\n" ) );
		}
	}

void CGraphView::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint ) {
	if ( !( STATIC_DOWNCAST( CDirstatDoc, m_pDocument ) )->IsRootDone( ) ) {
		CGraphView::Inactivate( );
		}

	switch ( lHint ) {
			case UpdateAllViews_ENUM::HINT_NEWROOT:
				CGraphView::EmptyView( );
				return CView::OnUpdate( pSender, lHint, pHint );

			case 0:
			case UpdateAllViews_ENUM::HINT_SELECTIONCHANGED:
			case UpdateAllViews_ENUM::HINT_SHOWNEWSELECTION:
			case UpdateAllViews_ENUM::HINT_SELECTIONSTYLECHANGED:
			case UpdateAllViews_ENUM::HINT_EXTENSIONSELECTIONCHANGED:
				return CView::OnUpdate( pSender, lHint, pHint );


			case UpdateAllViews_ENUM::HINT_REDRAWWINDOW:
				VERIFY( CWnd::RedrawWindow( ) );
				return;

			case UpdateAllViews_ENUM::HINT_TREEMAPSTYLECHANGED:
				CGraphView::Inactivate( );
				return CView::OnUpdate( pSender, lHint, pHint );

			default:
				return;
		}
	}

void CGraphView::OnLButtonDown( UINT nFlags, CPoint point ) {
	//auto Document = static_cast< CDirstatDoc* >( m_pDocument );
	const auto Document = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	auto guard = WDS_SCOPEGUARD_INSTANCE( [=]{ CWnd::OnLButtonDown( nFlags, point ); } );
	if ( Document == NULL ) {
		TRACE( _T( "User clicked on nothing. User CAN click on nothing. That's a sane case.\r\n" ) );
		//return CView::OnLButtonDown( nFlags, point );
		return;
		}
	const auto root = Document->m_rootItem.get( );
	if ( root == NULL ) {
		//return CView::OnLButtonDown( nFlags, point );
		return;
		}

	ASSERT( root->m_attr.m_done );

	if ( !( root->m_attr.m_done ) ) {
		displayWindowsMsgBoxWithMessage( L"CGraphView::OnLButtonDown: root item is NOT done! This should never happen!" );
		std::terminate( );
		}

	if ( !IsDrawn( ) ) {
		//return CView::OnLButtonDown( nFlags, point );
		return;
		}

	const auto item = static_cast< CTreeListItem* >( m_treemap.FindItemByPoint( root, point, Document ) );
	if ( item == NULL ) {
		//return CView::OnLButtonDown( nFlags, point );
		return;
		}
	Document->SetSelection( *item );
	Document->UpdateAllViews( NULL, UpdateAllViews_ENUM::HINT_SHOWNEWSELECTION );
	//CView::OnLButtonDown( nFlags, point );
	return;
	}

void CGraphView::DrawHighlights( _In_ CDC& pdc ) const {
	const auto logicalFocus = m_frameptr->m_logicalFocus;
	if ( logicalFocus == LOGICAL_FOCUS::LF_DIRECTORYLIST ) {
		DrawSelection( pdc );
		}
	if ( logicalFocus == LOGICAL_FOCUS::LF_EXTENSIONLIST ) {
		DrawHighlightExtension( pdc );
		}
	m_appptr->PeriodicalUpdateRamUsage( );
	}


void CGraphView::OnContextMenu( CWnd* /*pWnd*/, CPoint ptscreen ) {
	//auto Document = static_cast< CDirstatDoc* >( m_pDocument );
	const auto Document = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	if ( Document == NULL ) {
		TRACE( _T( "User tried to open a Context Menu, but the Document is NULL. Well, they'll get what they asked for: a (NULL context) menu :)\r\n" ) );//(NULL context) menu == no context menu
		return;
		}
	const auto root = Document->m_rootItem.get( );
	if ( root == NULL ) {
		TRACE( _T( "User tried to open a Context Menu, but there are no items in the Document. Well, they'll get what they asked for: a (NULL context) menu :)\r\n" ) );//(NULL context) menu == no context menu
		return;
		}
	if ( !( root->m_attr.m_done ) ) {
		return;
		}
	CMenu menu;
	VERIFY( menu.LoadMenuW( IDR_POPUPGRAPH ) );
	const auto sub = menu.GetSubMenu( 0 );
	ASSERT( sub != NULL );//How the fuck could we ever get NULL from that???!?
	if ( sub == NULL ) {
		return;
		}
	VERIFY( sub->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, ptscreen.x, ptscreen.y, AfxGetMainWnd( ) ) );
	}

void CGraphView::DrawHighlightExtension( _In_ CDC& pdc ) const {
	WTL::CWaitCursor wc;

	CPen pen( PS_SOLID, 1, GetOptions( )->m_treemapHighlightColor );
	CSelectObject sopen( pdc, pen );
	CSelectStockObject sobrush( pdc, NULL_BRUSH );
	//auto Document = static_cast< CDirstatDoc* >( m_pDocument );;
	const auto Document = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	if ( Document == NULL ) {
		ASSERT( Document != NULL );
		return;
		}
	const auto rItem = Document->m_rootItem.get( );
	if ( rItem != NULL ) {
		RecurseHighlightExtension( pdc, ( *rItem ), Document->m_highlightExtension );
		}

	}


void CGraphView::RenderHighlightRectangle( _In_ CDC& pdc, _In_ RECT rc_ ) const {
	/*
		The documentation of CDC::Rectangle() says that the width and height must be greater than 2. Experiment says that it must be greater than 1. We follow the documentation.
		A pen and the null brush must be selected.
		*/

	auto rc = rc_;

	ASSERT( ( rc.right - rc.left ) >= 0 );
	ASSERT( ( rc.bottom - rc.top ) >= 0 );

	//TODO: BUGBUG: why 7?
	if ( ( ( rc.right - rc.left ) >= 7 ) && ( ( rc.bottom - rc.top ) >= 7 ) ) {

		VERIFY( pdc.Rectangle( &rc ) );		// w = 7

		VERIFY( ::InflateRect( &rc, -( 1 ), -( 1 ) ) );
		//rc.DeflateRect( 1, 1 );


		VERIFY( pdc.Rectangle( &rc ) );		// w = 5



		VERIFY( ::InflateRect( &rc, -( 1 ), -( 1 ) ) );
		//rc.DeflateRect( 1, 1 );


		VERIFY( pdc.Rectangle( &rc ) );		// w = 3
		}
	else {
		const auto Options = GetOptions( );
		return pdc.FillSolidRect( &rc, Options->m_treemapHighlightColor );
		}
	}


void CGraphView::OnTimer( UINT_PTR /*nIDEvent*/ ) {
	WTL::CPoint point;
	VERIFY( ::GetCursorPos( &point ) );
	CWnd::ScreenToClient( &point );

	RECT rc;
	/*
_AFXWIN_INLINE void CWnd::GetClientRect(LPRECT lpRect) const
	{ ASSERT(::IsWindow(m_hWnd)); ::GetClientRect(m_hWnd, lpRect); }
	*/
	ASSERT( ::IsWindow( m_hWnd ) );

	//"If [GetClientRect] succeeds, the return value is nonzero. To get extended error information, call GetLastError."
	VERIFY( ::GetClientRect( m_hWnd, &rc ) );

	if ( !PtInRect( &rc, point ) ) {
		TRACE( _T( "Mouse has left the tree map area!\r\n" ) );
		m_frameptr->SetSelectionMessageText( );
		VERIFY( CWnd::KillTimer( m_timer ) );
		m_timer = 0;
		}
	}


#ifdef DEBUG
//this function exists for the singular purpose of tracing to console, as doing so from a .cpp is cleaner.
void trace_empty_view_graphview( ) {
	TRACE( _T( "Drawing Empty view...\r\n" ) );
	}

//this function exists for the singular purpose of tracing to console, as doing so from a .cpp is cleaner.
void trace_call_onidle( ) { 
	TRACE( _T( "\"[Causing] OnIdle() to be called once\"\r\n" ) );
	}

//this function exists for the singular purpose of tracing to console, as doing so from a .cpp is cleaner.
void trace_mouse_left( ) {
	TRACE( _T( "Mouse has left the tree map area?\r\n" ) );
	}

//this function exists for the singular purpose of tracing to console, as doing so from a .cpp is cleaner.
void trace_focused_mouspos( _In_ const LONG x, _In_ const LONG y, _In_z_ PCWSTR const path ) {
	TRACE( _T( "focused & Mouse on tree map!(x: %ld, y: %ld), %s\r\n" ), x, y, path );
	}
#endif



#else

#endif