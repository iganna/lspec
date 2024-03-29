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
#include <U2Core/ProjectModel.h>
#include <U2Core/Log.h>
#include <U2Core/IOAdapter.h>
#include <U2Core/IOAdapterUtils.h>
#include <U2Core/GObject.h>
#include <U2Core/BaseDocumentFormats.h>
#include <U2Core/Counter.h>
#include <U2Core/DNASequenceObject.h>
#include <U2Core/U2SequenceUtils.h>
#include <U2Core/MultiTask.h>
#include <U2Core/AddDocumentTask.h>
#include <U2Core/SaveDocumentTask.h>

#include <U2Core/AnnotationTableObject.h>
#include <U2Core/GObjectRelationRoles.h>
#include <U2Core/GObjectUtils.h>
#include <U2Core/U1AnnotationUtils.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/U2SafePoints.h>

#include "ModifySequenceObjectTask.h"

namespace U2 {

ModifySequenceContentTask::ModifySequenceContentTask(const DocumentFormatId& _dfId, 
                                                     U2SequenceObject *_seqObj, 
                                                     const U2Region& _regionTodelete, 
                                                     const DNASequence& seq2Insert,
                                                     U1AnnotationUtils::AnnotationStrategyForResize _str, 
                                                     const GUrl& _url, 
                                                     bool _mergeAnnotations )

:Task(tr("Modify sequence task"), TaskFlag_NoRun), resultFormatId(_dfId), mergeAnnotations(_mergeAnnotations),
curDoc(_seqObj->getDocument()), newDoc(NULL), url(_url), strat(_str), seqObj(_seqObj), 
regionToReplace(_regionTodelete), sequence2Insert(seq2Insert) 
{
    GCOUNTER( cvar, tvar, "Modify sequence task" );
    inplaceMod = url == curDoc->getURL() || url.isEmpty();
}

Task::ReportResult ModifySequenceContentTask::report(){
    CHECK(!(regionToReplace.isEmpty() && sequence2Insert.seq.isEmpty()), ReportResult_Finished);
    CHECK_EXT(!curDoc->isStateLocked(), setError(tr("Document is locked")), ReportResult_Finished);
        
    U2Region seqRegion(0, seqObj->getSequenceLength());
    if (!seqRegion.contains(regionToReplace)){
        algoLog.error(tr("Region to delete is larger than the whole sequence"));
        return ReportResult_Finished;
    }

    Project *p = AppContext::getProject();
    if (p != NULL){
        if (p->isStateLocked()){
            return ReportResult_CallMeAgain;
        }
        docs = p->getDocuments();
    }
    
    if (!docs.contains(curDoc)){
        docs.append(curDoc);
    }
    
    if (!inplaceMod){
        cloneSequenceAndAnnotations();
    }
    seqObj->replaceRegion(regionToReplace, sequence2Insert, stateInfo);
    CHECK_OP(stateInfo, ReportResult_Finished);

    fixAnnotations();
    
    if (!inplaceMod) {
        QList<Task*> tasks;
        IOAdapterFactory* iof = AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(IOAdapterUtils::url2io(url));
        tasks.append(new SaveDocumentTask(seqObj->getDocument(), iof, url.getURLString()));              
        Project *p = AppContext::getProject();
        if (p != NULL){
            tasks.append(new AddDocumentTask(newDoc));
        }
        AppContext::getTaskScheduler()->registerTopLevelTask(new MultiTask("Save document and add it to project (optional)", tasks));
    } 
    return ReportResult_Finished;
}


void ModifySequenceContentTask::fixAnnotations(){
    foreach(Document *d, docs){
        QList<GObject*> annotationTablesList = d->findGObjectByType(GObjectTypes::ANNOTATION_TABLE);
        foreach(GObject *table, annotationTablesList){
            AnnotationTableObject *ato = qobject_cast<AnnotationTableObject*>(table);
            if (ato->hasObjectRelation(seqObj, GObjectRelationRole::SEQUENCE)){
                QList<Annotation*> annList = ato->getAnnotations();
                foreach(Annotation *an, annList){
                    QVector<U2Region> locs = an->getRegions();

                    QList< QVector<U2Region> > newRegions = U1AnnotationUtils::fixLocationsForReplacedRegion(regionToReplace, sequence2Insert.seq.length(), an->getRegions(), strat);

                    if (newRegions[0].isEmpty()) {
                        ato->removeAnnotation(an);
                    }
                    else{
                        an->replaceRegions(newRegions[0]);

                        for (int i = 1; i < newRegions.size(); i++) {
                            Annotation* copy = new Annotation(an->data());
                            QStringList groupNames = an->getFullGroupsNames();
                            ato->addAnnotation(copy, groupNames);
                        }
                    }
                }
            }
        }
    }
}


void ModifySequenceContentTask::cloneSequenceAndAnnotations(){
    IOAdapterFactory* iof = AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(IOAdapterUtils::url2io(url));
    DocumentFormat *df = AppContext::getDocumentFormatRegistry()->getFormatById(resultFormatId);
    if (iof == NULL) {
        return;
    }
    U2SequenceObject *oldSeqObj = seqObj;
    U2OpStatus2Log os;
    newDoc = df->createNewLoadedDocument(iof, url, os, curDoc->getGHintsMap());
    SAFE_POINT_EXT(df->isObjectOpSupported(newDoc, DocumentFormat::DocObjectOp_Add, GObjectTypes::SEQUENCE), stateInfo.setError("Failed to add sequence object to document!"), );
    U2Sequence clonedSeq = U2SequenceUtils::copySequence(oldSeqObj->getSequenceRef(), newDoc->getDbiRef(), stateInfo); 
    CHECK_OP(stateInfo, );
    U2SequenceObject* seqObj = new U2SequenceObject(oldSeqObj->getGObjectName(), U2EntityRef(newDoc->getDbiRef(), clonedSeq.id), oldSeqObj->getGHintsMap());
    newDoc->addObject(seqObj);

    if (df->isObjectOpSupported(newDoc, DocumentFormat::DocObjectOp_Add, GObjectTypes::ANNOTATION_TABLE)) {
        if (mergeAnnotations) {
            AnnotationTableObject *newDocAto = new AnnotationTableObject("Annotations");
            newDocAto->addObjectRelation(seqObj, GObjectRelationRole::SEQUENCE);

            foreach(Document *d, docs){
                QList<GObject*> annotationTablesList = d->findGObjectByType(GObjectTypes::ANNOTATION_TABLE);
                foreach(GObject *table, annotationTablesList){
                    AnnotationTableObject *ato = (AnnotationTableObject*)table;
                    if (ato->hasObjectRelation(oldSeqObj, GObjectRelationRole::SEQUENCE)){
                        foreach(Annotation *ann, ato->getAnnotations()){
                            QStringList groupNames;
                            foreach(AnnotationGroup* gr,ann->getGroups()){
                                groupNames.append(gr->getGroupName());
                            }
                            newDocAto->addAnnotation(new Annotation(ann->data()), groupNames);
                        }
                    }
                }
            }
            newDoc->addObject(newDocAto);

        } else {
            // use only sequence-doc annotations
            foreach(GObject* o, curDoc->getObjects()){
                AnnotationTableObject* aObj = qobject_cast<AnnotationTableObject*>(o);
                if (aObj == NULL) {
                    continue;
                }
                U2OpStatus2Log os;
                GObject *cl = aObj->clone(newDoc->getDbiRef(), os);
                newDoc->addObject(cl);
                GObjectUtils::updateRelationsURL(cl, curDoc->getURL(), newDoc->getURL());
            }
        }
    }
    docs.append(newDoc);
}


}//ns
