///////////////////////////////////////////////////////////////////////////////
// Name:        src/ribbon/bar.cpp
// Purpose:     Top-level component of the ribbon-bar-style interface
// Author:      Peter Cawley
// Modified by:
// Created:     2009-05-23
// RCS-ID:      $Id$
// Copyright:   (C) Peter Cawley
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/ribbon/bar.h"

#if wxUSE_RIBBON

#include "wx/ribbon/art.h"
#include "wx/dcbuffer.h"

#ifndef WX_PRECOMP
#endif

#ifdef __WXMSW__
#include "wx/msw/private.h"
#endif

#include "wx/arrimpl.cpp"

WX_DEFINE_USER_EXPORTED_OBJARRAY(wxRibbonPageTabInfoArray);

wxDEFINE_EVENT(wxEVT_COMMAND_RIBBONBAR_PAGE_CHANGED, wxRibbonBarEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_RIBBONBAR_PAGE_CHANGING, wxRibbonBarEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_RIBBONBAR_TAB_MIDDLE_DOWN, wxRibbonBarEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_RIBBONBAR_TAB_MIDDLE_UP, wxRibbonBarEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_RIBBONBAR_TAB_RIGHT_DOWN, wxRibbonBarEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_RIBBONBAR_TAB_RIGHT_UP, wxRibbonBarEvent);

IMPLEMENT_CLASS(wxRibbonBar, wxControl)
IMPLEMENT_DYNAMIC_CLASS(wxRibbonBarEvent, wxNotifyEvent)

BEGIN_EVENT_TABLE(wxRibbonBar, wxControl)
  EVT_ERASE_BACKGROUND(wxRibbonBar::OnEraseBackground)
  EVT_LEAVE_WINDOW(wxRibbonBar::OnMouseLeave)
  EVT_LEFT_DOWN(wxRibbonBar::OnMouseLeftDown)
  EVT_MIDDLE_DOWN(wxRibbonBar::OnMouseMiddleDown)
  EVT_MIDDLE_UP(wxRibbonBar::OnMouseMiddleUp)
  EVT_MOTION(wxRibbonBar::OnMouseMove)
  EVT_PAINT(wxRibbonBar::OnPaint)
  EVT_RIGHT_DOWN(wxRibbonBar::OnMouseRightDown)
  EVT_RIGHT_UP(wxRibbonBar::OnMouseRightUp)
  EVT_SIZE(wxRibbonBar::OnSize)
END_EVENT_TABLE()

void wxRibbonBar::AddPage(wxRibbonPage *page)
{
	wxRibbonPageTabInfo info;

	info.page = page;
	info.active = false;
	info.hovered = false;
	// info.rect not set (intentional)

	wxMemoryDC dcTemp;
	m_art->GetBarTabWidth(dcTemp, this,
						  m_flags & wxRIBBON_BAR_SHOW_PAGE_LABELS ? page->GetLabel() : wxEmptyString,
						  m_flags & wxRIBBON_BAR_SHOW_PAGE_ICONS ? page->GetIcon() : wxNullBitmap,
						  &info.ideal_width,
						  &info.small_begin_need_separator_width,
						  &info.small_must_have_separator_width,
						  &info.minimum_width);

	if(m_pages.IsEmpty())
	{
		m_tabs_total_width_ideal = info.ideal_width;
		m_tabs_total_width_minimum = info.minimum_width;
	}
	else
	{
		int sep = m_art->GetMetric(wxRIBBON_ART_TAB_SEPARATION_SIZE);
		m_tabs_total_width_ideal += sep + info.ideal_width;
		m_tabs_total_width_minimum += sep + info.minimum_width;
	}
	m_pages.Add(info);

	page->Hide(); // Most likely case is that this new page is not the active tab

	m_tab_height = m_art->GetTabCtrlHeight(dcTemp, this, m_pages);
	RecalculateTabSizes();

	if(m_pages.GetCount() == 1)
	{
		SetActivePage((size_t)0);
	}

	Refresh();
}

