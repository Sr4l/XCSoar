/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Look/InfoBoxLook.hpp"
#include "InputEvents.hpp"
#include "Compatibility/string.h"
#include "Renderer/UnitSymbolRenderer.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Screen/Layout.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Interface.hpp"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/SubCanvas.hpp"
#endif

#include <algorithm>
#include <stdio.h>
#include <assert.h>

using std::max;

#define SELECTORWIDTH Layout::Scale(5)

InfoBoxWindow::InfoBoxWindow(ContainerWindow &_parent,
                             PixelScalar x, PixelScalar y,
                             UPixelScalar width, UPixelScalar height,
                             int border_flags, const InfoBoxSettings &_settings,
                             const InfoBoxLook &_look, WindowStyle style)
  :content(NULL),
   parent(_parent),
   settings(_settings), look(_look),
   border_kind(border_flags),
   focus_timer(*this)
{
  data.Clear();

  style.enable_double_clicks();
  set(parent, x, y, width, height, style);

  id = -1;
}

void
InfoBoxWindow::SetID(const int _id)
{
  id = _id;
}

void
InfoBoxWindow::SetTitle(const TCHAR *_title)
{
  data.SetTitle(_title);
  invalidate(title_rect);
}

void
InfoBoxWindow::PaintTitle(Canvas &canvas)
{
  if (data.title.empty())
    return;

  canvas.set_text_color(look.get_title_color(data.title_color));

  const Font &font = *look.title.font;
  canvas.select(font);

  PixelSize tsize = canvas.text_size(data.title);

  PixelScalar halftextwidth = (title_rect.left + title_rect.right - tsize.cx) / 2;
  PixelScalar x = max(PixelScalar(1),
                      PixelScalar(title_rect.left + halftextwidth));
  PixelScalar y = title_rect.top + 1 + font.GetCapitalHeight() -
    font.GetAscentHeight();

  canvas.TextAutoClipped(x, y, data.title);

  if (settings.border_style == apIbTab && halftextwidth > Layout::Scale(3)) {
    PixelScalar ytop = title_rect.top + font.GetCapitalHeight() / 2;
    PixelScalar ytopedge = ytop + Layout::Scale(2);
    PixelScalar ybottom = title_rect.top + Layout::Scale(6)
      + font.GetCapitalHeight();

    canvas.select(look.border_pen);

    RasterPoint tab[8];
    tab[0].x = tab[1].x = title_rect.left;
    tab[0].y = tab[7].y = ybottom;
    tab[2].x = title_rect.left + Layout::Scale(2);
    tab[2].y = tab[5].y = tab[3].y = tab[4].y = ytop;
    tab[1].y = tab[6].y = ytopedge;
    tab[5].x = title_rect.right - Layout::Scale(2);
    tab[6].x = tab[7].x = title_rect.right;
    tab[3].x = title_rect.left + halftextwidth - Layout::Scale(1);
    tab[4].x = title_rect.right - halftextwidth + Layout::Scale(1);

    canvas.polyline(tab, 4);
    canvas.polyline(tab + 4, 4);
  }
}

void
InfoBoxWindow::PaintValue(Canvas &canvas)
{
  if (data.value.empty())
    return;

  canvas.set_text_color(look.get_value_color(data.value_color));

#ifndef GNAV
  // Do text-based unit rendering on higher resolutions
  if (Layout::FastScale(10) > 18) {
    canvas.select(*look.unit_font);
    PixelSize unit_size = UnitSymbolRenderer::GetSize(canvas, data.value_unit);

    canvas.select(*look.value.font);
    UPixelScalar ascent_height = look.value.font->GetAscentHeight();

    PixelSize tsize = canvas.text_size(data.value);
    if (tsize.cx > value_rect.right - value_rect.left) {
      canvas.select(*look.small_font);
      ascent_height = look.small_font->GetAscentHeight();
      tsize = canvas.text_size(data.value);
    }

    PixelScalar x = max(PixelScalar(0),
                        PixelScalar((value_rect.left + value_rect.right - tsize.cx
                                     - unit_size.cx) / 2));

    PixelScalar y = (value_rect.top + value_rect.bottom - tsize.cy) / 2;

    canvas.TextAutoClipped(x, y, data.value);

    if (unit_size.cx != 0) {
      UPixelScalar unit_height =
          UnitSymbolRenderer::GetAscentHeight(*look.unit_font,
                                              data.value_unit);

      canvas.select(*look.unit_font);
      UnitSymbolRenderer::Draw(canvas,
                               { PixelScalar(x + tsize.cx),
                                 PixelScalar(y + ascent_height - unit_height) },
                               data.value_unit);
    }
    return;
  }
#endif

  canvas.select(*look.value.font);
  UPixelScalar ascent_height = look.value.font->GetAscentHeight();
  UPixelScalar capital_height = look.value.font->GetCapitalHeight();

  PixelSize tsize = canvas.text_size(data.value);
  if (tsize.cx > value_rect.right - value_rect.left) {
    canvas.select(*look.small_font);
    ascent_height = look.small_font->GetAscentHeight();
    capital_height = look.small_font->GetCapitalHeight();
    tsize = canvas.text_size(data.value);
  }

  PixelSize unit_size;
  const UnitSymbol *unit_symbol = GetUnitSymbol(data.value_unit);
  if (unit_symbol != NULL) {
    unit_size = unit_symbol->get_size();
  } else {
    unit_size.cx = 0;
    unit_size.cy = 0;
  }

  PixelScalar x = max(PixelScalar(1),
                      PixelScalar((value_rect.left + value_rect.right - tsize.cx
                                   - Layout::FastScale(unit_size.cx)) / 2));

  PixelScalar y = value_rect.top + 1 - ascent_height +
    (value_rect.bottom - value_rect.top + capital_height) / 2;

  canvas.TextAutoClipped(x, y, data.value);

  if (unit_symbol != NULL) {
#ifndef HAVE_CLIPPING
    /* sort-of clipping */
    if (x + tsize.cx >= (int)canvas.get_width())
      return;
#endif

    RasterPoint origin = unit_symbol->get_origin(look.inverse
                                           ? UnitSymbol::INVERSE
                                           : UnitSymbol::NORMAL);

    canvas.scale_copy(x + tsize.cx,
                      y + ascent_height
                      - Layout::FastScale(unit_size.cy),
                      *unit_symbol,
                      origin.x, origin.y,
                      unit_size.cx, unit_size.cy);
  }
}

