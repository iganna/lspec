/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2012 UniPro <ugene@unipro.ru>
 * http://ugene.unipro.ru
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "MSAEditorNameList.h"
#include "MSAEditor.h"
#include "MSAEditorSequenceArea.h"

#include <U2Core/MAlignmentObject.h>
#include <U2Core/MAlignment.h>
#include <U2Gui/GUIUtils.h>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QInputDialog>

namespace U2 {

static const int CROSS_SIZE = 9;
#define CHILDREN_OFFSET 8

MSAEditorNameList::MSAEditorNameList(MSAEditorUI* _ui, QScrollBar* _nhBar) : labels(NULL), editor(_ui->editor), ui(_ui), nhBar(_nhBar) {
    setObjectName("msa_editor_name_list");
    setFocusPolicy(Qt::WheelFocus);
    cachedView = new QPixmap();
    completeRedraw = true;
    scribbling = false;
    shifting = false;
    curSeq = -1;
    rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    
    connect(editor, SIGNAL(si_buildStaticMenu(GObjectView*, QMenu*)), SLOT(sl_buildStaticMenu(GObjectView*, QMenu*)));
    
    editSequenceNameAction = new QAction(tr("Edit sequence name"), this);
    connect(editSequenceNameAction, SIGNAL(triggered()), SLOT(sl_editSequenceName()));
    
    copyCurrentSequenceAction = new QAction(tr("Copy current sequence"), this);
    connect(copyCurrentSequenceAction, SIGNAL(triggered()), SLOT(sl_copyCurrentSequence()));

    removeCurrentSequenceAction = new QAction("Remove current sequence", this);
    connect(removeCurrentSequenceAction, SIGNAL(triggered()), SLOT(sl_removeCurrentSequence()));
        
    connect(editor, SIGNAL(si_buildPopupMenu(GObjectView* , QMenu*)), SLOT(sl_buildContextMenu(GObjectView*, QMenu*)));
    if (editor->getMSAObject()) {
        connect(editor->getMSAObject(), SIGNAL(si_alignmentChanged(const MAlignment&, const MAlignmentModInfo&)), 
            SLOT(sl_alignmentChanged(const MAlignment&, const MAlignmentModInfo&)));
        connect(editor->getMSAObject(), SIGNAL(si_lockedStateChanged()), SLOT(sl_lockedStateChanged()));
    }

    if (ui->seqArea) {
        connect(ui->seqArea, SIGNAL(si_startChanged(const QPoint& , const QPoint& )), SLOT(sl_startChanged(const QPoint& , const QPoint&)));
        connect(ui->seqArea, SIGNAL(si_selectionChanged(const MSAEditorSelection&, const MSAEditorSelection&)), SLOT(sl_selectionChanged(const MSAEditorSelection& , const MSAEditorSelection& )));
        connect(ui->editor, SIGNAL(si_fontChanged(const QFont&)), SLOT(sl_fontChanged()));
        connect(ui->seqArea->getVBar(), SIGNAL(actionTriggered(int)), SLOT(sl_onScrollBarActionTriggered(int)));
    }
    connect(ui->getCollapseModel(), SIGNAL(toggled()), SLOT(sl_modelChanged()));
    
    nhBar->setEnabled(false);
    updateActions();

    QObject *labelsParent = new QObject(this);
    labelsParent->setObjectName("labels_parent");
    labels = new QObject(labelsParent);
}

MSAEditorNameList::~MSAEditorNameList() {
    delete cachedView;
}

void MSAEditorNameList::updateActions() {
    copyCurrentSequenceAction->setEnabled(true);
    
    MAlignmentObject* maObj = editor->getMSAObject();
    if (maObj){
        removeCurrentSequenceAction->setEnabled(!maObj->isStateLocked() && maObj->getMAlignment().getNumRows() > 1);
        editSequenceNameAction->setEnabled(!maObj->isStateLocked());
    }
}

#define MARGIN_TEXT_LEFT 5
#define MARGIN_TEXT_TOP 2
#define MARGIN_TEXT_BOTTOM 2

void MSAEditorNameList::updateScrollBar() {
    nhBar->disconnect(this);

    QFont f = ui->editor->getFont();
    f.setItalic(true);
    QFontMetrics fm(f);
    int maxNameWidth = 0;

    MAlignmentObject* maObj = editor->getMSAObject();
    foreach(const MAlignmentRow& row, maObj->getMAlignment().getRows()) {
        maxNameWidth = qMax(fm.width(row.getName()), maxNameWidth);
    }
    // adjustment for branch primitive in collapsing mode
    if (ui->isCollapsibleMode()) {
        maxNameWidth += 2*CROSS_SIZE + CHILDREN_OFFSET;
    }

    int availableWidth = width() - MARGIN_TEXT_LEFT;
    int nSteps = 1;
    int stepSize = fm.width('W');
    if (availableWidth < maxNameWidth) {
        int dw = maxNameWidth - availableWidth;
        nSteps += dw / stepSize + (dw % stepSize != 0 ? 1 : 0);
    }
    nhBar->setMinimum(0);
    nhBar->setMaximum(nSteps - 1);
    nhBar->setValue(0);

    nhBar->setEnabled(nSteps > 1);
    connect(nhBar, SIGNAL(valueChanged(int)), SLOT(sl_nameBarMoved(int)));
}


void MSAEditorNameList::sl_buildStaticMenu(GObjectView* v, QMenu* m) {
    Q_UNUSED(v);
    buildMenu(m);
}

void MSAEditorNameList::sl_buildContextMenu(GObjectView* v, QMenu* m) {
    Q_UNUSED(v);
    buildMenu(m);
}

void MSAEditorNameList::buildMenu(QMenu* m) {    
    
    if ( !rect().contains( mapFromGlobal(QCursor::pos()) ) )  {
        return;
    } 
    
    QMenu* copyMenu = GUIUtils::findSubMenu(m, MSAE_MENU_COPY);
    assert(copyMenu!=NULL);
    copyMenu->addAction(copyCurrentSequenceAction);

    QMenu* editMenu = GUIUtils::findSubMenu(m, MSAE_MENU_EDIT);
    assert(editMenu!=NULL);
    editMenu->addAction(removeCurrentSequenceAction);
    editMenu->insertAction(editMenu->actions().first(), editSequenceNameAction);
   
}

int MSAEditorNameList::getSelectedRow() const {
    const MSAEditorSelection& selection = ui->seqArea->getSelection();
    if (selection.height() == 0) {
        return -1;
    }
    int n = selection.y();
    if (ui->isCollapsibleMode()) {
        n = ui->getCollapseModel()->mapToRow(n);
    }
    return n;
}

void MSAEditorNameList::sl_copyCurrentSequence() {
    int n = getSelectedRow();
    MAlignmentObject* maObj = editor->getMSAObject();
    if (maObj) {
        const MAlignmentRow& row = maObj->getMAlignment().getRow(n);
        //TODO: trim large sequence?
        QApplication::clipboard()->setText(row.toByteArray(maObj->getLength()));
    }
}

void MSAEditorNameList::sl_alignmentChanged(const MAlignment&, const MAlignmentModInfo& mi) {
    if (mi.sequenceListChanged) {
        completeRedraw = true;
        updateScrollBar();
        update();
    }
}

void MSAEditorNameList::sl_nameBarMoved(int) {
    completeRedraw = true;
    update();
}

void MSAEditorNameList::sl_removeCurrentSequence() {
    MAlignmentObject* maObj = editor->getMSAObject();
    if (maObj) {
        int n = getSelectedRow();
        if (n < 0) {
            return;
        }
        assert(!maObj->isStateLocked());
        maObj->removeRow(n);
        //MAlignment ma = maObj->getMAlignment();
        //assert(ma.getNumSequences() > 1);
        //ma.alignedSeqs.removeAt(n);
        //maObj->setMAlignment(ma);
    }
}

void MSAEditorNameList::sl_lockedStateChanged() {
    updateActions();
}

void MSAEditorNameList::resizeEvent(QResizeEvent* e) {

    completeRedraw = true;
    updateScrollBar();
    QWidget::resizeEvent(e);
}

void MSAEditorNameList::paintEvent(QPaintEvent*) {
    drawAll();
}

void MSAEditorNameList::keyPressEvent (QKeyEvent *e) {
    int key = e->key();
    switch(key) {
         case Qt::Key_Delete:
             ui->seqArea->deleteCurrentSelection();    
             break;
        case Qt::Key_Up:
            ui->seqArea->moveSelection(0, -1);    
            break;
        case Qt::Key_Down:
            ui->seqArea->moveSelection(0,  1);    
            break;
        case Qt::Key_Left:
            nhBar->triggerAction(QAbstractSlider::SliderSingleStepSub);
            break;
        case Qt::Key_Right:
            nhBar->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            break;
        case Qt::Key_Home:
            ui->seqArea->setFirstVisibleSequence(0);
            ui->seqArea->cancelSelection();
            //TODO: select first sequence?
            break;
        case Qt::Key_End: 
            {
                int s = editor->getNumSequences() - 1;
                ui->seqArea->setFirstVisibleSequence(s);
                ui->seqArea->cancelSelection();
                //TODO: select last sequence?
            }            
            break;
        case Qt::Key_PageUp:
            {
                int nVis = ui->seqArea->getNumVisibleSequences(false);
                int fp = qMax(0, ui->seqArea->getFirstVisibleSequence() - nVis);
                //int cp = qMax(0, ui->seqArea->getCursorPos().y() - nVis);
                ui->seqArea->setFirstVisibleSequence(fp);
                ui->seqArea->cancelSelection();
            }
            break;
        case Qt::Key_PageDown:
            {
                int nVis = ui->seqArea->getNumVisibleSequences(false);
                int nSeq = editor->getNumSequences();
                int fp = qMin(nSeq-1, ui->seqArea->getFirstVisibleSequence() + nVis);
                //int cp = qMin(nSeq-1, ui->seqArea->getCursorPos().y() + nVis);
                ui->seqArea->setFirstVisibleSequence(fp);
                ui->seqArea->cancelSelection();
            }
            break;
  }
    QWidget::keyPressEvent(e);
}

void MSAEditorNameList::mousePressEvent(QMouseEvent *e) {
    if ((e->button() == Qt::LeftButton)){
        origin = e->pos();
        curSeq = ui->seqArea->getSequenceNumByY(e->y());
        if (ui->isCollapsibleMode()) {
            MSACollapsibleItemModel* m = ui->getCollapseModel();
            if (m->isTopLevel(curSeq)) {
                const U2Region& yRange = ui->seqArea->getSequenceYRange(curSeq, true);
                bool selected = isRowInSelection(curSeq);
                QRect textRect = calculateTextRect(yRange, selected);
                QRect buttonRect = calculateButtonRect(textRect);
                if (buttonRect.contains(origin)) {
                    m->toggle(curSeq);
                    return;
                }
            }
        }
        MSAEditorSelection s = ui->seqArea->getSelection();
        if ( s.getRect().contains(0,curSeq) ) {
            if (!ui->isCollapsibleMode()) {
                shifting = true;
            }
        } else {
            rubberBand->setGeometry(QRect(origin, QSize()));
            rubberBand->show();
            ui->seqArea->cancelSelection();
        }
        if ( ui->seqArea->isSeqInRange(curSeq) ) {
            scribbling = true;
        }
    }

    QWidget::mousePressEvent(e);
}

void MSAEditorNameList::mouseMoveEvent( QMouseEvent* e )
{
    if ( (e->buttons() & Qt::LeftButton) && scribbling ) {
        int newSeqNum = ui->seqArea->getSequenceNumByY(e->pos().y());
        if (ui->seqArea->isSeqInRange(newSeqNum)) {
            ui->seqArea->updateVBarPosition(newSeqNum);
        }
        if (shifting) {
            assert(!ui->isCollapsibleMode());
            moveSelectedRegion(newSeqNum - curSeq);
        } else {
            rubberBand->setGeometry(QRect(origin, e->pos()).normalized());
        }
    }
    QWidget::mouseMoveEvent(e);

}

void MSAEditorNameList::mouseReleaseEvent( QMouseEvent *e )
{
    rubberBand->hide();
    if (scribbling) {
        int newSeq = ui->seqArea->getSequenceNumByY(e->y());
        if (!ui->seqArea->isSeqInRange(newSeq) ) {
            if (e->y() < origin.y()) {
                newSeq = 0;
            } else {
                newSeq = ui->editor->getNumSequences() - 1;
            }
        }
        if (e->pos() == origin) {
            // special case: click but don't drag
            shifting = false;
        }
        if (shifting) {
            assert(!ui->isCollapsibleMode());
            int shift = 0;
            int numSeq = ui->editor->getNumSequences();
            int selectionStart = ui->seqArea->getSelection().y();
            int selectionSize = ui->seqArea->getSelection().height();
            if (newSeq == 0) {
                shift = -selectionStart;
            } else if (newSeq == numSeq - 1) {
                shift = numSeq - (selectionStart + selectionSize); 
            } else {
                shift = newSeq - curSeq;  
            }
            moveSelectedRegion(shift);
            shifting = false;
        } else {
            updateSelection(newSeq);
        }
        scribbling = false;
    }
    ui->seqArea->getVBar()->setupRepeatAction(QAbstractSlider::SliderNoAction);
    
    QWidget::mouseReleaseEvent(e);
}

void MSAEditorNameList::updateSelection( int newSeq )
{
    if (newSeq != -1) {
        int startSeq = qMin(curSeq,newSeq);
        int width = editor->getAlignmentLen();
        int height = qAbs(newSeq - curSeq) + 1;
        MSAEditorSelection selection(0, startSeq, width, height );
        ui->seqArea->setSelection(selection);
    }
}

void MSAEditorNameList::wheelEvent (QWheelEvent * we) {
    bool toMin = we->delta() > 0;
    ui->seqArea->getVBar()->triggerAction(toMin ? QAbstractSlider::SliderSingleStepSub : QAbstractSlider::SliderSingleStepAdd);
    QWidget::wheelEvent(we);
}


void MSAEditorNameList::sl_startChanged(const QPoint& p, const QPoint& prev) {
    if (p.y() == prev.y()) {
        return;
    }
    completeRedraw = true;
    update();
}

void MSAEditorNameList::sl_selectionChanged( const MSAEditorSelection& current, const MSAEditorSelection& prev )
{
    Q_UNUSED(current);
    Q_UNUSED(prev);
    
    if (current.y() == prev.y() && current.height() == prev.height() ) {
        return;
    }
    completeRedraw = true;
   
    update();

}

void MSAEditorNameList::focusInEvent(QFocusEvent* fe) {
    QWidget::focusInEvent(fe);
    update();
}

void MSAEditorNameList::focusOutEvent(QFocusEvent* fe) {
    QWidget::focusOutEvent(fe);
    update();
}

void MSAEditorNameList::sl_fontChanged() {
    completeRedraw = true; 
    updateScrollBar(); 
    update();
}

void MSAEditorNameList::sl_modelChanged() {
    completeRedraw = true;
    update();
}

//////////////////////////////////////////////////////////////////////////
// draw methods
QFont MSAEditorNameList::getFont(bool selected) const {
    QFont f = ui->editor->getFont();
    f.setItalic(true);
    if (selected) {
        f.setBold(true);
    }
    return f;
}

QRect MSAEditorNameList::calculateTextRect(const U2Region& yRange, bool selected) const {
    int w = width();
    int textX = MARGIN_TEXT_LEFT;
    int textW = w - MARGIN_TEXT_LEFT;
    int textY = yRange.startPos + MARGIN_TEXT_TOP;
    int textH = yRange.length - MARGIN_TEXT_TOP - MARGIN_TEXT_BOTTOM;
    QRect textRect(textX, textY, textW, textH);
    if (nhBar->isEnabled()) {
        QFontMetrics fm(getFont(selected));
        int stepSize = fm.width('W');
        int dx = stepSize * nhBar->value();
        textRect = textRect.adjusted(-dx, 0, 0, 0);
    }
    return textRect;
}

QRect MSAEditorNameList::calculateButtonRect(const QRect& itemRect) const {
    return QRect(itemRect.left() + CROSS_SIZE/2, itemRect.top() + (itemRect.height() - CROSS_SIZE)/2, CROSS_SIZE, CROSS_SIZE);
}

void MSAEditorNameList::drawAll() {
    QSize s = size();
    if (cachedView->size() != s) {
        assert(completeRedraw);
        delete cachedView;
        cachedView = new QPixmap(s);
    }
    if (completeRedraw) {
        QPainter pCached(cachedView);
        drawContent(pCached);
        completeRedraw = false;
    }
    QPainter p(this);
    p.drawPixmap(0, 0, *cachedView);
    drawSelection(p);
    //drawFocus(p);
}

void MSAEditorNameList::drawContent(QPainter& p) {
    p.fillRect(cachedView->rect(), Qt::white);
    int startSeq = ui->seqArea->getFirstVisibleSequence();
    int lastSeq = ui->seqArea->getLastVisibleSequence(true);

    if (labels) {
        labels->setObjectName("");
    }
    if (ui->isCollapsibleMode()) {
        MSACollapsibleItemModel* m = ui->getCollapseModel();
        QVector<U2Region> range;
        m->getVisibleRows(startSeq, lastSeq, range);
        U2Region yRange = ui->seqArea->getSequenceYRange(startSeq, true);
        int numRows = editor->getNumSequences();

        int pos = startSeq;
        foreach(const U2Region& r, range) {
            int end = qMin((qint64)numRows, r.endPos());
            for (int s = r.startPos; s < end; s++) {
                bool isSelected = isRowInSelection(pos);
                drawSequenceItem(p, s, isSelected, yRange, pos);
                yRange.startPos += ui->editor->getRowHeight();
                pos++;
            }
        }
    } else {
        for (int s = startSeq; s <= lastSeq; s++) {
            bool isSelected = isRowInSelection(s);
            drawSequenceItem(p, s, isSelected);
        }
    }
}

void MSAEditorNameList::drawSequenceItem(QPainter& p, int s, bool selected) {
    p.setPen(Qt::black);
    p.setFont(getFont(selected));

    U2Region yRange = ui->seqArea->getSequenceYRange(s, true);
    QRect textRect = calculateTextRect(yRange, selected);

    MAlignmentObject* maObj = editor->getMSAObject();
    if (maObj == NULL)
        return;

    const MAlignment& ma = maObj->getMAlignment();
    const MAlignmentRow& row = ma.getRow(s);
    p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, row.getName());
    if (labels) {
        labels->setObjectName(labels->objectName() + "|" + row.getName());
    }
}