void wxRibbonBar::OnMouseMove(wxMouseEvent& evt)
{
	int x = evt.GetX();
	int y = evt.GetY();
	int hovered_page = -1;
	if(y < m_tab_height)
	{
		// It is quite likely that the mouse moved a small amount and is still over the same tab
		if(m_current_hovered_page != -1 && m_pages.Item((size_t)m_current_hovered_page).rect.Contains(x, y))
		{
			hovered_page = m_current_hovered_page;
		}
		else
		{
			HitTestTabs(evt.GetPosition(), &hovered_page);
		}
	}
	if(hovered_page != m_current_hovered_page)
	{
		if(m_current_hovered_page != -1)
		{
			m_pages.Item((int)m_current_hovered_page).hovered = false;
		}
		m_current_hovered_page = hovered_page;
		if(m_current_hovered_page != -1)
		{
			m_pages.Item((int)m_current_hovered_page).hovered = true;
		}
		Refresh();
	}
}

void wxRibbonBar::OnMouseLeave(wxMouseEvent& WXUNUSED(evt))
{
	// The ribbon bar is (usually) at the top of a window, and at least on MSW, the mouse
	// can leave the window quickly and leave a tab in the hovered state.
	if(m_current_hovered_page != -1)
	{
		m_pages.Item((int)m_current_hovered_page).hovered = false;
		m_current_hovered_page = -1;
		Refresh();
	}
}

bool wxRibbonBar::SetActivePage(size_t page)
{
	if(m_current_page == (int)page)
	{
		return true;
	}

	if(page >= m_pages.GetCount())
	{
		return false;
	}

	if(m_current_page != -1)
	{
		m_pages.Item((size_t)m_current_page).active = false;
		m_pages.Item((size_t)m_current_page).page->Hide();
	}
	m_current_page = (int)page;
	m_pages.Item(page).active = true;
	m_pages.Item(page).page->Show();
	Refresh();

	return true;
}

bool wxRibbonBar::SetActivePage(wxRibbonPage* page)
{
	size_t numpages = m_pages.GetCount();
	for(size_t i = 0; i < numpages; ++i)
	{
		if(m_pages.Item(i).page == page)
		{
			return SetActivePage(i);
		}
	}
	return false;
}

int wxRibbonBar::GetActivePage() const
{
	return m_current_page;
}

void wxRibbonBar::SetTabCtrlMargins(int left, int right)
{
	m_tab_margin_left = left;
	m_tab_margin_right = right;

	RecalculateTabSizes();
}

static int OrderPageTabInfoBySmallWidthAsc(wxRibbonPageTabInfo **first, wxRibbonPageTabInfo **second)
{
	return (**first).small_must_have_separator_width - (**second).small_must_have_separator_width;
}

