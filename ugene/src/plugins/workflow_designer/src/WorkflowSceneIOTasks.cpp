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

#include <QtXml/qdom.h>

#include <U2Core/AppContext.h>
#include <U2Core/Settings.h>
#include <U2Core/Counter.h>
#include <U2Core/L10n.h>
#include <U2Core/DocumentModel.h>
#include <U2Lang/HRSchemaSerializer.h>

#include "WorkflowViewController.h"
#include "HRSceneSerializer.h"
#include "SceneSerializer.h"
#include "WorkflowDocument.h"
#include "WorkflowSceneIOTasks.h"

namespace U2 {
using namespace Workflow;

/**********************************
 * SaveWorkflowSceneTask
 **********************************/
const QString SaveWorkflowSceneTask::SCHEMA_PATHS_SETTINGS_TAG = "workflow_settings/schema_paths";

SaveWorkflowSceneTask::SaveWorkflowSceneTask(WorkflowScene* s, const Metadata& m)
: Task(tr("Save workflow scene task"), TaskFlag_None), scene(s), meta(m) {
    GCOUNTER(cvar,tvar,"SaveWorkflowSceneTask");
    assert(scene != NULL);
    rawData = HRSceneSerializer::scene2String(scene, meta);
    
    // add ( name, path ) pair to settings. need for running schemas in cmdline by name
    Settings * settings = AppContext::getSettings();
    assert( settings != NULL );
    QVariantMap pathsMap = settings->getValue( SCHEMA_PATHS_SETTINGS_TAG ).toMap();
    pathsMap.insert(meta.name, meta.url);
    settings->setValue( SCHEMA_PATHS_SETTINGS_TAG, pathsMap );
}

void SaveWorkflowSceneTask::run() {
    if(hasError()) {
        return;
    }
    
    QFile file(meta.url);
    if(!file.open(QIODevice::WriteOnly)) {
        setError(L10N::errorOpeningFileWrite(meta.url));
        return;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << rawData;
}

Task::ReportResult SaveWorkflowSceneTask::report() {
    if (!stateInfo.hasError() && !scene.isNull()) {
        scene->setModified(false);
    }
    return ReportResult_Finished;
}

/**********************************
 * LoadWorkflowSceneTask
 **********************************/
LoadWorkflowSceneTask::LoadWorkflowSceneTask(WorkflowScene* s, Workflow::Metadata* m, const QString& u):
Task(tr("Load workflow scene"),TaskFlag_None), scene(s), meta(m), url(u) {
    GCOUNTER(cvar,tvar, "LoadWorkflowSceneTask");
    assert(scene != NULL);
    assert(meta != NULL);
}

void LoadWorkflowSceneTask::run() {
    QFile file(url);
    if(!file.open(QIODevice::ReadOnly)) {
        setError(L10N::errorOpeningFileRead(url));
        return;
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    rawData = in.readAll();
    format = LoadWorkflowTask::detectFormat(rawData);
    if(format == LoadWorkflowTask::UNKNOWN) {
        setError(tr("Undefined format: plain text or xml expected"));
        return;
    }
}

Task::ReportResult LoadWorkflowSceneTask::report() {
    if(hasError()) {
        return ReportResult_Finished;
    }
    
    QString err;
    if (!scene->items().isEmpty()) {
        scene->clearScene();
    }
    if(format == LoadWorkflowTask::HR) {
        err = HRSceneSerializer::string2Scene(rawData, scene, meta);
    } else if(format == LoadWorkflowTask::XML) {
        QDomDocument xml;
        QMap<ActorId, ActorId> remapping;
        xml.setContent(rawData);
        err = SceneSerializer::xml2scene(xml.documentElement(), scene, remapping);
        SchemaSerializer::readMeta(meta, xml.documentElement());
        scene->setIterations(QList<Iteration>());
        scene->setModified(false);
        meta->url = url;
    } else {
        // cause check for errors in the begin
        assert(false);
    }
    
    if(!err.isEmpty()) {
        setError(tr("Error while parsing file: %1").arg(err));
        scene->sl_reset();
        meta->reset();
        return ReportResult_Finished;
    }
    scene->setModified(false);
    scene->connectConfigurationEditors();
    meta->url = url;
    return ReportResult_Finished;
}

}//namespace
