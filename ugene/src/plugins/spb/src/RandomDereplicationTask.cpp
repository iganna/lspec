#include <U2Core/AppContext.h>
#include <U2Core/Task.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/U2SafePoints.h>

#include "ComparingAlgorithm.h"
#include "IntersectionsWorker.h"

#include "RandomDereplicationTask.h"

namespace SPB {
static const int initProgressStep = 10;
RandomDereplicationTask::RandomDereplicationTask(const DereplicationData &data, const QString &reportUrl)
: DereplicationTask("Random dereplication", data, reportUrl), initialSeqCount(0), initTask(NULL),
comparer(NULL), sequences(NULL), reportIO(NULL)
{
    iterationIdx = 0;
    nextInitProgress = initProgressStep;
}

RandomDereplicationTask::~RandomDereplicationTask() {
    delete comparer;
    delete sequences;
    delete reportIO;
}

void RandomDereplicationTask::prepare() {
    openReportAdapter();
    CHECK_OP(stateInfo, );
    writeReportHeader();

    initTask = new InitialSubTask(data);
    addSubTask(initTask);
}

QList<Task*> RandomDereplicationTask::onSubTaskFinished(Task* subTask) {
    QList<Task*> tasks;
    if (initTask == subTask) {
        comparer = initTask->takeComparer();
        sequences = initTask->takeSequences();
    }
    return tasks;
}

void RandomDereplicationTask::openReportAdapter() {
    IOAdapterFactory *iof = AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(BaseIOAdapters::LOCAL_FILE);
    SAFE_POINT_EXT(NULL != iof, setError("NULL IO factory"), );
    reportIO = iof->createIOAdapter();
    SAFE_POINT_EXT(NULL != reportIO, setError("NULL IO adapter"), );
    bool opened = reportIO->open(reportUrl, IOAdapterMode_Write);
    if (!opened) {
        setError(tr("Can not open report file"));
    }
}

void RandomDereplicationTask::writeReportHeader() {
    QByteArray str = "# removed_seq [TAB] accuracy(%) [TAB] similar_seq\n";
    reportIO->writeBlock(str);
}

void RandomDereplicationTask::run() {
    initialSeqCount = data.getSeqs().size();

    QMap<int, SharedDbiDataHandler> idMap;
    for (int i=0; i<initialSeqCount; i++) {
        idMap[i] = data.getSeqs().at(i);
    }

    while (!sequences->isEmpty()) {
        int leaderIdx = sequences->getRandomSeqNum();
        QString leaderSeqId = IntersectionsTask::seqId(sequences->getName(leaderIdx));
        result << idMap.take(leaderIdx);
        QList<BaseFullIndexComparer::Similar> result = comparer->removeSimilars(leaderIdx);
        writeReport(leaderSeqId, result);
        if (updateProgress()) {
            return;
        }
    }
}

void RandomDereplicationTask::writeReport(const QString &seqId, const QList<BaseFullIndexComparer::Similar> &similars) {
    foreach (const BaseFullIndexComparer::Similar &s, similars) {
        QString str = IntersectionsTask::seqId(s.seqName) + "\t" + QString::number(s.accuracy) + "\t" + seqId + "\n";
        reportIO->writeBlock(str.toLatin1());
    }
}

static const int UPDATE_STEP = 50;
inline bool RandomDereplicationTask::updateProgress() {
    iterationIdx++;
    if (iterationIdx > UPDATE_STEP) {
        iterationIdx = 0;
        double p = double(initialSeqCount - sequences->getSeqCount()) / initialSeqCount;
        int progress = int(100 * p);
        taskLog.details(tr("Dereplication progress: %1 %").arg(progress));
        stateInfo.setProgress(progress);

        if (nextInitProgress < progress) {
            nextInitProgress += initProgressStep;
            comparer->initialize();
        }
        return stateInfo.isCanceled();
    }
    return false;
}

/************************************************************************/
/* InitialSubTask */
/************************************************************************/
InitialSubTask::InitialSubTask(DereplicationData &_data)
: Task("Init random dereplication task", TaskFlag_None), data(_data), comparer(NULL), sequences(NULL)
{
    setSubtaskProgressWeight(0.0);
}

InitialSubTask::~InitialSubTask() {
    delete comparer;
    delete sequences;
}

void InitialSubTask::run() {
    sequences = new SequencesStorage(data.getSeqs());
    sequences->initialize(data.getStorage(), stateInfo);
    CHECK_OP(stateInfo, );

    comparer = new FullIndexComparer(data.getAccuracy(), *sequences, data.getComparingAlgoId());
    comparer->initialize();
}

FullIndexComparer * InitialSubTask::takeComparer() {
    FullIndexComparer *ret = comparer;
    comparer = NULL;
    return ret;
}

SequencesStorage * InitialSubTask::takeSequences() {
    SequencesStorage *ret = sequences;
    sequences = NULL;
    return ret;
}

} // SPB