void MSAEditorNameList::drawSequenceItem(QPainter& p, int s, bool selected, const U2Region& yRange, int pos) {
    p.setPen(Qt::black);
    p.setFont(getFont(selected));

    MAlignmentObject* maObj = editor->getMSAObject();

    const MAlignment& ma = maObj->getMAlignment();
    const MAlignmentRow& row = ma.getRow(s);

    QRect textRect = calculateTextRect(yRange, selected);
    
    MSACollapsibleItemModel const* model = ui->getCollapseModel();
    int index = model->itemAt(pos);

    QStyleOption branchOption;

    int delta = 0;

    if (index >= 0) {
        branchOption.rect = calculateButtonRect(textRect);

        const MSACollapsableItem& item = model->getItem(index);
        if (item.isCollapsed) {
            branchOption.state = QStyle::State_Children;
        } else {
            if (pos == model->getItemPos(index)) {
                branchOption.state = QStyle::State_Open | QStyle::State_Children;
            } else {
                branchOption.rect.setTop(yRange.startPos);
                branchOption.rect.setHeight(yRange.length);
                int itemLastPos = model->getItemPos(index) + item.numRows - 1;
                branchOption.state = QStyle::State_Item;
                if (itemLastPos != pos) {
                    branchOption.state |= QStyle::State_Sibling;
                }
                delta = CHILDREN_OFFSET;
            }
        }
        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, &p, this);
    }

    textRect = QRect(textRect.left() + CROSS_SIZE*2 + delta, textRect.top(), textRect.width() - ((5*CROSS_SIZE)/2), textRect.height());
    style()->drawItemText(&p, textRect, Qt::AlignVCenter | Qt::AlignLeft, branchOption.palette, true, row.getName());
    if (labels) {
        labels->setObjectName(labels->objectName() + "|" + row.getName());
    }
}