void wxRibbonBar::RecalculateTabSizes()
{
	size_t numtabs = m_pages.GetCount();

	if(numtabs == 0)
		return;

	int width = GetSize().GetWidth() - m_tab_margin_left - m_tab_margin_right;
	int tabsep = m_art->GetMetric(wxRIBBON_ART_TAB_SEPARATION_SIZE);
	int x = m_tab_margin_left;
	const int y = 0;

	if(width >= m_tabs_total_width_ideal)
	{
		// Simple case: everything at ideal width
		for(size_t i = 0; i < numtabs; ++i)
		{
			wxRibbonPageTabInfo& info = m_pages.Item(i);
			info.rect.x = x;
			info.rect.y = y;
			info.rect.width = info.ideal_width;
			info.rect.height = m_tab_height;
			x += info.rect.width + tabsep;
		}
		m_tab_scroll_buttons_shown = false;
		m_tab_scroll_left_button_width = 0;
		m_tab_scroll_right_button_width = 0;
	}
	else if(width < m_tabs_total_width_minimum)
	{
		// Simple case: everything minimum with scrollbar
		for(size_t i = 0; i < numtabs; ++i)
		{
			wxRibbonPageTabInfo& info = m_pages.Item(i);
			info.rect.x = x;
			info.rect.y = y;
			info.rect.width = info.minimum_width;
			info.rect.height = m_tab_height;
			x += info.rect.width + tabsep;
		}
		m_tab_scroll_buttons_shown = true;
		{
			wxMemoryDC temp_dc;
			m_tab_scroll_left_button_width = m_art->GetScrollButtonMinimumSize(temp_dc, this, wxRIBBON_SCROLL_BTN_LEFT | wxRIBBON_SCROLL_BTN_NORMAL | wxRIBBON_SCROLL_BTN_FOR_TABS).GetWidth();
			m_tab_scroll_right_button_width = m_art->GetScrollButtonMinimumSize(temp_dc, this, wxRIBBON_SCROLL_BTN_RIGHT | wxRIBBON_SCROLL_BTN_NORMAL | wxRIBBON_SCROLL_BTN_FOR_TABS).GetWidth();
		}
		if(m_tab_scroll_amount == 0)
		{
			m_tab_scroll_left_button_width = 0;
		}
		else if(m_tab_scroll_amount + width > m_tabs_total_width_minimum)
		{
			m_tab_scroll_amount = m_tabs_total_width_minimum - width;
			m_tab_scroll_right_button_width = 0;
		}
		else if(m_tab_scroll_amount + width == m_tabs_total_width_minimum)
		{
			m_tab_scroll_right_button_width = 0;
		}
	}
	else
	{
		m_tab_scroll_buttons_shown = false;
		m_tab_scroll_left_button_width = 0;
		m_tab_scroll_right_button_width = 0;
		// Complex case: everything sized such that: minimum <= width < ideal
		/*
		   Strategy:
		     1) Uniformly reduce all tab widths from ideal to small_must_have_separator_width
			 2) Reduce the largest tab by 1 pixel, repeating until all tabs are same width (or at minimum)
			 3) Uniformly reduce all tabs down to their minimum width
		*/
		int smallest_tab_width = INT_MAX;
		int total_small_width = tabsep * (numtabs - 1);
		for(size_t i = 0; i < numtabs; ++i)
		{
			wxRibbonPageTabInfo& info = m_pages.Item(i);
			if(info.small_must_have_separator_width < smallest_tab_width)
			{
				smallest_tab_width = info.small_must_have_separator_width;
			}
			total_small_width += info.small_must_have_separator_width;
		}
		if(width >= total_small_width)
		{
			// Do (1)
			int total_delta = m_tabs_total_width_ideal - total_small_width;
			total_small_width -= tabsep * (numtabs - 1);
			width -= tabsep * (numtabs - 1);
			for(size_t i = 0; i < numtabs; ++i)
			{
				wxRibbonPageTabInfo& info = m_pages.Item(i);
				int delta = info.ideal_width - info.small_must_have_separator_width;
				info.rect.x = x;
				info.rect.y = y;
				info.rect.width = info.small_must_have_separator_width + delta * (width - total_small_width) / total_delta;
				info.rect.height = m_tab_height;

				x += info.rect.width + tabsep;
				total_delta -= delta;
				total_small_width -= info.small_must_have_separator_width;
				width -= info.rect.width;
			}
		}
		else
		{
			total_small_width = tabsep * (numtabs - 1);
			for(size_t i = 0; i < numtabs; ++i)
			{
				wxRibbonPageTabInfo& info = m_pages.Item(i);
				if(info.minimum_width < smallest_tab_width)
				{
					total_small_width += smallest_tab_width;
				}
				else
				{
					total_small_width += info.minimum_width;
				}
			}
			if(width >= total_small_width)
			{
				// Do (2)
				wxRibbonPageTabInfoArray sorted_pages;
				for(size_t i = 0; i < numtabs; ++i)
				{
					// Sneaky obj array trickery to not copy the tab descriptors
					sorted_pages.Add(&m_pages.Item(i));
				}
				sorted_pages.Sort(OrderPageTabInfoBySmallWidthAsc);
				width -= tabsep * (numtabs - 1);
				for(size_t i = 0; i < numtabs; ++i)
				{
					wxRibbonPageTabInfo& info = sorted_pages.Item(i);
					if(info.small_must_have_separator_width * (int)(numtabs - i) <= width)
					{
						info.rect.width = info.small_must_have_separator_width;;
					}
					else
					{
						info.rect.width = width / (numtabs - i);
					}
					width -= info.rect.width;
				}
				for(size_t i = 0; i < numtabs; ++i)
				{
					wxRibbonPageTabInfo& info = m_pages.Item(i);
					info.rect.x = x;
					info.rect.y = y;
					info.rect.height = m_tab_height;
					x += info.rect.width + tabsep;
					sorted_pages.Detach(numtabs - (i + 1));
				}
			}
			else
			{
				// Do (3)
				total_small_width = (smallest_tab_width + tabsep) * numtabs - tabsep;
				int total_delta = total_small_width - m_tabs_total_width_minimum;
				total_small_width = m_tabs_total_width_minimum - tabsep * (numtabs - 1);
				width -= tabsep * (numtabs - 1);
				for(size_t i = 0; i < numtabs; ++i)
				{
					wxRibbonPageTabInfo& info = m_pages.Item(i);
					int delta = smallest_tab_width - info.minimum_width;
					info.rect.x = x;
					info.rect.y = y;
					info.rect.width = info.minimum_width + delta * (width - total_small_width) / total_delta;
					info.rect.height = m_tab_height;

					x += info.rect.width + tabsep;
					total_delta -= delta;
					total_small_width -= info.minimum_width;
					width -= info.rect.width;
				}
			}
		}
	}
}

