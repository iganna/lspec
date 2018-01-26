#include <U2Core/AppContext.h>
#include <U2Core/AppResources.h>
#include <U2Core/AppSettings.h>
#include <U2Core/BaseDocumentFormats.h>
#include <U2Core/Timer.h>
#include <U2Core/U2SafePoints.h>

#include <U2Designer/DelegateEditors.h>

#include <U2Formats/StreamSequenceReader.h>

#include <U2Gui/DialogUtils.h>

#include <U2Lang/ActorPrototypeRegistry.h>
#include <U2Lang/BaseActorCategories.h>
#include <U2Lang/BaseAttributes.h>
#include <U2Lang/BasePorts.h>
#include <U2Lang/BaseSlots.h>
#include <U2Lang/BaseTypes.h>
#include <U2Lang/WorkflowEnv.h>

#include "FullIndexComparer.h"
#include "SpbPlugin.h"

#include "IntersectionsWorker.h"

namespace SPB {

const QString IntersectionsWorkerFactory::ACTOR_ID("search-intersections");

static const QString IN_PORT("in");
static const QString OUT_PORT("out");

static const QString MARKER_FILE_SLOT("marker-file");

static const QString SAMPLE_REPORT_URL_SLOT("sample-report-url");
static const QString SAMPLE_REPORT_SLOT("sample-report");
static const QString MARKERS_REPORT_SLOT("markers-report");

static const QString SAMPLE_FILE_ATTR("sample-file");
static const QString SAMPLE_REPORTS_DIR_ATTR("sample-reports");
static const QString ACCURACY_ATTR("comparing-accuracy");
static const QString SEPARATOR_ATTR("separator");

/*******************************
 * IntersectionsWorker
 *******************************/
IntersectionsWorker::IntersectionsWorker(Actor *p)
: BaseWorker(p)
{

}

void IntersectionsWorker::init() {

}

Task * IntersectionsWorker::tick() {
    while (hasMarkerFile()) {
        getMarkerFile();
    }
    if (willBeMoreMarkerFiles()) {
        return NULL;
    }

    return createTask();
}

bool IntersectionsWorker::willBeMoreMarkerFiles() {
    return !ports[IN_PORT]->isEnded();
}

bool IntersectionsWorker::hasMarkerFile() {
    return ports[IN_PORT]->hasMessage();
}

void IntersectionsWorker::getMarkerFile() {
    Message m = getMessageAndSetupScriptValues(ports[IN_PORT]);
    QVariantMap data = m.getData().toMap();
    SAFE_POINT(data.contains(MARKER_FILE_SLOT), "No marker slot data", );

    markersFiles << data[MARKER_FILE_SLOT].toString();
}

IntersectionsTask::Settings IntersectionsWorker::settings() {
    IntersectionsTask::Settings s;
    s.separator = actor->getParameter(SEPARATOR_ATTR)->getAttributeValue<QString>(context);
    s.accuracy = actor->getParameter(ACCURACY_ATTR)->getAttributeValue<double>(context);
    s.sampleFile = actor->getParameter(SAMPLE_FILE_ATTR)->getAttributeValue<QString>(context);
    s.samplesReportsDir = actor->getParameter(SAMPLE_REPORTS_DIR_ATTR)->getAttributeValue<QString>(context);
    s.markersFiles = markersFiles;
    return s;
}

Task * IntersectionsWorker::createTask() {
    Task *t = new IntersectionsTask(settings());
    connect(t, SIGNAL(si_stateChanged()), SLOT(sl_taskFinished()));
    return t;
}

void IntersectionsWorker::finish() {
    setDone();
    ports[OUT_PORT]->setEnded();
}

void IntersectionsWorker::sendResult(IntersectionsTask *task) {
    foreach (const QString &url, task->getSampleReports().keys()) {
        QVariantMap data;
        data[SAMPLE_REPORT_URL_SLOT] = url;
        data[SAMPLE_REPORT_SLOT] = task->getSampleReports()[url];
        ports[OUT_PORT]->put(Message(ports[OUT_PORT]->getBusType(), data));
    }

    QVariantMap data;
    data[MARKERS_REPORT_SLOT] = task->getMarkersReport();
    ports[OUT_PORT]->put(Message(ports[OUT_PORT]->getBusType(), data));
}

void IntersectionsWorker::sl_taskFinished() {
    IntersectionsTask *t = dynamic_cast<IntersectionsTask*>(sender());
    CHECK(NULL != t, );
    CHECK(t->isFinished(), );

    sendResult(t);
    finish();
}

void IntersectionsWorker::cleanup() {

}

/*******************************
 * IntersectionsWorkerFactory
 *******************************/
void IntersectionsWorkerFactory::init() {
    QList<PortDescriptor*> ports;
    {
        Descriptor inPort(IN_PORT, "Input FASTA files", "Input FASTA files");
        QMap<Descriptor, DataTypePtr> inMap;
        inMap[Descriptor(MARKER_FILE_SLOT, "Marker FASTA files", "Marker FASTA files")] = BaseTypes::STRING_TYPE();
        ports << new PortDescriptor(inPort, DataTypePtr(new MapDataType("in-files", inMap)), true /*input*/);

        Descriptor outPort(OUT_PORT, "Output reports", "Output reports");
        QMap<Descriptor, DataTypePtr> outMap;
        outMap[Descriptor(SAMPLE_REPORT_URL_SLOT, "Sample report url", "Url for report about sample")] = BaseTypes::STRING_TYPE();
        outMap[Descriptor(SAMPLE_REPORT_SLOT, "Report about sample", "Report about sample")] = BaseTypes::STRING_TYPE();
        outMap[Descriptor(MARKERS_REPORT_SLOT, "Report about markers", "Report about markers")] = BaseTypes::STRING_TYPE();
        ports << new PortDescriptor(outPort, DataTypePtr(new MapDataType("out-reports", outMap)), false /*output*/, true /*multi*/);
    }

    QList<Attribute*> attrs;
    {
        Descriptor sampleD(SAMPLE_FILE_ATTR, "Sample FASTA file", "Sample FASTA file");
        attrs << new Attribute(sampleD, BaseTypes::STRING_TYPE(), true, "");

        Descriptor sampleDirD(SAMPLE_REPORTS_DIR_ATTR, "Sample reports directory", "Sample reports directory");
        attrs << new Attribute(sampleDirD, BaseTypes::STRING_TYPE(), true, "");

        Descriptor accD(ACCURACY_ATTR, "Comparing accuracy (%)", "Comparing accuracy (%)");
        attrs << new Attribute(accD, BaseTypes::NUM_TYPE(), true, 99);

        Descriptor sepD(SEPARATOR_ATTR, "CSV separator", "CSV separator");
        attrs << new Attribute(sepD, BaseTypes::STRING_TYPE(), true, ";");
    }

    QMap<QString, PropertyDelegate*> delegates;
    {
        QVariantMap percMap;
        percMap["minimum"] = QVariant(0.0); 
        percMap["maximum"] = QVariant(100.0); 
        percMap["suffix"] = "%";
        delegates[ACCURACY_ATTR] = new DoubleSpinBoxDelegate(percMap);

        delegates[SAMPLE_FILE_ATTR] = new URLDelegate(DialogUtils::prepareDocumentsFileFilter(BaseDocumentFormats::FASTA, true),
            "", false, false, false, NULL, BaseDocumentFormats::FASTA);
        delegates[SAMPLE_REPORTS_DIR_ATTR] = new URLDelegate("", "", false, true);
    }

    Descriptor protoD(ACTOR_ID, "Search for intersections", "Search for intersections");
    ActorPrototype *proto = new IntegralBusActorPrototype(protoD, ports, attrs);
    proto->setEditor(new DelegateEditor(delegates));
    proto->setPrompter(new IntersectionsPrompter());

    WorkflowEnv::getProtoRegistry()->registerProto(Constraints::WORKFLOW_CATEGORY, proto);
    DomainFactory *localDomain = WorkflowEnv::getDomainRegistry()->getById(LocalDomainFactory::ID);
    localDomain->registerEntry(new IntersectionsWorkerFactory());
}

Worker *IntersectionsWorkerFactory::createWorker(Actor *a) {
    return new IntersectionsWorker(a);
}

/*******************************
 * IntersectionsPrompter
 *******************************/
QString IntersectionsPrompter::composeRichDoc() {
    return "";
}

/************************************************************************/
/*  */
/************************************************************************/
class TaskLogHelper {
public:
    TaskLogHelper(const QString &startMessage, const QString &finishMessage)
        : fm(finishMessage)
    {
        if (!startMessage.isEmpty()) {
            taskLog.info(startMessage);
        }
        startTime = GTimer::currentTimeMicros();
    }
    ~TaskLogHelper() {
        qint64 finishTime = GTimer::currentTimeMicros();
        int msecs = GTimer::millisBetween(startTime, finishTime);
        QString time = QTime().addMSecs(msecs).toString("h:mm:ss");

        taskLog.info(fm + ". Elapsed time: " + time);
    }

private:
    QString fm;
    qint64 startTime;
};

/************************************************************************/
/* LoadSampleSubTask */
/************************************************************************/
class LoadSampleSubTask : public Task {
public:
    LoadSampleSubTask(const QString &sampleFile)
        : Task("Load sample", TaskFlag_None), file(sampleFile)
    {

    }

