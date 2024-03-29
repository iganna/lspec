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

#include "../bowtie/BowtieSupport.h"
#include "TopHatSupportTask.h"
#include "TopHatWorker.h"

#include <U2Core/AppContext.h>
#include <U2Core/AppSettings.h>
#include <U2Core/DNASequenceObject.h>
#include <U2Core/L10n.h>
#include <U2Core/U2SafePoints.h>
#include <U2Core/UserApplicationsSettings.h>

#include <U2Designer/DelegateEditors.h>

#include <U2Gui/DialogUtils.h>

#include <U2Lang/ActorPrototypeRegistry.h>
#include <U2Lang/BaseActorCategories.h>
#include <U2Lang/BasePorts.h>
#include <U2Lang/BaseSlots.h>
#include <U2Lang/BaseTypes.h>
#include <U2Lang/WorkflowEnv.h>


namespace U2 {
namespace LocalWorkflow {

/*****************************
 * TopHatWorkerFactory
 *****************************/
const QString TopHatWorkerFactory::ACTOR_ID("tophat");

const QString TopHatWorkerFactory::BOWTIE_INDEX_DIR("bowtie-index-dir");
const QString TopHatWorkerFactory::BOWTIE_INDEX_BASENAME("bowtie-index-basename");
const QString TopHatWorkerFactory::REF_SEQ("ref-seq");
const QString TopHatWorkerFactory::MATE_INNER_DISTANCE("mate-inner-distance");
const QString TopHatWorkerFactory::MATE_STANDARD_DEVIATION("mate-standard-deviation");
const QString TopHatWorkerFactory::LIBRARY_TYPE("library-type");
const QString TopHatWorkerFactory::NO_NOVEL_JUNCTIONS("no-novel-junctions");
const QString TopHatWorkerFactory::RAW_JUNCTIONS("raw-junctions");
const QString TopHatWorkerFactory::KNOWN_TRANSCRIPT("known-transcript");
const QString TopHatWorkerFactory::MAX_MULTIHITS("max-multihits");
const QString TopHatWorkerFactory::SEGMENT_LENGTH("segment-length");
const QString TopHatWorkerFactory::DISCORDANT_PAIR_ALIGNMENTS("discordant-pair-alignments");
const QString TopHatWorkerFactory::FUSION_SEARCH("fusion-search");
const QString TopHatWorkerFactory::TRANSCRIPTOME_ONLY("transcriptome-only");
const QString TopHatWorkerFactory::TRANSCRIPTOME_MAX_HITS("transcriptome-max-hits");
const QString TopHatWorkerFactory::PREFILTER_MULTIHITS("prefilter-multihits");
const QString TopHatWorkerFactory::MIN_ANCHOR_LENGTH("min-anchor-length");
const QString TopHatWorkerFactory::SPLICE_MISMATCHES("splice-mismatches");
const QString TopHatWorkerFactory::TRANSCRIPTOME_MISMATCHES("transcriptome-mismatches");
const QString TopHatWorkerFactory::GENOME_READ_MISMATCHES("genome-read-mismatches");
const QString TopHatWorkerFactory::READ_MISMATCHES("read-mismatches");
const QString TopHatWorkerFactory::SEGMENT_MISMATCHES("segment-mismatches");
const QString TopHatWorkerFactory::SOLEXA_1_3_QUALS("solexa-1-3-quals");
const QString TopHatWorkerFactory::BOWTIE_VERSION("bowtie-version");
const QString TopHatWorkerFactory::BOWTIE_N_MODE("bowtie-n-mode");
const QString TopHatWorkerFactory::BOWTIE_TOOL_PATH("bowtie-tool-path");
const QString TopHatWorkerFactory::SAMTOOLS_TOOL_PATH("samtools-tool-path");
const QString TopHatWorkerFactory::EXT_TOOL_PATH("path");
const QString TopHatWorkerFactory::TMP_DIR_PATH("temp-dir");

const QString TopHatWorkerFactory::FIRST_IN_SLOT_ID("first.in");
const QString TopHatWorkerFactory::SECOND_IN_SLOT_ID("second.in");

const QString TopHatWorkerFactory::OUT_MAP_DESCR_ID("out.tophat");
const QString TopHatWorkerFactory::ACCEPTED_HITS_SLOT_ID("accepted.hits");
const QString TopHatWorkerFactory::JUNCTIONS_SLOT_ID("junctions");
const QString TopHatWorkerFactory::INSERTIONS_SLOT_ID("insertions");
const QString TopHatWorkerFactory::DELETIONS_SLOT_ID("deletions");


void TopHatWorkerFactory::init()
{
    QList<PortDescriptor*> portDescriptors;
    QList<Attribute*> attributes;

    // Define ports and slots
    Descriptor inputPortDescriptor(
                BasePorts::IN_SEQ_PORT_ID(),
                TopHatWorker::tr("Input reads"),
                TopHatWorker::tr("Input RNA-Seq reads"));

    Descriptor outputPortDescriptor(
                BasePorts::OUT_ASSEMBLY_PORT_ID(),
                TopHatWorker::tr("TopHat output"),
                TopHatWorker::tr("Accepted hits, junctions, insertions and deletions"));

    QMap<Descriptor, DataTypePtr> inputMap;

    Descriptor firstInDescriptor =
            Descriptor(FIRST_IN_SLOT_ID,
                       TopHatWorker::tr("Input reads"),
                       TopHatWorker::tr("TopHat input reads. When running TopHat"
                                        " with paired-end reads, this should be"
                                        " the *_1 (\"left\") set of reads."));

    Descriptor secondInDescriptor =
            Descriptor(SECOND_IN_SLOT_ID,
                       TopHatWorker::tr("Input reads"),
                       TopHatWorker::tr("Only used when running TopHat with paired"
                                        " end reads, and contains the *_2 (\"right\")"
                                        " set of reads. Reads MUST appear in the same order"
                                        " as the *_1 reads."));

    inputMap[firstInDescriptor] = BaseTypes::DNA_SEQUENCE_TYPE();
    inputMap[secondInDescriptor] = BaseTypes::DNA_SEQUENCE_TYPE();

    portDescriptors << new PortDescriptor(inputPortDescriptor,
                                          DataTypePtr(new MapDataType("in.sequences", inputMap)),
                                          true /* input */);

    QMap<Descriptor, DataTypePtr> outputMap;

    Descriptor acceptedHitsDescriptor =
            Descriptor(ACCEPTED_HITS_SLOT_ID,
                       TopHatWorker::tr("Accepted hits"),
                       TopHatWorker::tr("A list of read alignments"));

    Descriptor junctionsDescriptor =
            Descriptor(JUNCTIONS_SLOT_ID,
                       TopHatWorker::tr("Junctions"),
                       TopHatWorker::tr("Junctions reported by TopHat"));

    Descriptor insertionsDescriptor =
            Descriptor(INSERTIONS_SLOT_ID,
                       TopHatWorker::tr("Insertions"),
                       TopHatWorker::tr("Insertions reported by TopHat"));

    Descriptor deletionsDescriptor =
            Descriptor(DELETIONS_SLOT_ID,
                       TopHatWorker::tr("Deletions"),
                       TopHatWorker::tr("Deletions reported by TopHat"));

    outputMap[acceptedHitsDescriptor] = BaseTypes::ASSEMBLY_TYPE();
    outputMap[junctionsDescriptor] = BaseTypes::ANNOTATION_TABLE_TYPE();
    outputMap[insertionsDescriptor] = BaseTypes::ANNOTATION_TABLE_TYPE();
    outputMap[deletionsDescriptor] = BaseTypes::ANNOTATION_TABLE_TYPE();

    DataTypeRegistry* registry = WorkflowEnv::getDataTypeRegistry();
    assert(registry);

    DataTypePtr mapDataType(new MapDataType(OUT_MAP_DESCR_ID, outputMap));
    registry->registerEntry(mapDataType);

    portDescriptors << new PortDescriptor(
                           outputPortDescriptor,
                           mapDataType,
                           false /* input */,
                           true /* multi */);

    // Description of the element
    Descriptor topHatDescriptor(ACTOR_ID,
        TopHatWorker::tr("Find Splice Junctions with TopHat"),
        TopHatWorker::tr("TopHat is a fast splice junction mapper for RNA-Seq"
            " reads. It aligns RNA-Seq reads to mammalian-sized genomes"
            " using the ultra high-throughput short read aligner Bowtie,"
            " and then analyzes the mapping results to identify splice"
            " junctions between exons."));

    // Define parameters of the element
    Descriptor bowtieIndexDir(BOWTIE_INDEX_DIR,
        TopHatWorker::tr("Bowtie index directory"),
        TopHatWorker::tr("The directory with the Bowtie index for the reference sequence."));

    // THe refSeq parameter and Bowtie parameters' descriptions have been commented because of UGENE-1160

    //     " It is required to either input a directory and a basename of a Bowtie index,"
    //     " or a reference sequence."));

    Descriptor bowtieIndexBasename(BOWTIE_INDEX_BASENAME,
        TopHatWorker::tr("Bowtie index basename"),
        TopHatWorker::tr("The basename of the Bowtie index for the reference sequence."));

    //    " It is required to either input a directory and a basename of a Bowtie index,"
    //    " or a reference sequence."));

    // Descriptor refSeq(REF_SEQ,
    //    TopHatWorker::tr("Reference sequence"),
    //    TopHatWorker::tr("The reference sequence for short reads."
    //    " It is required to either input a directory and a basename of a Bowtie index,"
    //    " or a reference sequence. Note that the Bowtie index parameters have higher priority"
    //    " than this parameter, i.e. if they are specified, this parameter is ignored."));

    Descriptor mateInnerDistance(MATE_INNER_DISTANCE,
        TopHatWorker::tr("Mate inner distance"),
        TopHatWorker::tr("The expected (mean) inner distance between mate pairs"));

    Descriptor mateStandardDeviation(MATE_STANDARD_DEVIATION,
        TopHatWorker::tr("Mate standard deviation"),
        TopHatWorker::tr("The standard deviation for the distribution on inner distances between mate pairs"));

    Descriptor libraryType(LIBRARY_TYPE,
        TopHatWorker::tr("Library type"),
        TopHatWorker::tr("Specifies RNA-Seq protocol"));

    Descriptor noNovelJunctions(NO_NOVEL_JUNCTIONS,
        TopHatWorker::tr("No novel junctions"),
        TopHatWorker::tr("Only look for reads across junctions indicated in"
        " the supplied GFF or junctions file. This parameter is ignored"
        " if <i>Raw junctions</i> or <i>Known transcript file</i> is not set."));

    Descriptor rawJunctions(RAW_JUNCTIONS,
        TopHatWorker::tr("Raw junctions"),
        TopHatWorker::tr("The list of raw junctions"));

    Descriptor knownTranscript(KNOWN_TRANSCRIPT,
        TopHatWorker::tr("Known transcript file"),
        TopHatWorker::tr("A set of gene model annotations and/or known transcripts"));

    Descriptor maxMultihits(MAX_MULTIHITS,
        TopHatWorker::tr("Max multihits"),
        TopHatWorker::tr("Instructs TopHat to allow up to this many alignments to"
        " the reference for a given read, and suppresses all alignments for"
        " reads with more than this many alignments."));

    Descriptor segmentLength(SEGMENT_LENGTH,
        TopHatWorker::tr("Segment length"),
        TopHatWorker::tr("Each read is cut up into segments, each at least this long."
        " These segments are mapped independently."));

    // Commented as it seems there is no "--report-discordant-pair-alignment"
    // in the latest TopHat version (not mentioned in the manual)
    //
    // Descriptor discordantPairAlignments(DISCORDANT_PAIR_ALIGNMENTS,
    //    TopHatWorker::tr("Report discordant pair alignments"),
    //    TopHatWorker::tr("This option will allow mate pairs to map to different"
    //    " chromosomes, distant places on the same chromosome, or on the same strand."));

    Descriptor fusionSearch(FUSION_SEARCH,
        TopHatWorker::tr("Fusion search"),
        TopHatWorker::tr("Turn on fusion mapping"));

    Descriptor transcriptomeOnly(TRANSCRIPTOME_ONLY,
        TopHatWorker::tr("Transcriptome only"),
        TopHatWorker::tr("Only align the reads to the transcriptome and report only"
        " those mappings as genomic mappings"));

    Descriptor transcriptomeMaxHits(TRANSCRIPTOME_MAX_HITS,
        TopHatWorker::tr("Transcriptome max hits"),
        TopHatWorker::tr("Maximum number of mappings allowed for a read, when aligned"
        " to the transcriptome (any reads found with more than this number of"
        " mappings will be discarded)"));

    Descriptor prefilterMultihits(PREFILTER_MULTIHITS,
        TopHatWorker::tr("Prefilter multihits"),
        TopHatWorker::tr("When mapping reads on the transcriptome, some repetitive or"
        " low complexity reads that would be discarded in the context of the genome"
        " may appear to align to the transcript sequences and thus may end up"
        " reported as mapped to those genes only. This option directs TopHat"
        " to first align the reads to the whole genome in order to determine"
        " and exclude such multi-mapped reads (according to the value of the"
        " <i>Max multihits</i> option)."));

    Descriptor minAnchorLength(MIN_ANCHOR_LENGTH,
        TopHatWorker::tr("Min anchor length"),
        TopHatWorker::tr("The <i>anchor length</i>. TopHat will report junctions"
        " spanned by reads with at least this many bases on each side of the"
        " junction. Note that individual spliced alignments may span a junction"
        " with fewer than this many bases on one side. However, every junction"
        " involved in spliced alignments is supported by at least one read with"
        " this many bases on each side."));

    Descriptor spliceMismatches(SPLICE_MISMATCHES,
        TopHatWorker::tr("Splice mismatches"),
        TopHatWorker::tr("The maximum number of mismatches that may appear in"
        " the <i>anchor</i> region of a spliced alignment"));

    Descriptor transcriptomeMismatches(TRANSCRIPTOME_MISMATCHES,
        TopHatWorker::tr("Transcriptome mismatches"),
        TopHatWorker::tr("The maximum number of mismatches allowed when reads"
        " are aligned to the transcriptome"));

    Descriptor genomeReadMismatches(GENOME_READ_MISMATCHES,
        TopHatWorker::tr("Genome read mismatches"),
        TopHatWorker::tr("When whole reads are first mapped on the genome,"
        " this many mismatches in each read alignment are allowed."));

    Descriptor readMismatches(READ_MISMATCHES,
        TopHatWorker::tr("Read mismatches"),
        TopHatWorker::tr("Final read alignments having more than these"
        " many mismatches are discarded."));

    Descriptor segmentMismatches(SEGMENT_MISMATCHES,
        TopHatWorker::tr("Segment mismatches"),
        TopHatWorker::tr("Read segments are mapped independently,"
        " allowing up to this many mismatches in each segment"
        " alignment."));

    Descriptor solexa13Quals(SOLEXA_1_3_QUALS,
        TopHatWorker::tr("Solexa 1.3 quals"),
        TopHatWorker::tr("As of the Illumina GA pipeline version 1.3,"
        " quality scores are encoded in Phred-scaled base-64."
        " Use this option for FASTQ files from pipeline 1.3 or later."));

    Descriptor bowtieVersion(BOWTIE_VERSION,
        TopHatWorker::tr("Bowtie version"),
        TopHatWorker::tr("Specifies which Bowtie version should be used"));

    Descriptor bowtieNMode(BOWTIE_N_MODE,
        TopHatWorker::tr("Bowtie -n mode"),
        TopHatWorker::tr("TopHat uses <i>-v</i> in Bowtie for initial"
        " read mapping (the default), but with this option, <i>-n</i>"
        " is used instead. Read segments are always mapped using"
        " <i>-v</i> option."));

    Descriptor bowtieToolPath(BOWTIE_TOOL_PATH,
        TopHatWorker::tr("Bowtie tool path"),
        TopHatWorker::tr("The path to the Bowtie external tool"));

    Descriptor samtoolsPath(SAMTOOLS_TOOL_PATH,
        TopHatWorker::tr("SAMtools tool path"),
        TopHatWorker::tr("The path to the SAMtools tool. Note that"
                         " the tool is available in the UGENE External Tool Package."));

    Descriptor extToolPath(EXT_TOOL_PATH,
        TopHatWorker::tr("TopHat tool path"),
        TopHatWorker::tr("The path to the TopHat external tool in UGENE"));

    Descriptor tmpDir(TMP_DIR_PATH,
        TopHatWorker::tr("Temporary directory"),
        TopHatWorker::tr("The directory for temporary files"));

    attributes << new Attribute(bowtieIndexDir, BaseTypes::STRING_TYPE(), true, QVariant(""));
    attributes << new Attribute(bowtieIndexBasename, BaseTypes::STRING_TYPE(), true, QVariant(""));
    // attributes << new Attribute(refSeq, BaseTypes::STRING_TYPE(), true, QVariant(""));
    attributes << new Attribute(mateInnerDistance, BaseTypes::NUM_TYPE(), false, QVariant(200));
    attributes << new Attribute(mateStandardDeviation, BaseTypes::NUM_TYPE(), false, QVariant(20));
    attributes << new Attribute(libraryType, BaseTypes::NUM_TYPE(), false, QVariant(0));
    attributes << new Attribute(noNovelJunctions, BaseTypes::BOOL_TYPE(), false, QVariant(false));
    attributes << new Attribute(rawJunctions, BaseTypes::STRING_TYPE(), false, QVariant());
    attributes << new Attribute(knownTranscript, BaseTypes::STRING_TYPE(), false, QVariant());
    attributes << new Attribute(maxMultihits, BaseTypes::NUM_TYPE(), false, QVariant(20));
    attributes << new Attribute(segmentLength, BaseTypes::NUM_TYPE(), false, QVariant(25));
    // attributes << new Attribute(discordantPairAlignments, BaseTypes::BOOL_TYPE(), false, QVariant(false));
    attributes << new Attribute(fusionSearch, BaseTypes::BOOL_TYPE(), false, QVariant(false));
    attributes << new Attribute(transcriptomeOnly, BaseTypes::BOOL_TYPE(), false, QVariant(false));
    attributes << new Attribute(transcriptomeMaxHits, BaseTypes::NUM_TYPE(), false, QVariant(60));
    attributes << new Attribute(prefilterMultihits, BaseTypes::BOOL_TYPE(), false, QVariant(false));
    attributes << new Attribute(minAnchorLength, BaseTypes::NUM_TYPE(), false, QVariant(8));
    attributes << new Attribute(spliceMismatches, BaseTypes::NUM_TYPE(), false, QVariant(0));
    attributes << new Attribute(transcriptomeMismatches, BaseTypes::NUM_TYPE(), false, QVariant(1));
    attributes << new Attribute(genomeReadMismatches, BaseTypes::NUM_TYPE(), false, QVariant(2));
    attributes << new Attribute(readMismatches, BaseTypes::NUM_TYPE(), false, QVariant(2));
    attributes << new Attribute(segmentMismatches, BaseTypes::NUM_TYPE(), false, QVariant(2));
    attributes << new Attribute(solexa13Quals, BaseTypes::BOOL_TYPE(), false, QVariant(false));
    attributes << new Attribute(bowtieVersion, BaseTypes::NUM_TYPE(), false, QVariant(0));
    attributes << new Attribute(bowtieNMode, BaseTypes::NUM_TYPE(), false, QVariant(0));
    attributes << new Attribute(bowtieToolPath, BaseTypes::STRING_TYPE(), true, QVariant(L10N::defaultStr()));
    attributes << new Attribute(samtoolsPath, BaseTypes::STRING_TYPE(), false, QVariant());
    attributes << new Attribute(extToolPath, BaseTypes::STRING_TYPE(), true, QVariant(L10N::defaultStr()));
    attributes << new Attribute(tmpDir, BaseTypes::STRING_TYPE(), true, QVariant(L10N::defaultStr()));

    // Create the actor prototype
    ActorPrototype* proto = new IntegralBusActorPrototype(topHatDescriptor,
        portDescriptors,
        attributes);

    // Values range of some parameters
    QMap<QString, PropertyDelegate*> delegates;

    {
        QVariantMap vm;
        vm[TopHatWorker::tr("Use -n mode")] = 1;
        vm[TopHatWorker::tr("Use -v mode")] = 0;
        delegates[BOWTIE_N_MODE] = new ComboBoxDelegate(vm);
    }
    {
        QVariantMap vm;
        vm[TopHatWorker::tr("Bowtie2")] = 0;
        vm[TopHatWorker::tr("Bowtie1")] = 1;
        delegates[BOWTIE_VERSION] = new ComboBoxDelegate(vm);
    }
    {
        QVariantMap vm;
        vm["minimum"] = 1;
        vm["maximum"] = 1020;
        vm["singleStep"] = 1;
        delegates[MATE_INNER_DISTANCE] = new SpinBoxDelegate(vm);
    }
    {
        QVariantMap vm;
        vm["minimum"] = 1;
        vm["maximum"] = INT_MAX;
        vm["singleStep"] = 1;
        delegates[MATE_STANDARD_DEVIATION] = new SpinBoxDelegate(vm);
    }
    {
        QVariantMap vm;
        vm["Standard Illumina"] = 0;
        vm["dUTP, NSR, NNSR"] = 1;
        vm["Ligation, Standard SOLiD"] = 2;
        delegates[LIBRARY_TYPE] = new ComboBoxDelegate(vm);
    }
    {
        QVariantMap vm;
        vm["minimum"] = 1;
        vm["maximum"] = INT_MAX;
        vm["singleStep"] = 1;
        delegates[MAX_MULTIHITS] = new SpinBoxDelegate(vm);
    }
    {
        QVariantMap vm;
        vm["minimum"] = 10;
        vm["maximum"] = 512;
        vm["singleStep"] = 1;
        delegates[SEGMENT_LENGTH] = new SpinBoxDelegate(vm);
    }
    {
        QVariantMap vm;
        vm["minimum"] = 1;
        vm["maximum"] = INT_MAX;
        vm["singleStep"] = 1;
        delegates[TRANSCRIPTOME_MAX_HITS] = new SpinBoxDelegate(vm);
    }
    {
        QVariantMap vm;
        vm["minimum"] = 3;
        vm["maximum"] = INT_MAX;
        vm["singleStep"] = 1;
        delegates[MIN_ANCHOR_LENGTH] = new SpinBoxDelegate(vm);
    }
    {
        QVariantMap vm;
        vm["minimum"] = 0;
        vm["maximum"] = 2;
        vm["singleStep"] = 1;
        delegates[SPLICE_MISMATCHES] = new SpinBoxDelegate(vm);
    }
    {
        QVariantMap vm;
        vm["minimum"] = 0;
        vm["maximum"] = 3;
        vm["singleStep"] = 1;
        delegates[TRANSCRIPTOME_MISMATCHES] = new SpinBoxDelegate(vm);
        delegates[GENOME_READ_MISMATCHES] = new SpinBoxDelegate(vm);
        delegates[READ_MISMATCHES] = new SpinBoxDelegate(vm);
        delegates[SEGMENT_MISMATCHES] = new SpinBoxDelegate(vm);
    }

    delegates[BOWTIE_INDEX_DIR] = new URLDelegate("", "", false, true);
    delegates[BOWTIE_TOOL_PATH] = new URLDelegate("", "executable", false);
    delegates[REF_SEQ] = new URLDelegate(DialogUtils::prepareDocumentsFileFilter(true), "", false);
    delegates[EXT_TOOL_PATH] = new URLDelegate("", "executable", false);
    delegates[TMP_DIR_PATH] = new URLDelegate("", "TmpDir", false, true);
    delegates[RAW_JUNCTIONS] = new URLDelegate("", "", false);
    delegates[KNOWN_TRANSCRIPT] = new URLDelegate("", "", false);

    // Init and register the actor prototype
    proto->setEditor(new DelegateEditor(delegates));
    proto->setPrompter(new TopHatPrompter());

    WorkflowEnv::getProtoRegistry()->registerProto(
        BaseActorCategories::CATEGORY_RNA_SEQ(),
        proto);

    DomainFactory* localDomain = WorkflowEnv::getDomainRegistry()->getById(LocalDomainFactory::ID);
    localDomain->registerEntry(new TopHatWorkerFactory());
}


/*****************************
 * TopHatPrompter
 *****************************/
TopHatPrompter::TopHatPrompter(Actor* parent)
    : PrompterBase<TopHatPrompter>(parent)
{
}


QString TopHatPrompter::composeRichDoc()
{
    QString result = TopHatWorker::tr(
        "Finds splice junctions using RNA-Seq data.");

    return result;
}


/*****************************
 * TopHatWorker
 *****************************/
TopHatWorker::TopHatWorker(Actor* actor)
    : BaseWorker(actor),
      input(NULL),
      output(NULL)
{
    bindedToSecondSlot = false;
}


void TopHatWorker::init()
{
    input = ports.value(BasePorts::IN_SEQ_PORT_ID());
    output = ports.value(BasePorts::OUT_ASSEMBLY_PORT_ID());

    // Verify if the second slot is connected
    Port* port = actor->getPort(BasePorts::IN_SEQ_PORT_ID());
    SAFE_POINT(NULL != port, "Internal error during initializing TopHatWorker: port is NULL!",);

    IntegralBusPort* bus = dynamic_cast<IntegralBusPort*>(port);
    SAFE_POINT(NULL != bus, "Internal error during initializing TopHatWorker: bus is NULL!",);

    QList<Actor*> producers = bus->getProducers(TopHatWorkerFactory::SECOND_IN_SLOT_ID);
    bindedToSecondSlot = !producers.isEmpty();

    // Init the parameters
    settingsAreCorrect = true;

    settings.workflowContext = context;
    settings.storage = context->getDataStorage();

    settings.bowtieIndexPathAndBasename = actor->getParameter(TopHatWorkerFactory::BOWTIE_INDEX_DIR)->getAttributeValue<QString>(context) +
            "/" + actor->getParameter(TopHatWorkerFactory::BOWTIE_INDEX_BASENAME)->getAttributeValue<QString>(context);

    settings.mateInnerDistance = actor->getParameter(TopHatWorkerFactory::MATE_INNER_DISTANCE)->getAttributeValue<int>(context);
    settings.mateStandardDeviation = actor->getParameter(TopHatWorkerFactory::MATE_STANDARD_DEVIATION)->getAttributeValue<int>(context);

    int libType = actor->getParameter(TopHatWorkerFactory::LIBRARY_TYPE)->getAttributeValue<int>(context);
    if (!settings.libraryType.setLibraryType(libType)) {
        algoLog.error(tr("Incorrect value of the library type parameter for Cufflinks!"));
        settingsAreCorrect = false;
    }

    settings.noNovelJunctions = actor->getParameter(TopHatWorkerFactory::NO_NOVEL_JUNCTIONS)->getAttributeValue<bool>(context);

    settings.rawJunctions = actor->getParameter(TopHatWorkerFactory::RAW_JUNCTIONS)->getAttributeValue<QString>(context);
    settings.knownTranscript = actor->getParameter(TopHatWorkerFactory::KNOWN_TRANSCRIPT)->getAttributeValue<QString>(context);
    settings.maxMultihits = actor->getParameter(TopHatWorkerFactory::MAX_MULTIHITS)->getAttributeValue<int>(context);
    settings.segmentLength = actor->getParameter(TopHatWorkerFactory::SEGMENT_LENGTH)->getAttributeValue<int>(context);
    settings.fusionSearch = actor->getParameter(TopHatWorkerFactory::FUSION_SEARCH)->getAttributeValue<bool>(context);
    settings.transcriptomeOnly = actor->getParameter(TopHatWorkerFactory::TRANSCRIPTOME_ONLY)->getAttributeValue<bool>(context);
    settings.transcriptomeMaxHits = actor->getParameter(TopHatWorkerFactory::TRANSCRIPTOME_MAX_HITS)->getAttributeValue<int>(context);
    settings.prefilterMultihits = actor->getParameter(TopHatWorkerFactory::PREFILTER_MULTIHITS)->getAttributeValue<bool>(context);
    settings.minAnchorLength = actor->getParameter(TopHatWorkerFactory::MIN_ANCHOR_LENGTH)->getAttributeValue<int>(context);
    settings.spliceMismatches = actor->getParameter(TopHatWorkerFactory::SPLICE_MISMATCHES)->getAttributeValue<int>(context);
    settings.transcriptomeMismatches = actor->getParameter(TopHatWorkerFactory::TRANSCRIPTOME_MISMATCHES)->getAttributeValue<int>(context);
    settings.genomeReadMismatches = actor->getParameter(TopHatWorkerFactory::GENOME_READ_MISMATCHES)->getAttributeValue<int>(context);
    settings.readMismatches = actor->getParameter(TopHatWorkerFactory::READ_MISMATCHES)->getAttributeValue<int>(context);
    settings.segmentMismatches = actor->getParameter(TopHatWorkerFactory::SEGMENT_MISMATCHES)->getAttributeValue<int>(context);
    settings.solexa13quals = actor->getParameter(TopHatWorkerFactory::SOLEXA_1_3_QUALS)->getAttributeValue<bool>(context);

    int bowtieModeVal = actor->getParameter(TopHatWorkerFactory::BOWTIE_N_MODE)->getAttributeValue<int>(context);
    switch (bowtieModeVal) {
        case vMode:
            settings.bowtieMode = vMode;
            break;
        case nMode:
            settings.bowtieMode = nMode;
            break;
        default:
           algoLog.error(tr("Unrecognized value of the Bowtie mode option!"));
           settingsAreCorrect = false;
    }

    // Set version (Bowtie1 or Bowtie2) and the path to the corresponding external tool
    int bowtieVersionVal = actor->getParameter(TopHatWorkerFactory::BOWTIE_VERSION)->getAttributeValue<int>(context);
    QString bowtieExtToolPath = actor->getParameter(TopHatWorkerFactory::BOWTIE_TOOL_PATH)->getAttributeValue<QString>(context);
    settings.bowtiePath = bowtieExtToolPath;
    bool bowtiePathIsDefault = (QString::compare(bowtieExtToolPath, "default", Qt::CaseInsensitive) == 0);

    if (0 != bowtieVersionVal) {
        settings.useBowtie1 = true;

        if (bowtiePathIsDefault) {
            settings.bowtiePath = AppContext::getExternalToolRegistry()->getByName(BOWTIE_TOOL_NAME)->getPath();
        }
    }
    else {
        if (bowtiePathIsDefault) {
            // Bowtie2 needs to be embedded as an external tool to uncomment this line
            // settings.bowtiePath = AppContext::getExternalToolRegistry()->getByName(BOWTIE2_TOOL_NAME)->getPath();
        }
    }

    settings.samtoolsPath = actor->getParameter(TopHatWorkerFactory::SAMTOOLS_TOOL_PATH)->getAttributeValue<QString>(context);

    QString tmpDirPath = actor->getParameter(TopHatWorkerFactory::TMP_DIR_PATH)->getAttributeValue<QString>(context);
    if (QString::compare(tmpDirPath, "default", Qt::CaseInsensitive) != 0) {
        AppContext::getAppSettings()->getUserAppsSettings()->setUserTemporaryDirPath(tmpDirPath);
    }

    QString extToolPath = actor->getParameter(TopHatWorkerFactory::EXT_TOOL_PATH)->getAttributeValue<QString>(context);
    if (QString::compare(extToolPath, "default", Qt::CaseInsensitive) != 0) {
        AppContext::getExternalToolRegistry()->getByName(TOPHAT_TOOL_NAME)->setPath(extToolPath);
    }
}


Task* TopHatWorker::tick()
{
    if (false == settingsAreCorrect) {
        return NULL;
    }

    if (input->hasMessage()) {
        Message inputMessage = getMessageAndSetupScriptValues(input);
        SAFE_POINT(!inputMessage.isEmpty(), "Internal error: message can't be NULL!", NULL);

        // Get the sequence ID
        SharedDbiDataHandler seqId = inputMessage.getData().toMap().value(TopHatWorkerFactory::FIRST_IN_SLOT_ID).value<SharedDbiDataHandler>();
        settings.seqIds.append(seqId);

        // If the second slot is connected, expect the sequence of the paired read
        if (bindedToSecondSlot) {
            SharedDbiDataHandler pairedSeqId = inputMessage.getData().toMap().value(TopHatWorkerFactory::SECOND_IN_SLOT_ID).value<SharedDbiDataHandler>();
            settings.pairedSeqIds.append(pairedSeqId);
        }
    }
    else if (input->isEnded()) {
        Task* topHatSupportTask = new TopHatSupportTask(settings);
        connect(topHatSupportTask, SIGNAL(si_stateChanged()), SLOT(sl_topHatTaskFinished()));

        return topHatSupportTask;
    }

    return NULL;
}


void TopHatWorker::sl_topHatTaskFinished()
{
    setDone();
    output->setEnded();

    TopHatSupportTask* topHatSupportTask = qobject_cast<TopHatSupportTask*>(sender());
    if (Task::State_Finished != topHatSupportTask->getState()) {
        return;
    }

    if (output) {
        DataTypePtr outputMapDataType =
                WorkflowEnv::getDataTypeRegistry()->getById(TopHatWorkerFactory::OUT_MAP_DESCR_ID);
        SAFE_POINT(0 != outputMapDataType, "Internal error: can't get DataTypePtr for TopHat output map!",);

        SharedDbiDataHandler acceptedHits = topHatSupportTask->getAcceptedHits();
        QList<SharedAnnotationData> junctionAnnots = topHatSupportTask->getJunctionAnnots();
        QList<SharedAnnotationData> insertionAnnots = topHatSupportTask->getInsertionAnnots();
        QList<SharedAnnotationData> deletionAnnots = topHatSupportTask->getDeletionAnnots();

        QVariantMap messageData;
        messageData[TopHatWorkerFactory::ACCEPTED_HITS_SLOT_ID] =
                qVariantFromValue< SharedDbiDataHandler >(acceptedHits);

        messageData[TopHatWorkerFactory::JUNCTIONS_SLOT_ID] =
                qVariantFromValue< QList<SharedAnnotationData> >(junctionAnnots);

        messageData[TopHatWorkerFactory::INSERTIONS_SLOT_ID] =
                qVariantFromValue< QList<SharedAnnotationData> >(insertionAnnots);

        messageData[TopHatWorkerFactory::DELETIONS_SLOT_ID] =
                qVariantFromValue< QList<SharedAnnotationData> >(deletionAnnots);

        output->put(Message(outputMapDataType, messageData));
    }
}


void TopHatWorker::cleanup()
{
}

} // namespace LocalWorkflow
} // namespace U2
