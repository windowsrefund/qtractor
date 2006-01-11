// qtractorTrackList.cpp
//
/****************************************************************************
   Copyright (C) 2005-2006, rncbc aka Rui Nuno Capela. All rights reserved.

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#include "qtractorAbout.h"
#include "qtractorTrackList.h"
#include "qtractorTrackView.h"
#include "qtractorTrackCommand.h"
#include "qtractorTracks.h"
#include "qtractorSession.h"

#include "qtractorOptions.h"
#include "qtractorInstrument.h"
#include "qtractorAudioEngine.h"
#include "qtractorMidiEngine.h"
#include "qtractorMainForm.h"

#include <qapplication.h>
#include <qpopupmenu.h>
#include <qpainter.h>
#include <qcursor.h>
#include <qheader.h>
#include <qpen.h>


// Base item height (in pixels).
#define QTRACTOR_ITEM_HEIGHT	48

#if QT_VERSION < 0x030200
#define WNoAutoErase	(WResizeNoErase | WRepaintNoErase)
#endif


//----------------------------------------------------------------------------
// qtractorTrackListItem -- Tracks list item.

// Constructors.
qtractorTrackListItem::qtractorTrackListItem ( qtractorTrackList *pTrackList,
	qtractorTrack *pTrack )
	: QListViewItem(pTrackList, pTrackList->lastItem())
{
	initItem(pTrackList, pTrack);
}

qtractorTrackListItem::qtractorTrackListItem ( qtractorTrackList *pTrackList,
	qtractorTrack *pTrack, QListViewItem *pItemAfter )
	: QListViewItem(pTrackList, pItemAfter)
{
	initItem(pTrackList, pTrack);
}


// Common item initializer.
void qtractorTrackListItem::initItem ( qtractorTrackList *pTrackList,
	qtractorTrack *pTrack )
{
	m_pTrack = pTrack;

	QListViewItem::setMultiLinesEnabled(true);

	// FIXME: Track number prone to confusion...
	int iTrackNumber = pTrackList->childCount();
	setText(qtractorTrackList::Number, QString::number(iTrackNumber));
	setText(qtractorTrackList::Name,   pTrack->trackName());
	setText(qtractorTrackList::Record, "R");
	setText(qtractorTrackList::Mute,   "M");
	setText(qtractorTrackList::Solo,   "S");
	setText(qtractorTrackList::Bus,    pTrack->busName());
	// qtractorTrackList::Channel
	// qtractorTrackList::Patch
	// qtractorTrackList::Instrument
}


// Track list brainless accessor.
qtractorTrackList *qtractorTrackListItem::trackList (void) const
{
	return static_cast<qtractorTrackList *> (QListViewItem::listView());
}


// Track container accessor.
qtractorTrack *qtractorTrackListItem::track (void) const
{
	return m_pTrack;
}


// Overriden to set extra text info.
void qtractorTrackListItem::setText ( int iColumn, const QString& sText )
{
	if (m_pTrack == NULL || iColumn != qtractorTrackList::Bus) {
		QListViewItem::setText(iColumn, sText);
		return;
	}

	const QString s = "--";

	switch (m_pTrack->trackType()) {

		case qtractorTrack::Audio: {
			qtractorAudioBus *pAudioBus
				= static_cast<qtractorAudioBus *> (m_pTrack->bus());
			QListViewItem::setText(qtractorTrackList::Bus,
				(pAudioBus ? pAudioBus->busName() : s)  + '\n'
				+ QObject::tr("Audio"));
			QListViewItem::setText(qtractorTrackList::Channel,
				QString::number(pAudioBus->channels()));
			QListViewItem::setText(qtractorTrackList::Patch, s);
			QListViewItem::setText(qtractorTrackList::Instrument, s);
			break;
		}

		case qtractorTrack::Midi: {
			qtractorMidiBus *pMidiBus
				= static_cast<qtractorMidiBus *> (m_pTrack->bus());
			QListViewItem::setText(qtractorTrackList::Bus,
				(pMidiBus ? pMidiBus->busName() : s)  + '\n'
				+ QObject::tr("MIDI"));
			unsigned short iChannel = m_pTrack->midiChannel();
			const qtractorMidiBus::Patch& patch = pMidiBus->patch(iChannel);
			QListViewItem::setText(qtractorTrackList::Channel,
				QString::number(iChannel + 1));
			if (!patch.instrumentName.isEmpty()
				&& trackList()->instruments()) {
				qtractorInstrument& instr
					= (*trackList()->instruments())[patch.instrumentName];
				qtractorInstrumentData& bank
					= instr.patch(m_pTrack->midiBank());
				QListViewItem::setText(qtractorTrackList::Patch,
					bank[m_pTrack->midiProgram()] + '\n' + bank.name());
				QListViewItem::setText(qtractorTrackList::Instrument,
					patch.instrumentName);
			} else {
				QListViewItem::setText(qtractorTrackList::Patch, s);
				QListViewItem::setText(qtractorTrackList::Instrument, s);
			}
			break;
		}

		case qtractorTrack::None:
		default:
			QListViewItem::setText(qtractorTrackList::Bus,
				s + '\n' + QObject::tr("Unknown"));
			QListViewItem::setText(qtractorTrackList::Channel, s);
			QListViewItem::setText(qtractorTrackList::Patch, s);
			QListViewItem::setText(qtractorTrackList::Instrument, s);
			break;
	}
}


// Set track item height.
void qtractorTrackListItem::setItemHeight ( int iItemHeight )
{
	QListViewItem::setHeight(iItemHeight);

	if (m_pTrack == NULL)
		return;

	qtractorSession *pSession = m_pTrack->session();
	if (pSession == NULL)
		return;

	int iHeight = (iItemHeight * 100) / pSession->verticalZoom();
	m_pTrack->setHeight(iHeight);
}


// Zoom item track's height.
void qtractorTrackListItem::zoomItemHeight ( unsigned short iVerticalZoom )
{
	if (m_pTrack == NULL)
		return;

	int iItemHeight = (m_pTrack->height() * iVerticalZoom) / 100;
	if (iItemHeight > QTRACTOR_ITEM_HEIGHT / 2)
		QListViewItem::setHeight(iItemHeight);
}


// Overriden view item setup .
void qtractorTrackListItem::setup (void)
{
	QListViewItem::setup();

	if (m_pTrack == NULL)
		return;

	qtractorSession *pSession = m_pTrack->session();
	if (pSession == NULL)
		return;

	int iItemHeight = (m_pTrack->height() * pSession->verticalZoom()) / 100;
	QListViewItem::setHeight(iItemHeight);
}


// Overrriden cell painter.
void qtractorTrackListItem::paintCell ( QPainter *p, const QColorGroup& cg,
		int column, int width, int align )
{
	// Paint the original but with a different background...
	QColorGroup _cg(cg);

	QColor bg = _cg.color(QColorGroup::Background);
	QColor fg = _cg.color(QColorGroup::Foreground);

	bool bSelected = isSelected();
	if (column > qtractorTrackList::Number && bSelected) {
		bg = _cg.color(QColorGroup::Midlight).dark(150);
		fg = _cg.color(QColorGroup::Midlight).light(150);
	}

	switch (column) {
	case qtractorTrackList::Number:
		bg = m_pTrack->foreground().light();
		fg = m_pTrack->background().light();
		break;
	case qtractorTrackList::Record:
		if (m_pTrack->isRecord()) {
			bg = Qt::darkRed;
			fg = bg.light(250);
		}
		break;
	case qtractorTrackList::Mute:
		if (m_pTrack->isMute()) {
			bg = Qt::darkYellow;
			fg = bg.light(250);
		}
		break;
	case qtractorTrackList::Solo:
		if (m_pTrack->isSolo()) {
			bg = Qt::darkCyan;
			fg = bg.light(250);
		}
		break;
	}

	_cg.setColor(QColorGroup::Base, bg);
	_cg.setColor(QColorGroup::Text, fg);
	_cg.setColor(QColorGroup::Highlight, bg);
	_cg.setColor(QColorGroup::HighlightedText, fg);

	QListViewItem::paintCell(p, _cg, column, width, align);

	// Draw cell frame lines...
	int height = QListViewItem::height();
	p->setPen(bg.light(150));
	p->drawLine(0, 0, 0, height - 1);
	p->drawLine(0, 0, width - 1, 0);
	p->setPen(bg.dark(150));
	p->drawLine(width - 1, 0, width - 1, height - 1);
	p->drawLine(0, height - 1, width - 1, height - 1);
}


//----------------------------------------------------------------------------
// qtractorTrackList -- Track list widget.

// Constructor.
qtractorTrackList::qtractorTrackList ( qtractorTracks *pTracks,
	QWidget *pParent, const char *pszName )
	: QListView(pParent, pszName, WStaticContents | WNoAutoErase)
{
	m_pTracks = pTracks;

	m_dragState  = DragNone;
	m_pItemDrag  = NULL;
	m_iItemDragY = 0;

//	QListView::header()->setClickEnabled(false);
//	QListView::header()->setFixedHeight(QTRACTOR_ITEM_HEIGHT);

	QListView::viewport()->setPaletteBackgroundColor(Qt::darkGray);

	QListView::addColumn(tr("Nr"), 26);		// qtractorTrackList::Number
	QListView::addColumn(tr("Track Name"));	// qtractorTrackList::Name
	QListView::addColumn(tr("R"), 20);		// qtractorTrackList::Record
	QListView::addColumn(tr("M"), 20);		// qtractorTrackList::Mute
	QListView::addColumn(tr("S"), 20);		// qtractorTrackList::Solo
	QListView::addColumn(tr("Bus"));		// qtractorTrackList::Bus
	QListView::addColumn(tr("Ch"), 26);		// qtractorTrackList::Channel
	QListView::addColumn(tr("Patch"));		// qtractorTrackList::Patch
	QListView::addColumn(tr("Instrument"));	// qtractorTrackList::Instrumnet

	QListView::setColumnAlignment(qtractorTrackList::Number, Qt::AlignHCenter);
	QListView::setColumnAlignment(qtractorTrackList::Record, Qt::AlignHCenter);
	QListView::setColumnAlignment(qtractorTrackList::Mute, Qt::AlignHCenter);
	QListView::setColumnAlignment(qtractorTrackList::Solo, Qt::AlignHCenter);
	QListView::setColumnAlignment(qtractorTrackList::Channel, Qt::AlignHCenter);

	QListView::setColumnWidthMode(qtractorTrackList::Record, QListView::Manual);
	QListView::setColumnWidthMode(qtractorTrackList::Mute, QListView::Manual);
	QListView::setColumnWidthMode(qtractorTrackList::Solo, QListView::Manual);
	QListView::setColumnWidthMode(qtractorTrackList::Channel, QListView::Manual);

//	QListView::setResizeMode(QListView::LastColumn);
	QListView::setSelectionMode(QListView::Single);
	QListView::setAllColumnsShowFocus(true);
	QListView::setItemMargin(4);
	// No sorting column.
#if QT_VERSION >= 0x030200
	QListView::setSortColumn(-1);
#else
	QListView::setSorting(-1);
#endif
	QListView::setHScrollBarMode(QListView::AlwaysOn);
	QListView::setVScrollBarMode(QListView::AlwaysOff);

	// Simple click handling...
	QObject::connect(
		this, SIGNAL(clicked(QListViewItem*,const QPoint&,int)),
		this, SLOT(clickedSlot(QListViewItem*,const QPoint&,int)));
	// Context menu handling...
	QObject::connect(
		this, SIGNAL(contextMenuRequested(QListViewItem*,const QPoint&,int)),
		this, SLOT(contextMenuSlot(QListViewItem*,const QPoint&,int)));
}


// Find the list view item from track pointer reference.
qtractorTrackListItem *qtractorTrackList::trackItem ( qtractorTrack *pTrack )
{
	QListViewItem *pItem = QListView::firstChild();
	while (pItem) {
		qtractorTrackListItem *pTrackItem
			= static_cast<qtractorTrackListItem *> (pItem);
		if (pTrackItem->track() == pTrack)
			return pTrackItem;
		pItem = pItem->nextSibling();
	}
	return NULL;
}


// Renumber track list items.
void qtractorTrackList::renumberTrackItems ( QListViewItem *pItem )
{
	// Gotta start from somewhere...
	int iTrackNumber = 0;
	if (pItem == NULL) {
		pItem = QListView::firstChild();
	} else {
		iTrackNumber += pItem->text(qtractorTrackList::Number).toInt();
		pItem = pItem->nextSibling();
	}

	// Renumbering of all other remaining items.
	while (pItem) {
		pItem->setText(qtractorTrackList::Number,
			QString::number(++iTrackNumber));
		pItem = pItem->nextSibling();
	}
}


// Zoom all tracks item height.
void qtractorTrackList::zoomItemHeight ( int iVerticalZoom )
{
	QListViewItem *pItem = QListView::firstChild();
	while (pItem) {
		qtractorTrackListItem *pTrackItem
			= static_cast<qtractorTrackListItem *> (pItem);
		if (pTrackItem)
			pTrackItem->zoomItemHeight(iVerticalZoom);
		pItem = pItem->nextSibling();
	}

	// Update track view total contents height...
	m_pTracks->trackView()->updateContentsHeight();
	m_pTracks->trackView()->updateContents();
}


// Instrument list accessor helper.
qtractorInstrumentList *qtractorTrackList::instruments() const
{
	return m_pTracks->instruments();
}


// Trap this widget width for remembering it later.
void qtractorTrackList::resizeEvent ( QResizeEvent *pResizeEvent )
{
	// One shoukld only remember visible sizes.
	if (QListView::isVisible()) {
		qtractorOptions *pOptions = m_pTracks->mainForm()->options();
		if (pOptions)
			pOptions->iTrackListWidth = QListView::size().width();
	}

	QListView::resizeEvent(pResizeEvent);
}


// Overriden to catch early attributes (e.g. header height)
void qtractorTrackList::polish (void)
{
	// Highly recommended (avoid recursion?)
	QListView::polish();

	// Make sure header is polished by now too.
	QListView::header()->polish();

	emit polishNotifySignal();
}


// Handle item height resizing with mouse.
void qtractorTrackList::contentsMousePressEvent ( QMouseEvent *pMouseEvent )
{
	if (m_dragState == DragNone) {
		m_posDrag = pMouseEvent->pos();
		if (m_pItemDrag) {
			QListView::setCursor(QCursor(Qt::SplitVCursor));
			m_dragState = DragResize;
			drawDragLine(m_posDrag);    // Show.
		} else {
			QListView::contentsMousePressEvent(pMouseEvent);
			m_pItemDrag = QListView::itemAt(
				QListView::contentsToViewport(m_posDrag));
			if (m_pItemDrag) {
				QListView::setCursor(QCursor(Qt::PointingHandCursor));
				m_dragState = DragStart;
			}
		}
	}
}


void qtractorTrackList::contentsMouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	// We're already on some item dragging/resizing?...
	switch (m_dragState) {
	case DragMove:
		if (m_pItemDrag) {
			drawDragLine(m_posDrag);	// Hide.
			QListViewItem *pItemDrop = QListView::itemAt(
				QListView::contentsToViewport(pMouseEvent->pos()));
			if (pItemDrop) {
				m_posDrag = QListView::viewportToContents(
					QListView::itemRect(pItemDrop).bottomLeft());
			}
			drawDragLine(m_posDrag);	// Show.
		}
		break;
	case DragResize:
		// Currently resizing an item...
		if (m_pItemDrag) {
			drawDragLine(m_posDrag);    // Hide.
			int y = pMouseEvent->y();
			if (y < m_iItemDragY + QTRACTOR_ITEM_HEIGHT / 2)
				y = m_iItemDragY + QTRACTOR_ITEM_HEIGHT / 2;
			m_posDrag.setY(y);
			drawDragLine(m_posDrag);    // Show.
		}
		break;
	case DragStart:
		// About to start dragging an item...
		if (m_pItemDrag && (m_posDrag - pMouseEvent->pos()).manhattanLength()
				> QApplication::startDragDistance()) {
			QListView::setCursor(QCursor(Qt::SizeVerCursor));
			m_dragState = DragMove;
			m_posDrag = QListView::viewportToContents(
				QListView::itemRect(m_pItemDrag).bottomLeft());
			drawDragLine(m_posDrag);	// Show.
		}
		break;
	case DragNone:
	default:
		// Look for the mouse hovering around some item boundary...
		int y = 0;
		QListViewItem *pItem = QListView::firstChild();
		while (pItem) {
			m_iItemDragY = y;
			y += pItem->totalHeight();
			if (pMouseEvent->y() > y - 4 && pMouseEvent->y() < y + 4) {
				m_pItemDrag = pItem;
				QListView::setCursor(QCursor(Qt::SplitVCursor));
				break;
			} else if (m_pItemDrag && m_dragState == DragNone) {
				m_pItemDrag = NULL;
				QListView::unsetCursor();
			}
			pItem = pItem->nextSibling();
		}
		break;
	}

//	QListView::contentsMouseMoveEvent(pMouseEvent);
}


void qtractorTrackList::contentsMouseReleaseEvent ( QMouseEvent *pMouseEvent )
{
	QListView::contentsMouseReleaseEvent(pMouseEvent);

	// If we were resizing, now's time to let
	// things know that have changed somehow...
	switch (m_dragState) {
	case DragMove:
		if (m_pItemDrag) {
			QListViewItem *pItemDrop = QListView::itemAt(
				QListView::contentsToViewport(pMouseEvent->pos()));
			if (pItemDrop) {
				qtractorSession *pSession = m_pTracks->session();
				qtractorTrackListItem *pTrackItemDrag
					= static_cast<qtractorTrackListItem *> (m_pItemDrag);
				qtractorTrackListItem *pTrackItemDrop
					= static_cast<qtractorTrackListItem *> (pItemDrop);
				if (pSession && pTrackItemDrag && pTrackItemDrop) {
					qtractorTrack *pTrackDrag = pTrackItemDrag->track();
					qtractorTrack *pTrackDrop = pTrackItemDrop->track();
					if (pTrackDrag && pTrackDrop
						&& pTrackDrag != pTrackDrop
						&& pTrackDrag != pTrackDrop->next()) {
						m_pTracks->mainForm()->commands()->exec(
							new qtractorMoveTrackCommand(m_pTracks->mainForm(),
								pTrackDrag, pTrackDrop));
					}
				}
			}
		}
		break;
	case DragResize:
		if (m_pItemDrag) {
			drawDragLine(m_posDrag);    // Hide.
			int iItemHeight = pMouseEvent->y() - m_iItemDragY;
			// Check for minimum item height.
			if (iItemHeight < QTRACTOR_ITEM_HEIGHT / 2)
				iItemHeight = QTRACTOR_ITEM_HEIGHT / 2;
			// Go for it...
			qtractorTrackListItem *pTrackItem
				= static_cast<qtractorTrackListItem *> (m_pItemDrag);
			if (pTrackItem) {
				m_pTracks->mainForm()->commands()->exec(
					new qtractorResizeTrackCommand(m_pTracks->mainForm(),
						pTrackItem->track(), iItemHeight));
			}
		}
		// Fall thru...
	case DragStart:
	case DragNone:
	default:
		break;
	}
	
	resetDragState();
}


// Draw a dragging separator line.
void qtractorTrackList::drawDragLine ( const QPoint& posDrag, int iThickness ) const
{
	QWidget *pViewport = QListView::viewport();
	int w = pViewport->width();
	int h = pViewport->height();

	// Make sure the line doesn't get too off view...
	QPoint pos = QListView::contentsToViewport(posDrag);
	if (pos.y() < 0)
		pos.setY(0);
	if (pos.y() > h)
		pos.setY(h);

	QPainter p(pViewport);

	QPen pen;
	pen.setColor(Qt::gray);
	pen.setWidth(iThickness);

	p.setRasterOp(Qt::NotROP);
	p.setPen(pen);
	p.drawLine(0, pos.y(), w, pos.y());
}


// Reset drag/select/move state.
void qtractorTrackList::resetDragState (void)
{
	// Cancel any dragging out there...
	if (m_dragState == DragMove || m_dragState == DragResize)
		drawDragLine(m_posDrag);    // Hide.

	// Should fallback mouse cursor...
	if (m_dragState != DragNone)
		QListView::unsetCursor();

	// Not dragging anymore.
	m_dragState  = DragNone;
	m_pItemDrag  = NULL;
	m_iItemDragY = 0;
}


// Keyboard event handler.
void qtractorTrackList::keyPressEvent ( QKeyEvent *pKeyEvent )
{
#ifdef CONFIG_DEBUG
	fprintf(stderr, "qtractorTrackList::keyPressEvent(key=%d)\n", pKeyEvent->key());
#endif
	switch (pKeyEvent->key()) {
	case Qt::Key_Escape:
		resetDragState();
		break;
	default:
		QListView::keyPressEvent(pKeyEvent);
		break;
	}
}


// To have track list in v-sync with main track view.
void qtractorTrackList::contentsMovingSlot ( int /*cx*/, int cy )
{
	if (QListView::contentsY() != cy)
		m_pTracks->setContentsPos(this, QListView::contentsX(), cy);
}


