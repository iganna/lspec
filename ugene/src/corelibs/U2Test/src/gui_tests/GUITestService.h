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

#ifndef _U2_GUI_TESTS_VIEWER_H_
#define _U2_GUI_TESTS_VIEWER_H_

#include <U2Core/global.h>
#include <U2Core/Task.h>
#include <U2Core/MultiTask.h>
#include <U2Gui/MainWindow.h>

#include <QtGui>

#include "GUITest.h"
#include "GUITestLauncher.h"


namespace U2 {

class GUITestLauncher;
class GUITestService;
class CMDLineRegistry;

class U2TEST_EXPORT GUITestService: public Service {
    Q_OBJECT
public:
    enum LaunchOptions {NONE, RUN_ONE_TEST, RUN_ALL_TESTS, RUN_ALL_TESTS_BATCH};

    GUITestService(QObject *parent = NULL);
    virtual ~GUITestService();

    void runTest(GUITests testsToRun);

public slots:
    // returns test error string or successResult
    static void runGUITest();
    static void runGUITest(GUITest* t);

    static void runAllGUITests();

protected:
    virtual void serviceStateChangedCallback(ServiceState oldState, bool enabledStateChanged);

    static GUITests preChecks();
    static GUITests postChecks();

protected slots:
    void sl_registerService();
    void sl_registerTestLauncherTask();
    void sl_taskStateChanged(Task*);

private:
    static void clearSandbox();

    LaunchOptions getLaunchOptions(CMDLineRegistry* cmdLine) const;

    void registerAllTestsTask();
    void registerServiceTask();

    void addServiceMenuItem();
    void deleteServiceMenuItem();

    Task* createTestLauncherTask() const;
    static void writeTestResult(const QString &result);

    void setQtFileDialogView();

    QAction *runTestsAction;
    Task *testLauncher;
    GUITests testsToRun;
};

} // U2

#endif
