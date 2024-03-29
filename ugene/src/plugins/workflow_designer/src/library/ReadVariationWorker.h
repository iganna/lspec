/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2011 UniPro <ugene@unipro.ru>
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

#ifndef _READ_VARIATION_LIST_WORKER_
#define _READ_VARIATION_LIST_WORKER_

#include "GenericReadWorker.h"

namespace U2 {
namespace LocalWorkflow {

class ReadVariationWorker : public GenericDocReader {
    Q_OBJECT
public:
    ReadVariationWorker(Actor *p);
    virtual void init() ;

protected slots:
    virtual void sl_taskFinished();

protected:
    virtual Task* createReadTask(const QString &url);
}; // ReadVariationWorker

class ReadVariationWorkerFactory : public DomainFactory {
public:
    static const QString ACTOR_ID;

    ReadVariationWorkerFactory() : DomainFactory(ACTOR_ID) {}
    static void init();
    virtual Worker *createWorker(Actor *a);

}; // ReadVariationWorkerFactory

class ReadVariationTask : public Task {
    Q_OBJECT
public:
    ReadVariationTask(const QString &url, DbiDataStorage *storage);
    virtual void prepare();
    virtual void run();

    QList<QVariantMap> results;

private:
    QString url;
    DbiDataStorage *storage;
};

} // LocalWorkflow
} // U2

#endif // _READ_VARIATION_LIST_WORKER_