void MSAEditorNameList::drawSelection(QPainter& p) {
    MSAEditorSelection sel = ui->seqArea->getSelection();
    
    if ((sel.height() == editor->getNumSequences()) || (sel.height() == 0)) {
        return;
    }

    int startPos = sel.y();
    int w = width();
    
    U2Region yRange = ui->seqArea->getSequenceYRange(startPos, true);
    QRect itemsRect(0, yRange.startPos, w - 1, sel.height()*yRange.length -1 );       
    
    p.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    p.drawRect(itemsRect);

}

void MSAEditorNameList::drawFocus(QPainter& p) {
    if (hasFocus()) {
        p.setPen(QPen(Qt::black, 1, Qt::DotLine));
        p.drawRect(0, 0, width()-1, height()-1);
    }
}

void MSAEditorNameList::sl_onScrollBarActionTriggered( int scrollAction )
{
    if (scrollAction ==  QAbstractSlider::SliderSingleStepAdd || scrollAction == QAbstractSlider::SliderSingleStepSub) {
        if (scribbling) {
            // TODO: sync origin point
            /*int y = origin.y();
            int step = ui->seqArea->getVBar()->singleStep();
            if (scrollAction == QAbstractSlider::SliderSingleStepAdd) {
                origin.setY(y - step);
            } else {
                origin.setY(y + step);
            }*/
        }
    }
}

