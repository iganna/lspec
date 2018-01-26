#ifndef _SPB_DISTANCE_REPORT_WORKER_H_
#define _SPB_DISTANCE_REPORT_WORKER_H_

#include <U2Core/MAlignment.h>

#include <U2Lang/LocalDomain.h>
#include <U2Lang/WorkflowUtils.h>

#include "ComparingAlgorithm.h"

using namespace U2;
using namespace U2::LocalWorkflow;

namespace SPB {

class DistanceReportWorker : public BaseWorker {
    Q_OBJECT
public:
    DistanceReportWorker(Actor *a);

    virtual void init();
    virtual Task * tick();
    virtual void cleanup();

private slots:
    void sl_taskFinished();

private:
    IntegralBus *inPort;
    IntegralBus *outPort;
}; // DistanceReportWorker

class DistanceReportWorkerFactory : public DomainFactory {
public:
    DistanceReportWorkerFactory() : DomainFactory(ACTOR_ID) {}
    static void init();
    virtual Worker * createWorker(Actor *a);

private:
    static const QString ACTOR_ID;
}; // DistanceReportWorkerFactory

class DistanceReportPrompter : public PrompterBase<DistanceReportPrompter> {
public:
    DistanceReportPrompter(Actor *a = 0)
        : PrompterBase<DistanceReportPrompter>(a) {}

protected:
    QString composeRichDoc();
}; // DistanceReportPrompter

class DistanceReportTask : public Task {
public:
    DistanceReportTask(const MAlignment &mainMsa, MAlignment &alignedMsa, const QString &algoId, const QString &separator);
    virtual ~DistanceReportTask();

    virtual void run();

    const QString & getResult();

private:
    MAlignment mainMsa;
    MAlignment alignedMsa;
    QString result;
    QString separator;
    ComparingAlgorithm *algo;

private:
    void addRowInfo(const MAlignmentRow &alignedRow);
    double getSimilarity(const QByteArray &row1, const QByteArray &row2);
    void updateProgress(int idx);
};

}

#endif // _SPB_DISTANCE_REPORT_WORKER_H_