void
InfoBoxWindow::PaintComment(Canvas &canvas)
{
  if (data.comment.empty())
    return;

  canvas.set_text_color(look.get_comment_color(data.comment_color));

  const Font &font = *look.comment.font;
  canvas.select(font);

  PixelSize tsize = canvas.text_size(data.comment);

  PixelScalar x = max(PixelScalar(1),
                      PixelScalar((comment_rect.left + comment_rect.right
                                   - tsize.cx) / 2));
  PixelScalar y = comment_rect.top + 1 + font.GetCapitalHeight()
    - font.GetAscentHeight();

  canvas.TextAutoClipped(x, y, data.comment);
}

void
InfoBoxWindow::PaintSelector(Canvas &canvas)
{
  canvas.select(look.selector_pen);

  const UPixelScalar width = canvas.get_width(), height = canvas.get_height();

  canvas.two_lines(width - SELECTORWIDTH - 1, 0,
                   width - 1, 0,
                   width - 1, SELECTORWIDTH + 1);

  canvas.two_lines(width - 1, height - SELECTORWIDTH - 2,
                   width - 1, height - 1,
                   width - SELECTORWIDTH - 1, height - 1);

  canvas.two_lines(SELECTORWIDTH + 1, height - 1,
                   0, height - 1,
                   0, height - SELECTORWIDTH - 2);

  canvas.two_lines(0, SELECTORWIDTH + 1,
                   0, 0,
                   SELECTORWIDTH + 1, 0);
}

void
InfoBoxWindow::Paint(Canvas &canvas)
{
  canvas.clear(look.background_brush);

  if (data.GetCustom() && content != NULL)
    content->on_custom_paint(*this, canvas);

  canvas.background_transparent();

  PaintTitle(canvas);
  PaintComment(canvas);
  PaintValue(canvas);

  if (border_kind != 0) {
    canvas.select(look.border_pen);

    const UPixelScalar width = canvas.get_width(),
      height = canvas.get_height();

    if (border_kind & BORDERTOP) {
      canvas.line(0, 0, width - 1, 0);
    }

    if (border_kind & BORDERRIGHT) {
      canvas.line(width - 1, 0, width - 1, height);
    }

    if (border_kind & BORDERBOTTOM) {
      canvas.line(0, height - 1, width - 1, height - 1);
    }

    if (border_kind & BORDERLEFT) {
      canvas.line(0, 0, 0, height - 1);
    }
  }
}

void
InfoBoxWindow::PaintInto(Canvas &dest, PixelScalar xoff, PixelScalar yoff,
                         UPixelScalar width, UPixelScalar height)
{
#ifdef ENABLE_OPENGL
  SubCanvas canvas(dest, xoff, yoff, width, height);
  Paint(canvas);
#else
  PixelSize size = get_size();
  BufferCanvas buffer(dest, size.cx, size.cy);

  Paint(buffer);
  dest.stretch(xoff, yoff, width, height, buffer, 0, 0, size.cx, size.cy);
#endif
}

void
InfoBoxWindow::SetContentProvider(InfoBoxContent *_content)
{
  delete content;
  content = _content;

  data.SetInvalid();
  invalidate();
}