void MSAEditorNameList::sl_editSequenceName()
{
    MAlignmentObject* maObj = editor->getMSAObject();
    if (maObj->isStateLocked()) {
        return;
    }
    
    bool ok = false;
    int n = getSelectedRow();
    if (n<0) {
        return;
    }
    const MAlignmentRow& row = maObj->getMAlignment().getRow(n);
    QString curName = row.getName();
    QString newName = QInputDialog::getText(this, tr("Rename"),
            tr("New sequence name:"), QLineEdit::Normal, curName, &ok);
    if (ok && !newName.isEmpty()) {
        maObj->renameRow(n,newName);
    }
}

void MSAEditorNameList::mouseDoubleClickEvent( QMouseEvent *e )
{
    Q_UNUSED(e);
    sl_editSequenceName();
}

void MSAEditorNameList::moveSelectedRegion( int shift )
{
    if (shift == 0) {
        return;
    }
    
    int numRowsInSelection = ui->seqArea->getSelection().height();
    int firstRowInSelection = ui->seqArea->getSelection().y();
    int lastRowInSelection = firstRowInSelection + numRowsInSelection - 1;
    
    // "out-of-range" checks
    if ( (shift > 0 && lastRowInSelection + shift >= editor->getNumSequences() )  || 
        (shift < 0 && firstRowInSelection + shift < 0 ) ||
        (shift < 0 && firstRowInSelection + qAbs(shift) > editor->getNumSequences()) ) {
            return;
    }

    MAlignmentObject* maObj = editor->getMSAObject();
    if ( !maObj->isStateLocked() ) {
        maObj->moveRowsBlock(firstRowInSelection, numRowsInSelection, shift);
        curSeq += shift;
        int selectionStart = firstRowInSelection + shift;
        MSAEditorSelection selection(0, selectionStart, editor->getAlignmentLen(), numRowsInSelection );
        ui->seqArea->setSelection(selection);
    }
}

bool MSAEditorNameList::isRowInSelection( int seqnum )
{
    MSAEditorSelection s = ui->seqArea->getSelection();
    int endPos = s.y() + s.height() - 1;
    return seqnum >= s.y() && seqnum <= endPos;
}

}//namespace