    void run() {
        TaskLogHelper helper("Started reading sample sequences", "Finished reading sample sequences");
        QScopedPointer<StreamSequenceReader> reader(getReader());
        CHECK_OP(stateInfo, );

        while (reader->hasNext()) {
            if (reader->hasError()) {
                stateInfo.setError(reader->getErrorMessage());
                return;
            }
            DNASequence *seq = reader->getNextSequenceObject();
            result << DNASequence(*seq);
        }

        if (reader->hasError()) {
            stateInfo.setError(reader->getErrorMessage());
        }
    }

    QList<DNASequence> takeResult() {
        QList<DNASequence> ret = result;
        result.clear();
        return ret;
    }

private:
    StreamSequenceReader * getReader() {
        StreamSequenceReader * reader = new StreamSequenceReader();
        QList<GUrl> urls;
        urls << file;
        bool inited = reader->init(urls);
        if (!inited) {
            stateInfo.setError(reader->getErrorMessage());
        }
        return reader;
    }

private:
    QString file;
    QList<DNASequence> result;
};

/************************************************************************/
/* SearchIntersectionsSubTask */
/************************************************************************/
class SearchIntersectionsSubTask : public Task {
public:
    SearchIntersectionsSubTask(const QString &_markerFile, double _accuracy, const QList<DNASequence> &_sampleSeqs)
        : Task("Search intersections in marker", TaskFlag_None), markerFile(_markerFile),
        accuracy(_accuracy), sampleSeqs(_sampleSeqs), seqCount(0)
    {

    }

