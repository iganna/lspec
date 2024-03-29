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

#include "DotPlotClasses.h"

namespace U2 {

DotPlotMiniMap::DotPlotMiniMap(int bigMapW, int bigMapH, float ratio) {

    this->ratio = ratio;

    Q_ASSERT(ratio>0);

    w = bigMapW/ratio;
    h = bigMapH/ratio;

    // coords of the mini map on widget
    x = bigMapW - w;
    y = bigMapH - h;
}

QRectF DotPlotMiniMap::getBoundary() const {

    return QRectF(x, y, w, h);
}

// get coordinates on the dotplot picture matching point on the minimap
QPointF DotPlotMiniMap::fromMiniMap(const QPointF &p, const QPointF &zoom) const {

    float lx = p.x() - x;
    float ly = p.y() - y;

    return QPointF((lx*zoom.x() - w/2)*ratio, (ly*zoom.y() - h/2)*ratio);
}

// draw minimap
void DotPlotMiniMap::draw(QPainter &p, int shiftX, int shiftY, const QPointF &zoom) const {

    p.save();

    p.setBrush(QBrush(QColor(200, 200, 200, 100)));

    p.translate(x, y);
    p.drawRect(0, 0, w, h);

    QRect r((-shiftX/ratio)/zoom.x(), (-shiftY/ratio)/zoom.y(), w/zoom.x(), h/zoom.y());

    if (!r.width()) {
        r.setWidth(1);
    }
    if (!r.height()) {
        r.setHeight(1);
    }
    p.drawRect(r);

    p.restore();
}


DotPlotResultsListener::DotPlotResultsListener() {

    dotPlotList = new QList<DotPlotResults>();
    stateOk = true;
    rfTask = NULL;
}

DotPlotResultsListener::~DotPlotResultsListener() {

    delete dotPlotList;
}

void DotPlotResultsListener::setTask(Task *t) {

    rfTask = t;
    stateOk = true;
}

// add new found results to the list
void DotPlotResultsListener::onResult(const RFResult& r) {
    QMutexLocker locker(&mutex);

    if (!dotPlotList) {
        return;
    }

    if (dotPlotList->size() > maxResults) {
        Q_ASSERT(rfTask);
        rfTask->cancel();
        stateOk = false;

        return;
    }

    DotPlotResults vec(r.x, r.y, r.l);
    dotPlotList->push_back(vec);
}

void DotPlotResultsListener::onResults(const QVector<RFResult>& v) {
    QMutexLocker locker(&mutex);

    foreach (const RFResult &r, v) {

        if (!dotPlotList) {
            return;
        }

        if (dotPlotList->size() > maxResults) {
            Q_ASSERT(rfTask);
            rfTask->cancel();
            stateOk = false;

            return;
        }

        DotPlotResults vec(r.x, r.y, r.l);
        dotPlotList->push_back(vec);
    }
}

// add new found results to the list
void DotPlotRevComplResultsListener::onResult(const RFResult& r) {
    RFResult res(xLen - r.x - r.l, r.y , r.l);
    assert(res.x >= 0 && res.x < xLen);
    DotPlotResultsListener::onResult(res);

}

void DotPlotRevComplResultsListener::onResults(const QVector<RFResult>& v) {
    QVector<RFResult> results;
    results.reserve(v.size());
    foreach(const RFResult& r, v) {
        RFResult res(xLen - r.x - r.l, r.y, r.l);
        assert(res.x >= 0 && res.x < xLen);
        results << res;
    }
    DotPlotResultsListener::onResults(results);
}


// dialogs and error messages
void DotPlotDialogs::taskRunning() {

    QMessageBox mb(QMessageBox::Critical, tr("Task is already running"), tr("Build or Load DotPlot task is already running"));
    mb.exec();
}

int DotPlotDialogs::saveDotPlot() {

    QMessageBox mb(QMessageBox::Information, tr("Save dot-plot"), tr("Save dot-plot data before closing?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    return mb.exec();
}

void DotPlotDialogs::fileOpenError(const QString &filename) {

    QMessageBox mb(QMessageBox::Critical, tr("File opening error"), tr("Error opening file %1").arg(filename));
    mb.exec();
}

void DotPlotDialogs::filesOpenError() {

    QMessageBox mb(QMessageBox::Critical, tr("Error"), tr("Error opening files"));
    mb.exec();
}

int DotPlotDialogs::loadDifferent() {

    QMessageBox mb(QMessageBox::Critical, tr("Sequences are different"), tr("Current and loading sequences are different. Continue loading dot-plot anyway?"), QMessageBox::Yes | QMessageBox::No);
    return mb.exec();
}

void DotPlotDialogs::loadWrongFormat() {

    QMessageBox mb(QMessageBox::Critical, tr("Wrong format"), tr("Wrong dot-plot file format, loading aborted"));
    mb.exec();
}

void DotPlotDialogs::wrongAlphabetTypes() {

    QMessageBox mb(QMessageBox::Critical, tr("Wrong alphabet types"), tr("Both sequence must have the same alphabet"));
    mb.exec();
}

void DotPlotDialogs::tooManyResults() {

    QMessageBox mb(QMessageBox::Critical, tr("Too many results"), tr("Too many results. Try to increase minimum repeat length"));
    mb.exec();
}

}
