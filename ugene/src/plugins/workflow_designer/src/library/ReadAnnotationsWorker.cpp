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

#include <U2Core/AppContext.h>
#include <U2Core/AppResources.h>
#include <U2Core/AnnotationTableObject.h>
#include <U2Core/DocumentUtils.h>
#include <U2Core/IOAdapter.h>
#include <U2Core/IOAdapterUtils.h>
#include <U2Core/U2SafePoints.h>
#include <U2Core/ZlibAdapter.h>

#include <U2Designer/DelegateEditors.h>

#include <U2Gui/DialogUtils.h>
#include <U2Gui/GUIUtils.h>

#include <U2Lang/ActorPrototypeRegistry.h>
#include <U2Lang/BaseActorCategories.h>
#include <U2Lang/BaseAttributes.h>
#include <U2Lang/BasePorts.h>
#include <U2Lang/BaseSlots.h>
#include <U2Lang/BaseTypes.h>
#include <U2Lang/WorkflowEnv.h>

#include "DocActors.h"

#include "ReadAnnotationsWorker.h"

namespace U2 {
namespace LocalWorkflow {

const QString ReadAnnotationsWorkerFactory::ACTOR_ID("read-annotations");

/************************************************************************/
/* Worker */
/************************************************************************/
ReadAnnotationsWorker::ReadAnnotationsWorker(Actor *p)
: GenericDocReader(p)
{

}

void ReadAnnotationsWorker::init() {
    GenericDocReader::init();
    IntegralBus *outBus = dynamic_cast<IntegralBus*>(ch);
    assert(outBus);
    mtype = outBus->getBusType();
}

Task *ReadAnnotationsWorker::createReadTask(const QString &url) {
    Task *t = new ReadAnnotationsTask(url);
    connect(t, SIGNAL(si_stateChanged()), SLOT(sl_taskFinished()));
    return t;
}

void ReadAnnotationsWorker::sl_taskFinished() {
    ReadAnnotationsTask *t = qobject_cast<ReadAnnotationsTask*>(sender());
    if (!t->isFinished() || t->hasError()) {
        return;
    }

    foreach(const QVariantMap &m, t->takeResults()) {
        cache.append(Message(mtype, m));
    }
}

/************************************************************************/
/* Factory */
/************************************************************************/
void ReadAnnotationsWorkerFactory::init() {
    QList<PortDescriptor*> portDescs;
    {
        QMap<Descriptor, DataTypePtr> outTypeMap;
        outTypeMap[BaseSlots::ANNOTATION_TABLE_SLOT()] = BaseTypes::ANNOTATION_TABLE_TYPE();
        outTypeMap[BaseSlots::URL_SLOT()] = BaseTypes::STRING_TYPE();
        DataTypePtr outTypeSet(new MapDataType(BasePorts::OUT_ANNOTATIONS_PORT_ID(), outTypeMap));

        Descriptor outDesc(BasePorts::OUT_ANNOTATIONS_PORT_ID(),
            ReadAnnotationsWorker::tr("Annotations"),
            ReadAnnotationsWorker::tr("Annotations."));

        portDescs << new PortDescriptor(outDesc, outTypeSet, false, true);
    }

    QList<Attribute*> attrs;
    {
        attrs << new Attribute(BaseAttributes::URL_IN_ATTRIBUTE(), BaseTypes::STRING_TYPE(), true);
    }

    QMap<QString, PropertyDelegate*> delegates;
    {
        delegates[BaseAttributes::URL_IN_ATTRIBUTE().getId()] = new URLDelegate(DialogUtils::prepareDocumentsFileFilter(true), QString(), true);
    }

    Descriptor protoDesc(ReadAnnotationsWorkerFactory::ACTOR_ID,
        ReadAnnotationsWorker::tr("Read Annotations"),
        ReadAnnotationsWorker::tr("Reads annotations from files"));

    ActorPrototype *proto = new IntegralBusActorPrototype(protoDesc, portDescs, attrs);
    proto->setEditor(new DelegateEditor(delegates));
    proto->setPrompter(new ReadDocPrompter(ReadAnnotationsWorker::tr("Reads annotations from <u>%1</u>.")));
    if (AppContext::isGUIMode()) {
        proto->setIcon(GUIUtils::createRoundIcon(QColor(85,85,255), 22));
    }

    WorkflowEnv::getProtoRegistry()->registerProto(BaseActorCategories::CATEGORY_DATASRC(), proto);
    WorkflowEnv::getDomainRegistry()->getById(LocalDomainFactory::ID)->registerEntry(new ReadAnnotationsWorkerFactory());
}

Worker *ReadAnnotationsWorkerFactory::createWorker(Actor *a) {
    return new ReadAnnotationsWorker(a);
}

/************************************************************************/
/* Task */
/************************************************************************/
ReadAnnotationsTask::ReadAnnotationsTask(const QString &_url)
: Task(tr("Read annotations from %1").arg(_url), TaskFlag_None), url(_url)
{

}

void ReadAnnotationsTask::prepare() {
    int memUseMB = 0;
    QFileInfo file(url);
    memUseMB = file.size() / (1024*1024) + 1;
    IOAdapterFactory *iof = AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(IOAdapterUtils::url2io(url));
    if (BaseIOAdapters::GZIPPED_LOCAL_FILE == iof->getAdapterId()) {
        memUseMB = ZlibAdapter::getUncompressedFileSizeInBytes(url) / (1024*1024) + 1;
    } else if (BaseIOAdapters::GZIPPED_HTTP_FILE == iof->getAdapterId()) {
        memUseMB *= 2.5; //Need to calculate compress level
    }
    coreLog.trace(QString("Load annotations: Memory resource %1").arg(memUseMB));

    if (memUseMB > 0) {
        addTaskResource(TaskResourceUsage(RESOURCE_MEMORY, memUseMB, false));
    }
}

void ReadAnnotationsTask::run() {
    QFileInfo fi(url);
    CHECK_EXT(fi.exists(), stateInfo.setError(tr("File '%1' does not exist").arg(url)), );

    DocumentFormat *format = NULL;
    QList<DocumentFormat*> fs = DocumentUtils::toFormats(DocumentUtils::detectFormat(url));
    foreach(DocumentFormat *f, fs) {
        if (f->getSupportedObjectTypes().contains(GObjectTypes::ANNOTATION_TABLE)) {
            format = f;
            break;
        }
    }
    CHECK_EXT(NULL != format, stateInfo.setError(tr("Unsupported document format")), );

    ioLog.info(tr("Reading annotations from %1 [%2]").arg(url).arg(format->getFormatName()));
    IOAdapterFactory *iof = AppContext::getIOAdapterRegistry()->
        getIOAdapterFactoryById(IOAdapterUtils::url2io(url));
    std::auto_ptr<Document> doc(format->loadDocument(iof, url, QVariantMap(), stateInfo));
    CHECK_OP(stateInfo, );

    QList<GObject*> annsObjList = doc->findGObjectByType(GObjectTypes::ANNOTATION_TABLE);
    foreach(GObject *go, annsObjList) {
        AnnotationTableObject *annsObj = dynamic_cast<AnnotationTableObject*>(go);
        CHECK_EXT(NULL != annsObj, stateInfo.setError("NULL annotations object"), );

        QList<SharedAnnotationData> dataList;
        foreach(Annotation *a, annsObj->getAnnotations()) {
            dataList << a->data();
        }

        QVariantMap m;
        m[BaseSlots::URL_SLOT().getId()] = url;
        m[BaseSlots::ANNOTATION_TABLE_SLOT().getId()] = qVariantFromValue<QList<SharedAnnotationData> >(dataList);
        results.append(m);
    }
}

QList<QVariantMap> ReadAnnotationsTask::takeResults() {
    QList<QVariantMap> ret = results;
    results.clear();

    return ret;
}

void ReadAnnotationsTask::cleanup() {
    results.clear();
}

} // LocalWorkflow
} // U2