void
InfoBoxWindow::UpdateContent()
{
  if (content == NULL)
    return;

  InfoBoxData old = data;
  content->Update(data);

  if (old.GetCustom() || data.GetCustom())
    /* must invalidate everything when custom painting is/was
       enabled */
    invalidate();
  else {
#ifdef ENABLE_OPENGL
    if (!data.CompareTitle(old) || !data.CompareValue(old) ||
        !data.CompareComment(old))
      invalidate();
#else
    if (!data.CompareTitle(old))
      invalidate(title_rect);
    if (!data.CompareValue(old))
      invalidate(value_rect);
    if (!data.CompareComment(old))
      invalidate(comment_rect);
#endif
  }
}

bool
InfoBoxWindow::HandleKey(InfoBoxContent::InfoBoxKeyCodes keycode)
{
  if (content != NULL && content->HandleKey(keycode)) {
    UpdateContent();
    return true;
  }
  return false;
}

bool
InfoBoxWindow::HandleQuickAccess(const TCHAR *value)
{
  if (content != NULL && content->HandleQuickAccess(value)) {
    UpdateContent();
    return true;
  }
  return false;
}

const InfoBoxContent::DialogContent *
InfoBoxWindow::GetDialogContent()
{
  if (content != NULL)
    return content->GetDialogContent();

  return NULL;
}

void
InfoBoxWindow::on_destroy()
{
  focus_timer.Cancel();
  PaintWindow::on_destroy();
}

bool
InfoBoxWindow::on_resize(UPixelScalar width, UPixelScalar height)
{
  PaintWindow::on_resize(width, height);

  PixelRect rc = get_client_rect();

  if (border_kind & BORDERLEFT)
    rc.left += look.BORDER_WIDTH;

  if (border_kind & BORDERRIGHT)
    rc.right -= look.BORDER_WIDTH;

  if (border_kind & BORDERTOP)
    rc.top += look.BORDER_WIDTH;

  if (border_kind & BORDERBOTTOM)
    rc.bottom -= look.BORDER_WIDTH;

  title_rect = rc;
  title_rect.bottom = rc.top + look.title.font->GetCapitalHeight() + 2;

  comment_rect = rc;
  comment_rect.top = comment_rect.bottom
    - (look.comment.font->GetCapitalHeight() + 2);

  value_rect = rc;
  value_rect.top = title_rect.bottom;
  value_rect.bottom = comment_rect.top;

  value_and_comment_rect = value_rect;
  value_and_comment_rect.bottom = comment_rect.bottom;

  return true;
}

bool
InfoBoxWindow::on_key_down(unsigned key_code)
{
  // Get the input event_id of the event
  if (InputEvents::ProcessKey(InputEvents::MODE_INFOBOX, key_code)) {
    // restart focus timer if not idle
    focus_timer.Schedule(FOCUS_TIMEOUT_MAX);
    return true;
  }

  return PaintWindow::on_key_down(key_code);
}

bool
InfoBoxWindow::on_mouse_down(PixelScalar x, PixelScalar y)
{
  click_clock.update();

  // if single clicked -> focus the InfoBoxWindow
  set_focus();
  return true;
}

bool
InfoBoxWindow::on_mouse_up(PixelScalar x, PixelScalar y)
{
  if (!has_focus())
    return PaintWindow::on_mouse_up(x, y);

  if (click_clock.check(1000)) {
    InfoBoxManager::ShowDlgInfoBox(id);
    return true;
  } else
    return PaintWindow::on_mouse_up(x, y);
}

bool
InfoBoxWindow::on_mouse_double(PixelScalar x, PixelScalar y)
{
  if (!is_altair()) {
    // JMW capture double click, so infoboxes double clicked also bring up menu
    // VENTA3: apparently this is working only on PC ! Disable it to let PC work
    // with same timeout of PDA and PNA versions with synthetic DBLCLK
    InputEvents::ShowMenu();
  }

  return true;
}

void
InfoBoxWindow::on_paint(Canvas &canvas)
{
  // Call the parent function
  Paint(canvas);

  // Paint the selector
  if (has_focus())
    PaintSelector(canvas);
}

bool
InfoBoxWindow::on_setfocus()
{
  // Call the parent function
  PaintWindow::on_setfocus();

  // Start the focus-auto-return timer
  // to automatically return focus back to MapWindow if idle
  focus_timer.Schedule(FOCUS_TIMEOUT_MAX);

  // Redraw fast to paint the selector
  invalidate();

  return true;
}

bool
InfoBoxWindow::on_killfocus()
{
  // Call the parent function
  PaintWindow::on_killfocus();

  // Destroy the time if it exists
  focus_timer.Cancel();

  // Redraw fast to remove the selector
  invalidate();

  return true;
}

bool
InfoBoxWindow::on_timer(WindowTimer &timer)
{
  if (timer == focus_timer) {
    focus_timer.Cancel();
    parent.set_focus();
    return true;
  } else
    return PaintWindow::on_timer(timer);
}
