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

#include "ClustalOSupportTask.h"
#include "ClustalOSupport.h"

#include <U2Core/AppContext.h>
#include <U2Core/AppSettings.h>
#include <U2Core/Counter.h>
#include <U2Core/UserApplicationsSettings.h>
#include <U2Core/DocumentModel.h>
#include <U2Core/DNAAlphabet.h>
#include <U2Core/ExternalToolRegistry.h>
#include <U2Core/ProjectModel.h>
#include <U2Core/MAlignmentObject.h>
#include <U2Core/IOAdapterUtils.h>
#include <U2Core/U2SafePoints.h>
#include <U2Core/AddDocumentTask.h>
#include <U2Gui/OpenViewTask.h>

namespace U2 {

void ClustalOSupportTaskSettings::reset() {
    numIterations=-1;
    maxGuidetreeIterations=-1;
    maxHMMIterations=-1;

    setAutoOptions=false;
    numberOfProcessors=1;
}

ClustalOSupportTask::ClustalOSupportTask(MAlignmentObject* _mAObject, const ClustalOSupportTaskSettings& _settings) :
        Task("Run ClustalO alignment task", TaskFlags_NR_FOSCOE),
        mAObject(_mAObject), settings(_settings)
{
    GCOUNTER( cvar, tvar, "ClustalOSupportTask" );
    currentDocument = mAObject->getDocument();
    saveTemporaryDocumentTask=NULL;
    loadTemporyDocumentTask=NULL;
    clustalOTask=NULL;
    newDocument=NULL;
    logParser=NULL;
    if (!mAObject->getAlphabet()->isAmino()){
        stateInfo.setError(tr("Input sequences is non-protein sequences."));
        return;
    }
}

void ClustalOSupportTask::prepare(){
    algoLog.info(tr("ClustalO alignment started"));

    //Add new subdir for temporary files
    //Directory name is ExternalToolName + CurrentDate + CurrentTime

    QString tmpDirName = "ClustalO_"+QString::number(this->getTaskId())+"_"+
                         QDate::currentDate().toString("dd.MM.yyyy")+"_"+
                         QTime::currentTime().toString("hh.mm.ss.zzz")+"_"+
                         QString::number(QCoreApplication::applicationPid())+"/";
    QString tmpDirPath = AppContext::getAppSettings()->getUserAppsSettings()->getCurrentProcessTemporaryDirPath(CLUSTALO_TMP_DIR) + "/" + tmpDirName;
    url= tmpDirPath + "tmp.aln";
    ioLog.details(tr("Saving data to temporary file '%1'").arg(url));

    //Check and remove subdir for temporary files
    QDir tmpDir(tmpDirPath);
    if(tmpDir.exists()){
        foreach(const QString& file, tmpDir.entryList()){
            tmpDir.remove(file);
        }
        if(!tmpDir.rmdir(tmpDir.absolutePath())){
            stateInfo.setError(tr("Subdirectory for temporary files exists. Can not remove this directory."));
            return;
        }
    }
    if(!tmpDir.mkpath(tmpDirPath)){
        stateInfo.setError(tr("Can not create directory for temporary files."));
        return;
    }

    saveTemporaryDocumentTask = new SaveAlignmentTask(mAObject->getMAlignment(), url, BaseDocumentFormats::CLUSTAL_ALN);
    saveTemporaryDocumentTask->setSubtaskProgressWeight(5);
    addSubTask(saveTemporaryDocumentTask);
}

QList<Task*> ClustalOSupportTask::onSubTaskFinished(Task* subTask) {
    QList<Task*> res;
    if(subTask->hasError()) {
        stateInfo.setError(subTask->getError());
        return res;
    }
    if(hasError() || isCanceled()) {
        return res;
    }
    QString outputUrl = url+".out.aln";
    if(subTask==saveTemporaryDocumentTask){
        QStringList arguments;
        arguments << "-v";
        arguments <<"--infile="+url;
        arguments << "--outfmt=clu";
        arguments << "--outfile="+outputUrl;
        if(settings.numIterations != -1){
            arguments << "--iterations" << QString::number(settings.numIterations);
        }
        if(settings.maxGuidetreeIterations != -1){
            arguments << "--max-guidetree-iterations="+QString::number(settings.maxGuidetreeIterations);
        }
        if(settings.maxHMMIterations!= -1){
            arguments << "--max-hmm-iterations="+QString::number(settings.maxHMMIterations);
        }
        if(settings.setAutoOptions){
            arguments << "--auto";
        }
        arguments <<"--threads="+QString::number(settings.numberOfProcessors);

        logParser=new ClustalOLogParser();
        clustalOTask=new ExternalToolRunTask(CLUSTALO_TOOL_NAME,arguments, logParser);
        clustalOTask->setSubtaskProgressWeight(95);
        res.append(clustalOTask);
    }else if(subTask==clustalOTask){
        assert(logParser);
        delete logParser;
        if(!QFileInfo(outputUrl).exists()){
            if(AppContext::getExternalToolRegistry()->getByName(CLUSTALO_TOOL_NAME)->isValid()){
                stateInfo.setError(tr("Output file %1 not found").arg(outputUrl));
            }else{
                stateInfo.setError(tr("Output file %3 not found. May be %1 tool path '%2' not valid?")
                                   .arg(AppContext::getExternalToolRegistry()->getByName(CLUSTALO_TOOL_NAME)->getName())
                                   .arg(AppContext::getExternalToolRegistry()->getByName(CLUSTALO_TOOL_NAME)->getPath())
                                   .arg(outputUrl));
            }
            emit si_stateChanged();
            return res;
        }
        ioLog.details(tr("Loading output file '%1'").arg(outputUrl));
        loadTemporyDocumentTask=
                new LoadDocumentTask(BaseDocumentFormats::CLUSTAL_ALN,
                                     outputUrl,
                                     AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(BaseIOAdapters::LOCAL_FILE));
        loadTemporyDocumentTask->setSubtaskProgressWeight(5);
        res.append(loadTemporyDocumentTask);
    }else if(subTask==loadTemporyDocumentTask){
        newDocument=loadTemporyDocumentTask->takeDocument();
        SAFE_POINT(newDocument!=NULL, QString("output document '%1' not loaded").arg(newDocument->getURLString()), res);
        SAFE_POINT(newDocument->getObjects().length()==1, QString("no objects in output document '%1'").arg(newDocument->getURLString()), res);

        //move MAlignment from new alignment to old document
        MAlignmentObject* newMAligmentObject=qobject_cast<MAlignmentObject*>(newDocument->getObjects().first());
        SAFE_POINT(newMAligmentObject!=NULL, "newDocument->getObjects().first() is not a MAlignmentObject", res);

        resultMA=newMAligmentObject->getMAlignment();
        mAObject->setMAlignment(resultMA);
        if(currentDocument != NULL){
            currentDocument->setModified(true);
        }
        algoLog.info(tr("ClustalO alignment successfully finished"));
        //new document deleted in destructor of LoadDocumentTask
    }
    return res;
}
Task::ReportResult ClustalOSupportTask::report(){
    //Remove subdir for temporary files, that created in prepare
    if(!url.isEmpty()){
        QDir tmpDir(QFileInfo(url).absoluteDir());
        foreach(QString file, tmpDir.entryList()){
            tmpDir.remove(file);
        }
        if(!tmpDir.rmdir(tmpDir.absolutePath())){
            stateInfo.setError(tr("Can not remove directory for temporary files."));
            emit si_stateChanged();
        }
    }

    return ReportResult_Finished;
}

////////////////////////////////////////
//ClustalOWithExtFileSpecifySupportTask
ClustalOWithExtFileSpecifySupportTask::ClustalOWithExtFileSpecifySupportTask(const ClustalOSupportTaskSettings& _settings) :
        Task("Run ClustalO alignment task", TaskFlags_NR_FOSCOE),
        settings(_settings)
{
    GCOUNTER( cvar, tvar, "ClustalOSupportTask" );
    mAObject = NULL;
    currentDocument = NULL;
    saveDocumentTask = NULL;
    loadDocumentTask = NULL;
    clustalOSupportTask = NULL;
    cleanDoc = true;
}

ClustalOWithExtFileSpecifySupportTask::~ClustalOWithExtFileSpecifySupportTask(){
    if (cleanDoc) {
        delete currentDocument;
    }
}

void ClustalOWithExtFileSpecifySupportTask::prepare(){
    DocumentFormatConstraints c;
    c.checkRawData = true;
    c.supportedObjectTypes += GObjectTypes::MULTIPLE_ALIGNMENT;
    c.rawData = IOAdapterUtils::readFileHeader(settings.inputFilePath);
    QList<DocumentFormatId> formats = AppContext::getDocumentFormatRegistry()->selectFormats(c);
    if (formats.isEmpty()) {
        stateInfo.setError(tr("input_format_error"));
        return;
    }

    DocumentFormatId alnFormat = formats.first();
    QVariantMap hints;
    if(alnFormat == BaseDocumentFormats::FASTA){
        hints[DocumentReadingMode_SequenceAsAlignmentHint] = true;
    }
    IOAdapterFactory* iof = AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(IOAdapterUtils::url2io(settings.inputFilePath));
    loadDocumentTask = new LoadDocumentTask(alnFormat, settings.inputFilePath,iof, hints);

    addSubTask(loadDocumentTask);
}
QList<Task*> ClustalOWithExtFileSpecifySupportTask::onSubTaskFinished(Task* subTask) {
    QList<Task*> res;
    if(subTask->hasError()) {
        stateInfo.setError(subTask->getError());
        return res;
    }
    if (hasError() || isCanceled()) {
        return res;
    }
    if (subTask == loadDocumentTask){
        currentDocument=loadDocumentTask->takeDocument();
        SAFE_POINT(currentDocument != NULL, QString("Failed loading document: %1").arg(loadDocumentTask->getURLString()), res);
        SAFE_POINT(currentDocument->getObjects().length() == 1, QString("Number of objects != 1 : %1").arg(loadDocumentTask->getURLString()), res);
        mAObject = qobject_cast<MAlignmentObject*>(currentDocument->getObjects().first());
        SAFE_POINT(mAObject != NULL, QString("MA object not found!: %1").arg(loadDocumentTask->getURLString()), res);
        clustalOSupportTask=new ClustalOSupportTask(mAObject,settings);
        res.append(clustalOSupportTask);
    } else if (subTask == clustalOSupportTask) {
        saveDocumentTask = new SaveDocumentTask(currentDocument, AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(IOAdapterUtils::url2io(settings.outputFilePath)),settings.outputFilePath);
        res.append(saveDocumentTask);
    } else if (subTask == saveDocumentTask) {
        //Project* proj = AppContext::getProject();
        //if (proj == NULL) {
        //    res.append(AppContext::getProjectLoader()->openWithProjectTask(currentDocument->getURLString(), currentDocument->getGHintsMap()));
        //} else {
        //    Document* projDoc = proj->findDocumentByURL(currentDocument->getURL());
        //    if (projDoc != NULL) {
        //        projDoc->setLastUpdateTime();
        //        res.append(new LoadUnloadedDocumentAndOpenViewTask(projDoc));
        //    } else {
        //        // Add document to project
        //        res.append(new AddDocumentAndOpenViewTask(currentDocument));
        //        cleanDoc = false;
        //    }
        //}
        Task* openTask = AppContext::getProjectLoader()->openWithProjectTask(settings.outputFilePath);
        res << openTask;

    }
    return res;
}
Task::ReportResult ClustalOWithExtFileSpecifySupportTask::report(){
    return ReportResult_Finished;
}

////////////////////////////////////////
//ClustalOLogParser
ClustalOLogParser::ClustalOLogParser() : ExternalToolLogParser() {

}
void ClustalOLogParser::parseOutput(const QString& partOfLog){
    lastPartOfLog=partOfLog.split(QChar('\r'));
    lastPartOfLog.first()=lastLine+lastPartOfLog.first();
    lastLine=lastPartOfLog.takeLast();
    foreach(QString buf, lastPartOfLog){
        if(buf.contains("error",Qt::CaseInsensitive)){
            ioLog.error(buf);
            lastError = buf;
        }else{
            ioLog.trace(buf);
        }
    }
}
int ClustalOLogParser::getProgress(){
    if(!lastPartOfLog.isEmpty()){
        QString lastMessage=lastPartOfLog.last();
        //0..10% progresss
        if(lastMessage.contains(QRegExp("Pairwise distance calculation progress: \\d+ %"))){
            QRegExp rx("Pairwise distance calculation progress: (\\d+) %");
            assert(rx.indexIn(lastMessage)>-1);
            rx.indexIn(lastMessage);
            return rx.cap(1).toInt()/10;
        }
        //10..20% progresss
        if(lastMessage.contains(QRegExp("Distance calculation within sub-clusters: \\d+ %"))){
            QRegExp rx("Distance calculation within sub-clusters: (\\d+) %");
            assert(rx.indexIn(lastMessage)>-1);
            rx.indexIn(lastMessage);
            return rx.cap(1).toInt()/10+10;
        }
        //20..100% progresss
        if(lastMessage.contains(QRegExp("Progressive alignment progress: (\\d+) %"))){
            QRegExp rx("Progressive alignment progress: (\\d+) %");
            assert(rx.indexIn(lastMessage)>-1);
            rx.indexIn(lastMessage);
            return rx.cap(1).toInt()*0.8+20;
        }

    }
    return 0;
}
}//namespace