wxRibbonBar::wxRibbonBar()
{
	m_art = NULL;
	m_flags = 0;
	m_tabs_total_width_ideal = 0;
	m_tabs_total_width_minimum = 0;
	m_tab_scroll_amount = 0;
	m_current_page = -1;
	m_current_hovered_page = -1;
	m_tab_scroll_buttons_shown = false;
}

wxRibbonBar::wxRibbonBar(wxWindow* parent,
						 wxWindowID id,
						 const wxPoint& pos,
						 const wxSize& size,
						 long style) : wxControl(parent, id, pos, size, style)
{
	CommonInit(style);
}

wxRibbonBar::~wxRibbonBar()
{
	SetArtProvider(NULL);
}

bool wxRibbonBar::Create(wxWindow* parent,
				wxWindowID id,
				const wxPoint& pos,
				const wxSize& size,
				long style)
{
	if(!wxControl::Create(parent, id, pos, size, style))
		return false;

	CommonInit(style);

	return true;
}

void wxRibbonBar::CommonInit(long style)
{
    SetName(wxT("wxRibbonBar"));

	m_art = NULL;
	m_flags = style;
	m_tabs_total_width_ideal = 0;
	m_tabs_total_width_minimum = 0;
	m_tab_margin_left = 50;
	m_tab_margin_right = 20;
	m_tab_height = 20; // initial guess
	m_tab_scroll_amount = 0;
	m_current_page = -1;
	m_current_hovered_page = -1;
	m_tab_scroll_buttons_shown = false;

	SetArtProvider(new wxRibbonDefaultArtProvider);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

void wxRibbonBar::SetArtProvider(wxRibbonArtProvider* art)
{
	delete m_art;
	m_art = art;

	if(m_art)
	{
		m_art->SetFlags(m_flags);
	}
}

void wxRibbonBar::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
	wxAutoBufferedPaintDC dc(this);

	DoEraseBackground(dc);

	size_t numtabs = m_pages.GetCount();
	double sep_visibility = 0.0;
	bool draw_sep = false;
	wxRect tabs_rect(m_tab_margin_left, 0, GetClientSize().GetWidth() - m_tab_margin_left - m_tab_margin_right, m_tab_height);
	if(m_tab_scroll_buttons_shown)
	{
		tabs_rect.x += m_tab_scroll_left_button_width;
		tabs_rect.width -= m_tab_scroll_left_button_width + m_tab_scroll_right_button_width;
	}
	for(size_t i = 0; i < numtabs; ++i)
	{
		wxRibbonPageTabInfo& info = m_pages.Item(i);

		dc.DestroyClippingRegion();
		dc.SetClippingRegion(tabs_rect);
		dc.SetClippingRegion(info.rect);
		m_art->DrawTab(dc, this, info);

		if(info.rect.width < info.small_begin_need_separator_width)
		{
			draw_sep = true;
			if(info.rect.width < info.small_must_have_separator_width)
			{
				sep_visibility += 1.0;
			}
			else
			{
				sep_visibility += (double)(info.small_begin_need_separator_width - info.rect.width) / (double)(info.small_begin_need_separator_width - info.small_must_have_separator_width);
			}
		}
	}
	if(draw_sep)
	{
		wxRect rect = m_pages.Item(0).rect;
		rect.width = m_art->GetMetric(wxRIBBON_ART_TAB_SEPARATION_SIZE);
		sep_visibility /= (double)numtabs;
		for(size_t i = 0; i < numtabs - 1; ++i)
		{
			wxRibbonPageTabInfo& info = m_pages.Item(i);
			rect.x = info.rect.x + info.rect.width;

			dc.DestroyClippingRegion();
			dc.SetClippingRegion(rect);
			m_art->DrawTabSeparator(dc, this, rect, sep_visibility);
		}
	}
	if(m_tab_scroll_buttons_shown)
	{
		dc.DestroyClippingRegion();
		if(m_tab_scroll_left_button_width != 0)
		{
			wxRect rect(tabs_rect.x - m_tab_scroll_left_button_width, 0, m_tab_scroll_left_button_width, m_tab_height);
			m_art->DrawScrollButton(dc, this, rect, wxRIBBON_SCROLL_BTN_LEFT | wxRIBBON_SCROLL_BTN_NORMAL | wxRIBBON_SCROLL_BTN_FOR_TABS);
		}
		if(m_tab_scroll_right_button_width != 0)
		{
			wxRect rect(tabs_rect.x + tabs_rect.width, 0, m_tab_scroll_right_button_width, m_tab_height);
			m_art->DrawScrollButton(dc, this, rect, wxRIBBON_SCROLL_BTN_RIGHT | wxRIBBON_SCROLL_BTN_NORMAL | wxRIBBON_SCROLL_BTN_FOR_TABS);
		}
	}
}