    void run() {
        TaskLogHelper helper("Started searching intersections for marker: " + markerName(),
            "Finished searching intersections for marker: " + markerName());
        FastaSequenceStorage storage(markerFile);
        storage.initialize(stateInfo);
        CHECK_OP(stateInfo, );

        BaseFullIndexComparer comparer(accuracy, storage);
        comparer.initialize();

        BaseSequencesStorage &markerSeqs = comparer.getSequences();
        foreach (const DNASequence &sampleSeq, sampleSeqs) {
            QList<int> sims = comparer.getSimilars(sampleSeq.seq);
            if (sims.isEmpty()) {
                continue;
            }

            QStringList markerSeqIds;
            foreach (int seqNum, sims) {
                const DNASequence &markerSeq = markerSeqs.getSeqMap()[seqNum];
                markerSeqIds << IntersectionsTask::seqId(markerSeq.getName());
            }
            allMarkerIds.unite(markerSeqIds.toSet());
            matchedMarkerSeqIds << markerSeqIds;
            sampleSeqIds << IntersectionsTask::seqId(sampleSeq.getName());
        }

        seqCount = comparer.getSequences().getInitialSeqCount();
    }

    QString markerName() {
        return QFileInfo(markerFile).baseName();
    }

    int markerSeqCount() {
        return seqCount;
    }

    int markerMatchCount() {
        return allMarkerIds.size();
    }

    int sampleMatchCount() {
        return sampleSeqIds.size();
    }

    const QSet<QString> & getMatchedMarkerIds() {
        return allMarkerIds;
    }

    const QList<QStringList> & getMatchedMarkerSeqIds() {
        return matchedMarkerSeqIds;
    }

