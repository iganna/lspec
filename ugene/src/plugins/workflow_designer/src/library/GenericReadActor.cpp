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

#include "GenericReadActor.h"
#include "CoreLib.h"
#include "DocActors.h"

#include <U2Lang/WorkflowEnv.h>
#include <U2Designer/DelegateEditors.h>
#include <U2Lang/CoreLibConstants.h>
#include <U2Lang/BaseTypes.h>
#include <U2Lang/BaseSlots.h>
#include <U2Lang/BasePorts.h>
#include <U2Lang/BaseAttributes.h>
#include <U2Lang/BaseActorCategories.h>

#include <U2Core/DNAInfo.h>
#include <U2Core/DNASequenceObject.h>
#include <U2Core/DocumentModel.h>
#include <U2Core/AppContext.h>
#include <U2Core/GObjectTypes.h>
#include <U2Gui/DialogUtils.h>
#include <U2Gui/GUIUtils.h>

#include <QtCore/QFileInfo>
#include <limits.h>

/* TRANSLATOR U2::Workflow::SeqReadPrompter */
/* TRANSLATOR U2::Workflow::CoreLib */

namespace U2 {
namespace Workflow {

/************************************************************************/
/* GenericReadDocProto */
/************************************************************************/
#define ICOLOR QColor(85,85,255)

const QString GenericReadDocProto::FILE_OR_DIR("input-type");
const QString GenericReadDocProto::INPUT_PATH("in-path");
const QString GenericReadDocProto::RECURSIVE("recursive");
const QString GenericReadDocProto::INCLUDE_NAME_FILTER("include-name-filter");
const QString GenericReadDocProto::EXCLUDE_NAME_FILTER("exclude-name-filter");

const QString GenericReadDocProto::INPUT_FILES("Files");
const QString GenericReadDocProto::INPUT_DIRS("Directories");

GenericReadDocProto::GenericReadDocProto(const Descriptor &desc)
: IntegralBusActorPrototype(desc)
{
    {
        Descriptor fileOrDir(FILE_OR_DIR,
            QObject::tr("Input type"),
            QObject::tr("Choose input type: file or directory"));
        Descriptor inPath(INPUT_PATH,
            QObject::tr("Input directory"),
            QObject::tr("Input directory"));
        Descriptor isRecursive(RECURSIVE,
            QObject::tr("Recursive reading"),
            QObject::tr("Get files from all nested directories or just from the current one"));
        Descriptor includeFilter(INCLUDE_NAME_FILTER,
            QObject::tr("Relative path include filter"),
            QObject::tr("Filter files by relative path using this regular expression. "
            "<p><i>Set it empty to switch off this filter. Use <b>*</b> and <b>?</b> to mask some symbols.</i></p>"));
        Descriptor excludeFilter(EXCLUDE_NAME_FILTER,
            QObject::tr("Relative path exclude filter"),
            QObject::tr("Exclude files which relative paths are matched by this regular expression. "
            "<p><i>Set it empty to switch off this filter. Use <b>*</b> and <b>?</b> to mask some symbols.</i></p>"));

        attrs << new Attribute(fileOrDir, BaseTypes::STRING_TYPE(), false, INPUT_FILES);
        Attribute *filesUrlAttr = new Attribute(BaseAttributes::URL_IN_ATTRIBUTE(), BaseTypes::STRING_TYPE(), false);
        filesUrlAttr->addRelation(new VisibilityRelation(fileOrDir.getId(), INPUT_FILES));
        attrs << filesUrlAttr;

        Attribute *dirsUrlAttr = new Attribute(inPath, BaseTypes::STRING_TYPE(), false);
        dirsUrlAttr->addRelation(new VisibilityRelation(fileOrDir.getId(), INPUT_DIRS));
        attrs << dirsUrlAttr;

        Attribute *recursiveAttr = new Attribute(isRecursive, BaseTypes::BOOL_TYPE(), false, false);
        recursiveAttr->addRelation(new VisibilityRelation(fileOrDir.getId(), INPUT_DIRS));
        attrs << recursiveAttr;

        Attribute *incAttr = new Attribute(includeFilter, BaseTypes::STRING_TYPE());
        incAttr->addRelation(new VisibilityRelation(fileOrDir.getId(), INPUT_DIRS));
        attrs << incAttr;

        Attribute *excAttr = new Attribute(excludeFilter, BaseTypes::STRING_TYPE());
        excAttr->addRelation(new VisibilityRelation(fileOrDir.getId(), INPUT_DIRS));
        attrs << excAttr;
    }

    setEditor(new DelegateEditor(QMap<QString, PropertyDelegate*>()));
    getEditor()->addDelegate(
        new URLDelegate(DialogUtils::prepareDocumentsFileFilter(true), QString(), true),
        BaseAttributes::URL_IN_ATTRIBUTE().getId());
    getEditor()->addDelegate(
        new URLDelegate(QString(), QString(), true, true),
        INPUT_PATH);
    {
        QVariantMap types;
        types[INPUT_FILES] = INPUT_FILES;
        types[INPUT_DIRS] = INPUT_DIRS;
        getEditor()->addDelegate(new ComboBoxDelegate(types), FILE_OR_DIR);
    }

    if(AppContext::isGUIMode()) {
        setIcon( GUIUtils::createRoundIcon(ICOLOR, 22) );
    }
}

const QString GenericSeqActorProto::MODE_ATTR("mode");
const QString GenericSeqActorProto::GAP_ATTR("merge-gap");
const QString GenericSeqActorProto::ACC_ATTR("accept-accession");
const QString GenericSeqActorProto::LIMIT_ATTR("sequence-count-limit");

const QString GenericSeqActorProto::TYPE("generic.seq");
const QString GenericMAActorProto::TYPE("generic.ma");

GenericSeqActorProto::GenericSeqActorProto() : GenericReadDocProto(CoreLibConstants::GENERIC_READ_SEQ_PROTO_ID)
{
    setDisplayName(U2::Workflow::CoreLib::tr("Read Sequence"));
    desc = U2::Workflow::CoreLib::tr("Reads sequences and annotations if any from local or remote files.");
    QMap<Descriptor, DataTypePtr> m;
    m[BaseSlots::URL_SLOT()] = BaseTypes::STRING_TYPE();
    m[BaseSlots::DNA_SEQUENCE_SLOT()] = BaseTypes::DNA_SEQUENCE_TYPE();
    m[BaseSlots::ANNOTATION_TABLE_SLOT()] = BaseTypes::ANNOTATION_TABLE_TYPE();

    DataTypePtr seqTypeset(new MapDataType(Descriptor(TYPE), m));
    bool treg = WorkflowEnv::getDataTypeRegistry()->registerEntry(seqTypeset);
    Q_UNUSED(treg);assert(treg);

    ports << new PortDescriptor(Descriptor(BasePorts::OUT_SEQ_PORT_ID(), U2::Workflow::CoreLib::tr("Sequence"), 
                                            U2::Workflow::CoreLib::tr("A sequence of any type (nucleotide, protein).")), 
                                seqTypeset, false, true);
    {
        Descriptor md(GenericSeqActorProto::MODE_ATTR, SeqReadPrompter::tr("Mode"),
            SeqReadPrompter::tr("If the file contains more than one sequence, <i>Split</i> mode sends them \"as is\" to the output, " 
            "while <i>Merge</i> appends all the sequences and outputs the sole merged sequence."));
        Descriptor gd(GenericSeqActorProto::GAP_ATTR, SeqReadPrompter::tr("Merging gap"), 
            SeqReadPrompter::tr("In <i>Merge</i> mode, the specified number of gaps are inserted between the original sequences. "
            "This is helpful, for example, to avoid finding false positives at the merge boundaries."));
        Descriptor acd(GenericSeqActorProto::ACC_ATTR, SeqReadPrompter::tr("Accession filter"), 
            SeqReadPrompter::tr("Reports only sequences containing the specified regular expression."
                                "<p><i>Leave it empty to switch off this filter. Use <b>*</b> and <b>?</b> to mask some symbols.</i></p>"));
        Descriptor ld(LIMIT_ATTR, SeqReadPrompter::tr("Sequence count limit"),
            SeqReadPrompter::tr("<i>Split mode</i> only."
            "<p>Read only first N sequences from each file."
            "<br>Set 0 value for reading all sequences.</p>"));

        attrs << new Attribute(md, BaseTypes::NUM_TYPE(), true, SPLIT);
        attrs << new Attribute(gd, BaseTypes::NUM_TYPE(), false, 10);
        attrs << new Attribute(ld, BaseTypes::NUM_TYPE(), false, 0);
        attrs << new Attribute(acd, BaseTypes::STRING_TYPE(), false, QString());
    }

    QMap<QString, PropertyDelegate*> delegates;
    {
        QVariantMap modeMap;
        QString splitStr = SeqReadPrompter::tr("Split");
        QString mergeStr = SeqReadPrompter::tr("Merge");
        modeMap[splitStr] = SPLIT;
        modeMap[mergeStr] = MERGE;
        getEditor()->addDelegate(new ComboBoxDelegate(modeMap), MODE_ATTR);
    }
    {
        QVariantMap m; m["minimum"] = 0; m["maximum"] = INT_MAX;
        getEditor()->addDelegate(new SpinBoxDelegate(m), GAP_ATTR);
        getEditor()->addDelegate(new SpinBoxDelegate(m), LIMIT_ATTR);
    }
    //setPrompter(new ReadDocPrompter(U2::Workflow::CoreLib::tr("Reads sequence(s) from <u>%1</u>.")));
    setPrompter(new ReadDocPrompter(U2::Workflow::CoreLib::tr("Reads sequence(s) from file(s).")));

    QString seqSlotId = BasePorts::OUT_SEQ_PORT_ID() + "." + BaseSlots::DNA_SEQUENCE_SLOT().getId();
    QString annsSlotId = BasePorts::OUT_SEQ_PORT_ID() + "." + BaseSlots::ANNOTATION_TABLE_SLOT().getId();
    addSlotRelation(BasePorts::OUT_SEQ_PORT_ID(), BaseSlots::DNA_SEQUENCE_SLOT().getId(),
        BasePorts::OUT_SEQ_PORT_ID(), BaseSlots::ANNOTATION_TABLE_SLOT().getId());
}

GenericMAActorProto::GenericMAActorProto() : IntegralBusActorPrototype(CoreLibConstants::GENERIC_READ_MA_PROTO_ID) 
{
    setDisplayName(U2::Workflow::CoreLib::tr("Read Alignment"));
    desc = U2::Workflow::CoreLib::tr("Reads multiple sequence alignments (MSAs) from local or remote files."
        "<p>Besides the known alignment formats, it supports composing an alignment from a set of sequences in a corresponding file (e.g. FASTA or Genbank).");

    QMap<Descriptor, DataTypePtr> m;
    m[BaseSlots::URL_SLOT()] = BaseTypes::STRING_TYPE();
    m[BaseSlots::MULTIPLE_ALIGNMENT_SLOT()] = BaseTypes::MULTIPLE_ALIGNMENT_TYPE();
    DataTypePtr blockTypeset(new MapDataType(Descriptor(TYPE), m));
    bool treg = WorkflowEnv::getDataTypeRegistry()->registerEntry(blockTypeset);
    Q_UNUSED(treg);assert(treg);

    ports << new PortDescriptor(Descriptor(BasePorts::OUT_MSA_PORT_ID(), U2::Workflow::CoreLib::tr("Multiple sequence alignment"), ""), 
                                blockTypeset, false, true);
    
    attrs << new Attribute(BaseAttributes::URL_IN_ATTRIBUTE(), BaseTypes::STRING_TYPE(), true);
    setEditor(new DelegateEditor(BaseAttributes::URL_IN_ATTRIBUTE().getId(), new URLDelegate(DialogUtils::prepareDocumentsFileFilter(true), QString(),true)));
    setPrompter(new ReadDocPrompter(U2::Workflow::CoreLib::tr("Reads MSA(s) from <u>%1</u>.")));
    
    if( AppContext::isGUIMode() ) {
        setIcon( GUIUtils::createRoundIcon(ICOLOR, 22) );
    }
}

bool GenericMAActorProto::isAcceptableDrop(const QMimeData* md, QVariantMap* params) const {
    QList<DocumentFormat*> fs;
    QString url = WorkflowUtils::getDropUrl(fs, md);
    foreach(DocumentFormat* f, fs) {
        if (f->getSupportedObjectTypes().contains(GObjectTypes::MULTIPLE_ALIGNMENT)) {
            if (params != NULL) {
                params->insert(BaseAttributes::URL_IN_ATTRIBUTE().getId(), url);
            }
            return true;
        }
    }
    /*DocumentFormatConstraints sc;
    sc.supportedObjectTypes.append(GObjectTypes::SEQUENCE);
    foreach(DocumentFormat* f, fs) {
        if (f->checkConstraints(sc)) {
            if (params) {
                params->insert(CoreLib::URL_ATTR_ID, url);
            }
            return true;
        }
    }*/
    if (QFileInfo(url).isDir()) {
        if (params) {
            params->insert(BaseAttributes::URL_IN_ATTRIBUTE().getId(), url + "/*");
        }
        return true;
    }

    return false;
}

bool GenericSeqActorProto::isAcceptableDrop(const QMimeData* md, QVariantMap* params) const {
    QList<DocumentFormat*> fs;
    const GObjectMimeData* gomd = qobject_cast<const GObjectMimeData*>(md);
    if (gomd && params) {
        const U2SequenceObject* obj = qobject_cast<const U2SequenceObject*>(gomd->objPtr.data());
        if (obj) {
            params->insert(BaseAttributes::URL_IN_ATTRIBUTE().getId(), obj->getDocument()->getURLString());
            QString acc = obj->getStringAttribute(DNAInfo::ACCESSION);
            if (acc.isEmpty()) {
                acc = obj->getSequenceName();
            }
            params->insert(ACC_ATTR, acc);
        }
    }

    QString url = WorkflowUtils::getDropUrl(fs, md);
    foreach(DocumentFormat* f, fs) {
        if (f->getSupportedObjectTypes().contains(GObjectTypes::SEQUENCE)) {
            if (params) {
                params->insert(BaseAttributes::URL_IN_ATTRIBUTE().getId(), url);
            }
            return true;
        }
    }
    if (QFileInfo(url).isDir()) {
        if (params) {
            params->insert(BaseAttributes::URL_IN_ATTRIBUTE().getId(), url + "/*");
        }
        return true;
    }

    return false;
}

QString SeqReadPrompter::composeRichDoc() {
    //TODO finish
    //BusPort* input = qobject_cast<BusPort*>(target->getPort(CoreLib::DATA_PORT_ID));
    QString url = getURL(BaseAttributes::URL_IN_ATTRIBUTE().getId());
    return QString("%1").arg(url);
}

}//namespace Workflow
}//namespace U2
