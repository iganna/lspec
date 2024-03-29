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

#ifndef _U2_TCOFFEE_SUPPORT_TASK_H
#define _U2_TCOFFEE_SUPPORT_TASK_H

#include <U2Core/Task.h>
#include <U2Core/IOAdapter.h>

#include <U2Core/LoadDocumentTask.h>
#include <U2Core/SaveDocumentTask.h>
#include "utils/ExportTasks.h"

#include <U2Core/MAlignmentObject.h>

#include "ExternalToolRunTask.h"

namespace U2 {

class TCoffeeLogParser;
class TCoffeeSupportTaskSettings {
public:
    TCoffeeSupportTaskSettings() {reset();}
    void reset();

    float   gapOpenPenalty;
    float   gapExtenstionPenalty;
    int     numIterations;
    QString inputFilePath;
    QString outputFilePath;
};


class TCoffeeSupportTask : public Task {
    Q_OBJECT
public:
    TCoffeeSupportTask(MAlignmentObject* _mAObject, const TCoffeeSupportTaskSettings& settings);
    void prepare();
    Task::ReportResult report();

    QList<Task*> onSubTaskFinished(Task* subTask);

    MAlignment                  resultMA;
private:
    MAlignmentObject*           mAObject;
    Document*                   currentDocument;
    Document*                   newDocument;
    QString                     url;
    TCoffeeLogParser*           logParser;

    SaveMSA2SequencesTask*      saveTemporaryDocumentTask;
    ExternalToolRunTask*        tCoffeeTask;
    LoadDocumentTask*           loadTmpDocumentTask;
    TCoffeeSupportTaskSettings  settings;
};

class TCoffeeWithExtFileSpecifySupportTask : public Task {
    Q_OBJECT
public:
    TCoffeeWithExtFileSpecifySupportTask(const TCoffeeSupportTaskSettings& settings);
    ~TCoffeeWithExtFileSpecifySupportTask();
    void prepare();
    Task::ReportResult report();

    QList<Task*> onSubTaskFinished(Task* subTask);
private:
    MAlignmentObject*           mAObject;
    Document*                   currentDocument;
    bool                        cleanDoc;

    SaveDocumentTask*               saveDocumentTask;
    LoadDocumentTask*               loadDocumentTask;
    TCoffeeSupportTask*             tCoffeeSupportTask;
    TCoffeeSupportTaskSettings      settings;
};

class TCoffeeLogParser : public ExternalToolLogParser {
public:
    TCoffeeLogParser();

    int getProgress();
    void parseOutput(const QString& partOfLog);
    void parseErrOutput(const QString& partOfLog);

private:
    QString lastErrLine;
    int     progress;
};

}//namespace
#endif // _U2_TCOFFEE_SUPPORT_TASK_H