void wxRibbonBar::OnEraseBackground(wxEraseEvent& WXUNUSED(evt))
{
	// Background painting done in main paint handler to reduce screen flicker
}

void wxRibbonBar::DoEraseBackground(wxDC& dc)
{
	wxRect tabs(GetClientSize());
	tabs.height = m_tab_height;
	m_art->DrawTabCtrlBackground(dc, this, tabs);

	wxRect page(GetClientSize());
	page.height -= tabs.height;
	page.y += tabs.height;
	m_art->DrawPageBackground(dc, this, page);
}

void wxRibbonBar::OnSize(wxSizeEvent& WXUNUSED(evt))
{
	RecalculateTabSizes();
	Refresh();
}

wxRibbonPageTabInfo* wxRibbonBar::HitTestTabs(wxPoint position, int* index)
{
	if(position.y < m_tab_height)
	{
		size_t numtabs = m_pages.GetCount();
		for(size_t i = 0; i < numtabs; ++i)
		{
			wxRibbonPageTabInfo& info = m_pages.Item(i);
			if(info.rect.Contains(position))
			{
				if(index != NULL)
				{
					*index = (int)i;
				}
				return &info;
			}
		}
	}
	if(index != NULL)
	{
		*index = -1;
	}
	return NULL;
}

void wxRibbonBar::OnMouseLeftDown(wxMouseEvent& evt)
{
	wxRibbonPageTabInfo *tab = HitTestTabs(evt.GetPosition());
	if(tab && tab != &m_pages.Item(m_current_page))
	{
		wxRibbonBarEvent query(wxEVT_COMMAND_RIBBONBAR_PAGE_CHANGING, GetId(), tab->page);
		query.SetEventObject(this);
		ProcessWindowEvent(query);
		if(query.IsAllowed())
		{
			SetActivePage(query.GetPage());

			wxRibbonBarEvent notification(wxEVT_COMMAND_RIBBONBAR_PAGE_CHANGED, GetId(), m_pages.Item(m_current_page).page);
			notification.SetEventObject(this);
			ProcessWindowEvent(notification);
		}
	}
}

void wxRibbonBar::OnMouseMiddleDown(wxMouseEvent& evt)
{
	DoMouseButtonCommon(evt, wxEVT_COMMAND_RIBBONBAR_TAB_MIDDLE_DOWN);
}

void wxRibbonBar::OnMouseMiddleUp(wxMouseEvent& evt)
{
	DoMouseButtonCommon(evt, wxEVT_COMMAND_RIBBONBAR_TAB_MIDDLE_UP);
}

void wxRibbonBar::OnMouseRightDown(wxMouseEvent& evt)
{
	DoMouseButtonCommon(evt, wxEVT_COMMAND_RIBBONBAR_TAB_RIGHT_DOWN);
}

void wxRibbonBar::OnMouseRightUp(wxMouseEvent& evt)
{
	DoMouseButtonCommon(evt, wxEVT_COMMAND_RIBBONBAR_TAB_RIGHT_UP);
}

void wxRibbonBar::DoMouseButtonCommon(wxMouseEvent& evt, wxEventType tab_event_type)
{
	wxRibbonPageTabInfo *tab = HitTestTabs(evt.GetPosition());
	if(tab)
	{
		wxRibbonBarEvent notification(tab_event_type, GetId(), tab->page);
		notification.SetEventObject(this);
		ProcessWindowEvent(notification);
	}
}

#endif // wxUSE_RIBBON
