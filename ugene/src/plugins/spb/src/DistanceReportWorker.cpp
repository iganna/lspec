#include <U2Designer/DelegateEditors.h>

#include <U2Lang/ActorPrototypeRegistry.h>
#include <U2Lang/BasePorts.h>
#include <U2Lang/BaseSlots.h>
#include <U2Lang/BaseTypes.h>
#include <U2Lang/WorkflowEnv.h>

#include "SpbPlugin.h"

#include "DistanceReportWorker.h"

namespace SPB {

const QString DistanceReportWorkerFactory::ACTOR_ID("distance-report");

static const QString IN_PROFILES_PORT_ID("in-profiles");

static const QString MAIN_MSA_SLOT_ID("main-msa");
static const QString ALIGNED_MSA_SLOT_ID("aligned-msa");

static const QString COMPARING_ALGO_ATTR_ID("comparing-algorithm");
static const QString SEPARATOR_ATTR_ID("separator");

/************************************************************************/
/* Task */
/************************************************************************/
DistanceReportWorker::DistanceReportWorker(Actor *a)
: BaseWorker(a), inPort(NULL), outPort(NULL)
{

}

void DistanceReportWorker::init() {
    inPort = ports[IN_PROFILES_PORT_ID];
    outPort = ports[BasePorts::OUT_TEXT_PORT_ID()];
}

Task * DistanceReportWorker::tick() {
    if (inPort->hasMessage()) {
        Message m = getMessageAndSetupScriptValues(inPort);
        QVariantMap data = m.getData().toMap();
        MAlignment mainMsa = data.value(MAIN_MSA_SLOT_ID).value<MAlignment>();
        MAlignment alignedMsa = data.value(ALIGNED_MSA_SLOT_ID).value<MAlignment>();
        QString algoId = actor->getParameter(COMPARING_ALGO_ATTR_ID)->
                getAttributeValue<QString>(context);
        QString separator = actor->getParameter(SEPARATOR_ATTR_ID)->
                getAttributeValue<QString>(context);

        Task *t = new DistanceReportTask(mainMsa, alignedMsa, algoId, separator);
        connect(t, SIGNAL(si_stateChanged()), SLOT(sl_taskFinished()));
        return t;
    } else if (inPort->isEnded()) {
        setDone();
        outPort->setEnded();
    }
    return NULL;
}

void DistanceReportWorker::cleanup() {

}

void DistanceReportWorker::sl_taskFinished() {
    DistanceReportTask *t = dynamic_cast<DistanceReportTask*>(sender());
    if (!t->isFinished() || t->hasError()) {
        return;
    }

    QVariantMap data;
    data[BaseSlots::TEXT_SLOT().getId()] = t->getResult();
    outPort->put(Message(outPort->getBusType(), data));
}

/************************************************************************/
/* Task */
/************************************************************************/
DistanceReportTask::DistanceReportTask(const MAlignment &_mainMsa, MAlignment &_alignedMsa,
                                       const QString &algoId, const QString &_separator)
: Task("Distance report", TaskFlag_None), mainMsa(_mainMsa), alignedMsa(_alignedMsa),
separator(_separator)
{
    algo = ComparingAlgorithmFactory::createAlgorithm(algoId);
}

DistanceReportTask::~DistanceReportTask() {
    delete algo;
}

void DistanceReportTask::run() {
    foreach (const MAlignmentRow &row, mainMsa.getRows()) {
        result += separator + row.getName();
    }
    result += "\n";

    int rowIdx = 0;
    foreach (const MAlignmentRow &row, alignedMsa.getRows()) {
        addRowInfo(row);
        updateProgress(rowIdx);
        rowIdx++;
    }
}

inline void DistanceReportTask::addRowInfo(const MAlignmentRow &alignedRow) {
    result += alignedRow.getName();

    foreach (const MAlignmentRow &row, mainMsa.getRows()) {
        double simD = getSimilarity(row.getCore(), alignedRow.getCore());
        result += separator + QByteArray::number(simD);
    }
    result += "\n";
}

inline double DistanceReportTask::getSimilarity(const QByteArray &row1, const QByteArray &row2) {
    return algo->compare(row1, row2);
}

inline void DistanceReportTask::updateProgress(int idx) {
    stateInfo.setProgress(100*idx/alignedMsa.getNumRows());
}

const QString & DistanceReportTask::getResult() {
    return result;
}

/************************************************************************/
/* Factory */
/************************************************************************/
void DistanceReportWorkerFactory::init() {
    QList<PortDescriptor*> portDescs;
    {
        Descriptor mainMsaD(MAIN_MSA_SLOT_ID,
            DistanceReportWorker::tr("Main alignment"),
            DistanceReportWorker::tr("Main alignment."));
        Descriptor alignedMsaD(ALIGNED_MSA_SLOT_ID,
            DistanceReportWorker::tr("Alignment for report"),
            DistanceReportWorker::tr("Alignment for report."));

        QMap<Descriptor, DataTypePtr> inMap;
        inMap[mainMsaD] = BaseTypes::MULTIPLE_ALIGNMENT_TYPE();
        inMap[alignedMsaD] = BaseTypes::MULTIPLE_ALIGNMENT_TYPE();
        portDescs << new PortDescriptor(IN_PROFILES_PORT_ID, DataTypePtr(new MapDataType("in.profiles", inMap)), true);

        QMap<Descriptor, DataTypePtr> outMap;
        outMap[BaseSlots::TEXT_SLOT()] = BaseTypes::STRING_TYPE();
        portDescs << new PortDescriptor(BasePorts::OUT_TEXT_PORT_ID(), DataTypePtr(new MapDataType("out.report", outMap)), false, true);
    }

    QList<Attribute*> attrs;
    {
        Descriptor cAlgoD(COMPARING_ALGO_ATTR_ID,
            DistanceReportWorker::tr("Sequence comparing algorithm"),
            DistanceReportWorker::tr("Sequence comparing algorithm."));
        Descriptor sepD(SEPARATOR_ATTR_ID,
            DistanceReportWorker::tr("CSV separator"),
            DistanceReportWorker::tr("CSV separator."));
        attrs << new Attribute(cAlgoD, BaseTypes::STRING_TYPE(), true, ComparingAlgorithmFactory::DEFAULT);
        attrs << new Attribute(sepD, BaseTypes::STRING_TYPE(), true, ",");
    }

    QMap<QString, PropertyDelegate*> delegates;
    {
        QVariantMap cAlgos;
        cAlgos[ComparingAlgorithmFactory::DEFAULT] = ComparingAlgorithmFactory::DEFAULT;
        delegates[COMPARING_ALGO_ATTR_ID] = new ComboBoxDelegate(cAlgos);
    }

    Descriptor protoD(ACTOR_ID,
        DistanceReportWorker::tr("Generate Distance Report"),
        DistanceReportWorker::tr("Generate Distance Report."));

    ActorPrototype *proto = new IntegralBusActorPrototype(protoD, portDescs, attrs);
    proto->setEditor(new DelegateEditor(delegates));
    proto->setPrompter(new DistanceReportPrompter());

    WorkflowEnv::getProtoRegistry()->registerProto(Constraints::WORKFLOW_CATEGORY, proto);
    DomainFactory *localDomain = WorkflowEnv::getDomainRegistry()->getById(LocalDomainFactory::ID);
    localDomain->registerEntry(new DistanceReportWorkerFactory());
}

Worker * DistanceReportWorkerFactory::createWorker(Actor *a) {
    return new DistanceReportWorker(a);
}

/************************************************************************/
/* Prompter */
/************************************************************************/
QString DistanceReportPrompter::composeRichDoc() {
    return "Creates alignment distance report.";
}

} // SPB