    const QStringList & getSampleSeqIds() {
        return sampleSeqIds;
    }

private:
    QString markerFile;
    double accuracy;
    const QList<DNASequence> &sampleSeqs;
    QSet<QString> allMarkerIds;
    QList<QStringList> matchedMarkerSeqIds;
    QStringList sampleSeqIds;
    int seqCount;
};

/************************************************************************/
/* IntersectionsTask */
/************************************************************************/
IntersectionsTask::IntersectionsTask(const IntersectionsTask::Settings &_settings)
: Task("Search sequence intersections", TaskFlags_FOSCOE), settings(_settings),
sampleTask(NULL), currentTaskCount(0), maxTaskCount(0), logHelper(NULL)
{
    sep = settings.separator;
    sampleName = QFileInfo(settings.sampleFile).baseName();
}

void IntersectionsTask::prepare() {
    logHelper = new TaskLogHelper("", "Intersection task is finished");
    sampleTask = new LoadSampleSubTask(settings.sampleFile);
    addSubTask(sampleTask);
    maxTaskCount = AppContext::getAppSettings()->getAppResourcePool()->getIdealThreadCount();
    setMaxParallelSubtasks(maxTaskCount);
}

QList<Task*> IntersectionsTask::onSubTaskFinished(Task* subTask) {
    QList<Task*> result;
    if (sampleTask == subTask) {
        sampleSeqs = sampleTask->takeResult();
    } else {
        currentTaskCount--;
    }
    while (canCreateTask() && hasMoreMarkers()) {
        result << createSearchTask();
    }
    return result;
}

bool IntersectionsTask::canCreateTask() {
    return (currentTaskCount < maxTaskCount);
}

bool IntersectionsTask::hasMoreMarkers() {
    return (searchTasks.size() < settings.markersFiles.size());
}

Task * IntersectionsTask::createSearchTask() {
    int currentMarker = searchTasks.size();
    SearchIntersectionsSubTask *t = new SearchIntersectionsSubTask(settings.markersFiles[currentMarker], settings.accuracy, sampleSeqs);
    searchTasks << t;
    currentTaskCount++;
    return t;
}

QString IntersectionsTask::headerString() {
    QString sampleName = QFileInfo(settings.sampleFile).baseName();
    return sampleName + sep + QString::number(sampleSeqs.size()) + "\n"
        "Marker" + sep +
        "Sequence count" + sep +
        "Sample match count (abs, %)" + sep +
        "Marker match count (abs, %)" + sep +
        "File\n";
}

QString IntersectionsTask::markerString(SearchIntersectionsSubTask *task) {
    double sampleMatchCountPers = 100.0 * task->sampleMatchCount() / sampleSeqs.size();
    double markerMatchCountPers = 100.0 * task->markerMatchCount() / task->markerSeqCount();

    return
        task->markerName() + sep +
        QString::number(task->markerSeqCount()) + sep +
        "(" + QString::number(task->sampleMatchCount()) + ", " + QString::number(sampleMatchCountPers) + ")" + sep +
        "(" + QString::number(task->markerMatchCount()) + ", " + QString::number(markerMatchCountPers) + ")" + sep +
        sampleFileName(task->markerName()) + "\n";
}

QString IntersectionsTask::sampleFileName(const QString &markerName) {
    return sampleName + "_" + markerName + "_" + QString::number(settings.accuracy) + ".txt";
}

QString IntersectionsTask::sampleFilePath(const QString &markerName) {
    return settings.samplesReportsDir + "/" + sampleFileName(markerName);
}

void IntersectionsTask::run() {
    TaskLogHelper helper("Started generating reports", "Finished generating reports");
    SAFE_POINT(searchTasks.size() == settings.markersFiles.size(), "Wrong subtask count", );

    // sample report
    foreach (SearchIntersectionsSubTask *t, searchTasks) {
        QString sampleReport;
        QList<QStringList>::ConstIterator mi = t->getMatchedMarkerSeqIds().constBegin();
        QStringList::ConstIterator si = t->getSampleSeqIds().constBegin();
        for (; mi != t->getMatchedMarkerSeqIds().constEnd() && si != t->getSampleSeqIds().constEnd(); mi++, si++) {
            const QStringList &matched = *mi;
            const QString &sampleSeqId = *si;

            foreach (const QString &markerSeqId, matched) {
                sampleReport += sampleSeqId + sep + markerSeqId + "\n";
            }
        }
        sampleReports[sampleFilePath(t->markerName())] = sampleReport;
    }

    // markers report
    markersReport += headerString();
    foreach (SearchIntersectionsSubTask *t, searchTasks) {
        markersReport += markerString(t);
    }
}

QMap<QString, QString> IntersectionsTask::getSampleReports() {
    return sampleReports;
}

QString IntersectionsTask::getMarkersReport() {
    return markersReport;
}

Task::ReportResult IntersectionsTask::report() {
    delete logHelper;
    return ReportResult_Finished;
}

QString IntersectionsTask::seqId(const QString &seqName) {
    return seqName.left(14);
}

} // SPB