// Context menu request slot.
void qtractorTrackList::contextMenuSlot ( QListViewItem* /*pItem*/,
	const QPoint& pos, int /*col*/ )
{
	m_pTracks->mainForm()->trackMenu->exec(pos);
}


// Simple click handler.
void qtractorTrackList::clickedSlot ( QListViewItem *pItem,
	const QPoint& /*pos*/, int col )
{
	qtractorTrack *pTrack = NULL;
	qtractorTrackListItem *pTrackItem
		= static_cast<qtractorTrackListItem *> (pItem);
	if (pTrackItem)
		pTrack = pTrackItem->track();
	if (pTrack == NULL)
		return;

	switch (col) {
	case qtractorTrackList::Record:
		pTrack->setRecord(!pTrack->isRecord());
		break;
	case qtractorTrackList::Mute:
		pTrack->setMute(!pTrack->isMute());
		break;
	case qtractorTrackList::Solo:
		pTrack->setSolo(!pTrack->isSolo());
		break;
	default:
		// Bail out.
		return;
	}

	// Update only the changed cell area...
	QRect sectRect = QListView::header()->sectionRect(col);
	QRect itemRect = QListView::itemRect(pItem);
	itemRect.setX(sectRect.x());
	itemRect.setWidth(sectRect.x());
	QListView::viewport()->update(itemRect);

	m_pTracks->contentsChangeNotify();
}


// end of qtractorTrackList.cpp
