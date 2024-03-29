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

#include <U2Core/AppContext.h>
#include <U2Core/DbiDocumentFormat.h>
#include <U2Core/DocumentUtils.h>
#include <U2Core/TaskSignalMapper.h>
#include <U2Core/AddDocumentTask.h>
#include <U2Core/TextUtils.h>
#include <U2Core/ProjectModel.h>
#include <U2Core/IOAdapter.h>
#include <U2Core/IOAdapterUtils.h>
#include <U2Core/U2DbiRegistry.h>
#include <U2Core/LoadDocumentTask.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/U2SafePoints.h>

#include <U2Formats/SAMFormat.h>

#include <U2Gui/OpenViewTask.h>
#include <U2Gui/MainWindow.h>

#include <U2Gui/LastUsedDirHelper.h>

#include "Dbi.h"
#include "Exception.h"
#include "ConvertToSQLiteDialog.h"
#include "ConvertToSQLiteTask.h"
#include "BAMDbiPlugin.h"
#include "LoadBamInfoTask.h"
#include "BAMFormat.h"
#include "SamtoolsBasedDbi.h"


#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QMainWindow>

#include "SamtoolsBasedDbi.h"

namespace U2 {
namespace BAM {

extern "C" Q_DECL_EXPORT Plugin* U2_PLUGIN_INIT_FUNC() {
    BAMDbiPlugin* plug = new BAMDbiPlugin();
    return plug;
}

BAMDbiPlugin::BAMDbiPlugin() : Plugin(tr("BAM format support"), tr("Interface for indexed read-only access to BAM files"))
{
    DocumentFormatFlags flags = DocumentFormatFlags(DocumentFormatFlag_NoPack)
        | DocumentFormatFlag_NoFullMemoryLoad
        | DocumentFormatFlag_Hidden
        | DocumentFormatFlag_SupportWriting;
    DocumentFormat *bamDbi = new DbiDocumentFormat(SamtoolsBasedDbiFactory::ID, BaseDocumentFormats::BAM, tr("BAM File"), QStringList("bam"), flags);
    AppContext::getDocumentFormatRegistry()->registerFormat(bamDbi);
    AppContext::getDbiRegistry()->registerDbiFactory(new SamtoolsBasedDbiFactory());
    //AppContext::getDbiRegistry()->registerDbiFactory(new DbiFactory());

    AppContext::getDocumentFormatRegistry()->getImportSupport()->addDocumentImporter(new BAMImporter());
}

void BAMDbiPlugin::sl_converter() {
    try {
        if(!AppContext::getDbiRegistry()->getRegisteredDbiFactories().contains(SQLITE_DBI_ID)) {
            throw Exception(tr("SQLite DBI plugin is not loaded"));
        }
        LastUsedDirHelper lod;
        QString fileName = QFileDialog::getOpenFileName(AppContext::getMainWindow()->getQMainWindow(), tr("Open BAM/SAM file"), lod.dir, tr("Assembly Files (*.bam *.sam)"));
        if (!fileName.isEmpty()) {
            lod.url = fileName;
            GUrl sourceUrl(fileName);
            QList<FormatDetectionResult> detectedFormats = DocumentUtils::detectFormat(sourceUrl);
            bool sam = false;
            if (!detectedFormats.isEmpty()) {
                if (detectedFormats.first().format->getFormatId() == BaseDocumentFormats::SAM) {
                    sam = true;
                }
            }
            LoadInfoTask* task = new LoadInfoTask(sourceUrl, sam);
            connect(new TaskSignalMapper(task), SIGNAL(si_taskFinished(Task*)), SLOT(sl_infoLoaded(Task*)));
            AppContext::getTaskScheduler()->registerTopLevelTask(task);
        }
    } catch(const Exception &e) {
        QMessageBox::critical(NULL, tr("Error"), e.getMessage());
    }
}

void BAMDbiPlugin::sl_infoLoaded(Task* task) {
    LoadInfoTask* loadInfoTask = qobject_cast<LoadInfoTask*>(task);
    bool sam = loadInfoTask->isSam();
    if(!loadInfoTask->hasError()) {
        const GUrl& sourceUrl = loadInfoTask->getSourceUrl();
        BAMInfo& bamInfo = loadInfoTask->getInfo();
        ConvertToSQLiteDialog convertDialog(sourceUrl, bamInfo, sam);
        if(QDialog::Accepted == convertDialog.exec()) {
            GUrl destUrl = convertDialog.getDestinationUrl();
            ConvertToSQLiteTask *task = new ConvertToSQLiteTask(sourceUrl, destUrl, loadInfoTask->getInfo(), sam);
            if(convertDialog.addToProject()) {
                connect(new TaskSignalMapper(task), SIGNAL(si_taskFinished(Task*)), SLOT(sl_addDbFileToProject(Task*)));
            }
            AppContext::getTaskScheduler()->registerTopLevelTask(task);
        }
    }
}

void BAMDbiPlugin::sl_addDbFileToProject(Task * task) {
    ConvertToSQLiteTask * convertToBAMTask = qobject_cast<ConvertToSQLiteTask*>(task);
    if(convertToBAMTask == NULL) {
        assert(false);
        return;
    }
    if(convertToBAMTask->hasError() || convertToBAMTask->isCanceled()) {
        return;
    }
    GUrl url = convertToBAMTask->getDestinationUrl();
    assert(!url.isEmpty());
    Project * prj = AppContext::getProject();
    if(prj == NULL) {
        QList<GUrl> list;
        list.append(url);
        Task * t = AppContext::getProjectLoader()->openWithProjectTask(list);
        if (t != NULL) {
            AppContext::getTaskScheduler()->registerTopLevelTask(t);
        }
        return;
    }
    Document * doc = prj->findDocumentByURL(url);
    if(doc != NULL && doc->isLoaded()) {
        return;
    }
    AddDocumentTask * addTask = NULL;
    if(doc == NULL) {
        IOAdapterFactory * iof = AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(IOAdapterUtils::url2io(url.getURLString()));
        CHECK(NULL != iof, );
        DocumentFormat * df = AppContext::getDocumentFormatRegistry()->getFormatById(BaseDocumentFormats::UGENEDB);
        CHECK(NULL != df, );
        U2OpStatus2Log os;
        doc = df->createNewUnloadedDocument(iof, url, os);
        CHECK_OP(os, );
        addTask = new AddDocumentTask(doc);
    }
    LoadUnloadedDocumentAndOpenViewTask * openViewTask = new LoadUnloadedDocumentAndOpenViewTask(doc);
    if(addTask != NULL) {
        openViewTask->addSubTask(addTask);
        openViewTask->setMaxParallelSubtasks(1);    
    }
    AppContext::getTaskScheduler()->registerTopLevelTask(openViewTask);
}

//////////////////////////////////////////////////////////////////////////
// BAM importer
BAMImporter::BAMImporter() : DocumentImporter("bam-importer", tr("BAM/SAM file import")){
    //prepare sorted extensions list
    QSet<QString> extsSet; BAMFormat bam; SAMFormat sam;
    extsSet.unite(bam.getSupportedDocumentFileExtensions().toSet()).unite(sam.getSupportedDocumentFileExtensions().toSet());
    QStringList exts = extsSet.toList();
    qSort(exts);
    
    extensions << exts;
    importerDescription = tr("BAM files importer is used to convert conventional BAM and SAM files into UGENE database format. Having BAM or SAM file converted into UGENE DB format you get an fast and efficient interface to your data with an option to change the content");
}

#define SAM_HINT "bam-importer-sam-hint"

FormatCheckResult BAMImporter::checkRawData(const QByteArray& rawData, const GUrl& url) {
    BAMFormat bamFormat;
    FormatCheckResult bamScore = bamFormat.checkRawData(rawData, url);

    SAMFormat samFormat;
    FormatCheckResult samScore = samFormat.checkRawData(rawData, url);

    if (bamScore.score > samScore.score ) {
        return bamScore;
    }
    samScore.properties[SAM_HINT] = true;
    return samScore;
}

DocumentProviderTask* BAMImporter::createImportTask(const FormatDetectionResult& res, bool showWizard, const QVariantMap &hints) {
    bool sam = res.rawDataCheckResult.properties[SAM_HINT].toBool();
    QVariantMap fullHints(hints);
    fullHints[SAM_HINT] = sam;
    return new BAMImporterTask(res.url, showWizard, fullHints);
}


BAMImporterTask::BAMImporterTask(const GUrl& url, bool _useGui, const QVariantMap &hints) 
: DocumentProviderTask(tr("BAM/SAM file import: %1").arg(url.fileName()), TaskFlags_NR_FOSCOE)
{
    useGui = _useGui;
    sam = hints.value(SAM_HINT, false).toBool();
    if (hints.contains(DocumentFormat::DBI_REF_HINT)) {
        U2DbiRef ref = hints.value(DocumentFormat::DBI_REF_HINT).value<U2DbiRef>();
        hintedDbiUrl = ref.dbiId;
    }
    convertTask = NULL;
    loadDocTask = NULL;
    loadInfoTask = new LoadInfoTask(url, sam);
    addSubTask(loadInfoTask);

    documentDescription = url.fileName();
}

QList<Task*> BAMImporterTask::onSubTaskFinished(Task* subTask) {
    QList<Task*> res;
    if (subTask->hasError()) {
        propagateSubtaskError();
        return res;
    }
    if (loadInfoTask == subTask) {
        GUrl srcUrl = loadInfoTask->getSourceUrl();
        GUrl destUrl;
        if (hintedDbiUrl.isEmpty()) {
            destUrl = srcUrl.dirPath() + "/" + srcUrl.fileName() + ".ugenedb";
        } else {
            destUrl = hintedDbiUrl;
        }
        bool convert = true;
        if (useGui) {
            ConvertToSQLiteDialog convertDialog(loadInfoTask->getSourceUrl(), loadInfoTask->getInfo(), loadInfoTask->isSam());
            convertDialog.hideAddToProjectOption();
            int rc = convertDialog.exec();
            if (rc == QDialog::Accepted) {
                destUrl = convertDialog.getDestinationUrl();
                convert = true;
            } else {
                convert = false;
                stateInfo.setCanceled(true);
            }
        }
        if (convert) {
            convertTask = new ConvertToSQLiteTask(loadInfoTask->getSourceUrl(), destUrl, loadInfoTask->getInfo(), sam);
            res << convertTask;
        }
    } else if (convertTask == subTask) {
        loadDocTask = LoadDocumentTask::getDefaultLoadDocTask(convertTask->getDestinationUrl());
        if (loadDocTask == NULL) {
            setError(tr("Failed to get load task for : %1").arg(convertTask->getDestinationUrl().getURLString()));
            return res;
        }
        res << loadDocTask;
    } else {
        assert(subTask == loadDocTask);
        resultDocument = loadDocTask->takeDocument();
    }
    return res;
}

} // namespace BAM
} // namespace U2
