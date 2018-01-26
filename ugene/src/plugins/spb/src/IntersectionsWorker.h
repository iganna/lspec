#ifndef _U2_INTERSECTIONS_WORKER_H_
#define _U2_INTERSECTIONS_WORKER_H_

#include <U2Lang/LocalDomain.h>
#include <U2Lang/WorkflowUtils.h>

using namespace U2;
using namespace U2::LocalWorkflow;

namespace U2 {
    class StreamSequenceReader;
}

namespace SPB {

class LoadSampleSubTask;
class SearchIntersectionsSubTask;
class TaskLogHelper;

class IntersectionsTask : public Task {
    Q_OBJECT
public:
    class Settings {
    public:
        QString sampleFile;
        QString samplesReportsDir;
        QStringList markersFiles;
        double accuracy;
        QString separator;
    };
    class Result {
    public:
        QString sampleSeqId;
        QStringList markerSeqIds;
    };
    IntersectionsTask(const Settings &settings);

    void prepare();
    QList<Task*> onSubTaskFinished(Task* subTask);
    void run();
    ReportResult report();

    QMap<QString, QString> getSampleReports();
    QString getMarkersReport();

    static QString seqId(const QString &seqName);

private:
    bool canCreateTask();
    bool hasMoreMarkers();
    Task * createSearchTask();
    QString headerString();
    QString markerString(SearchIntersectionsSubTask *task);
    QString sampleFileName(const QString &markerName);
    QString sampleFilePath(const QString &markerName);

private:
    Settings settings;
    QList<DNASequence> sampleSeqs;
    LoadSampleSubTask *sampleTask;
    QList<SearchIntersectionsSubTask*> searchTasks;
    int currentTaskCount;
    int maxTaskCount;
    QMap<QString, QString> sampleReports;
    QString markersReport;
    QString sep;
    QString sampleName;
    TaskLogHelper *logHelper;
};

class IntersectionsWorker : public BaseWorker {
    Q_OBJECT
public:
    IntersectionsWorker(Actor *p);

    virtual void init();
    virtual Task * tick();
    virtual void cleanup();

private slots:
    void sl_taskFinished();

private:
    bool willBeMoreMarkerFiles();
    bool hasMarkerFile();
    void getMarkerFile();
    IntersectionsTask::Settings settings();
    Task * createTask();
    void finish();
    void sendResult(IntersectionsTask *task);

private:
    QStringList markersFiles;

}; // IntersectionsWorker

class IntersectionsWorkerFactory : public DomainFactory {
public:
    IntersectionsWorkerFactory() : DomainFactory(ACTOR_ID) {}
    static void init();
    virtual Worker * createWorker(Actor *a);

private:
    static const QString ACTOR_ID;
}; // IntersectionsWorkerFactory

class IntersectionsPrompter : public PrompterBase<IntersectionsPrompter> {
    Q_OBJECT
public:
    IntersectionsPrompter(Actor *p = NULL) : PrompterBase<IntersectionsPrompter>(p) {}

protected:
    QString composeRichDoc();
}; // IntersectionsPrompter

} // SPB

#endif // _U2_INTERSECTIONS_WORKER_H_
