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

#ifndef _U2_WORKFLOW_READ_SEQ_WORKER_H_
#define _U2_WORKFLOW_READ_SEQ_WORKER_H_

#include <U2Lang/LocalDomain.h>
#include <U2Core/DNASequenceObject.h>
#include <U2Core/DNASequence.h>
#include <U2Core/DocumentModel.h>
#include <U2Core/MAlignment.h>

namespace U2 {

class FilesIterator;

namespace LocalWorkflow {

class GenericDocReader : public BaseWorker {
    Q_OBJECT
public:
    GenericDocReader(Actor *a) : BaseWorker(a), ch(NULL), files(NULL) {}
    virtual ~GenericDocReader();

    virtual void init();
    virtual Task *tick();
    virtual bool isDone();
    virtual void cleanup() {}

protected slots:
    virtual void sl_taskFinished() = 0;

protected:
    virtual Task *createReadTask(const QString &url) = 0;

    CommunicationChannel *ch;
    QList<Message> cache;
    DataTypePtr mtype;
    FilesIterator *files;
};

/************************************************************************/
/* Inherited classes */
/************************************************************************/
class DNASelector {
public:
    bool matches(const DNASequence &);
    bool objectMatches(const U2SequenceObject *);
    QString accExpr;
};

class LoadSeqTask : public Task {
    Q_OBJECT
public:
    LoadSeqTask(QString url, const QVariantMap &cfg, DNASelector *sel, DbiDataStorage *storage)
        : Task(tr("Read sequences from %1").arg(url), TaskFlag_None),
        url(url), selector(sel), cfg(cfg), storage(storage), format(NULL) {}
    virtual void prepare();
    virtual void run();

    QString url;
    DNASelector *selector;
    QVariantMap cfg;
    QList<QVariantMap> results;
    DbiDataStorage *storage;
    DocumentFormat *format;
};

class LoadMSATask : public Task {
    Q_OBJECT
public:
    LoadMSATask(QString url) : Task(tr("Read MSA from %1").arg(url), TaskFlag_None), url(url) {}
    virtual void prepare();
    virtual void run();

    QString url;
    QList<MAlignment> results;
};

class GenericMSAReader : public GenericDocReader {
    Q_OBJECT
public:
    GenericMSAReader(Actor *a) : GenericDocReader(a) {}
    virtual void init() ;
    virtual void cleanup() {}

protected slots:
    virtual void sl_taskFinished();

protected:
    virtual Task *createReadTask(const QString &url) {return new LoadMSATask(url);}
};

class GenericSeqReader : public GenericDocReader {
    Q_OBJECT
public:
    GenericSeqReader(Actor *a) : GenericDocReader(a){}
    virtual void init() ;

protected slots:
    virtual void sl_taskFinished();

protected:
    virtual Task *createReadTask(const QString &url) {
        return new LoadSeqTask(url, cfg, &selector, context->getDataStorage());
    }
    QVariantMap cfg;
    DNASelector selector;
};


} // Workflow namespace
} // U2 namespace

#endif
