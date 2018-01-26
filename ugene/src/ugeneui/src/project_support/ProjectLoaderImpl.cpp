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

#include "ProjectLoaderImpl.h"
#include "DocumentFormatSelectorController.h"
#include "DocumentReadingModeSelectorController.h"
#include "MultipleDocumentsReadingModeSelectorController.h"
#include "ProjectTasksGui.h"
#include "ProjectImpl.h"

#include <U2Core/AddDocumentTask.h>
#include <U2Core/Settings.h>
#include <U2Core/ServiceTypes.h>
#include <U2Core/IOAdapter.h>
#include <U2Core/IOAdapterUtils.h>
#include <U2Core/AppContext.h>
#include <U2Core/ProjectModel.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/DocumentUtils.h>
#include <U2Core/CMDLineUtils.h>
#include <U2Core/DocumentImport.h>
#include <U2Core/U2SafePoints.h>
#include <U2Core/L10n.h>
#include <U2Core/CMDLineCoreOptions.h>

#include <U2Gui/LastUsedDirHelper.h>

#include <U2Gui/CreateDocumentFromTextDialogController.h>
#include <U2Gui/DownloadRemoteFileDialog.h>
#include <U2Gui/ObjectViewModel.h>
#include <U2Gui/ProjectView.h>
#include <U2Gui/MainWindow.h>
#include <U2Gui/OpenViewTask.h>

#include <U2View/DnaAssemblyGUIExtension.h>

#include <QtGui/QAction>

namespace U2 {

/* TRANSLATOR U2::ProjectLoaderImpl */

//////////////////////////////////////////////////////////////////////////
/// ProjectLoaderImpl
//////////////////////////////////////////////////////////////////////////

ProjectLoaderImpl::ProjectLoaderImpl() {
    openProjectAction = newProjectAction = separatorAction1 = separatorAction2 = NULL;
	recentProjectsMenu = NULL;

	assert(AppContext::getProject() == NULL);
	assert(AppContext::getProjectLoader() == NULL);

	ServiceRegistry* sr = AppContext::getServiceRegistry();
	connect(sr, SIGNAL(si_serviceStateChanged(Service*, ServiceState)), SLOT(sl_serviceStateChanged(Service*, ServiceState)));
    
    newProjectAction = new QAction(QIcon(":ugene/images/project_new.png"), tr("&New project..."), this);
	newProjectAction->setObjectName(ACTION_PROJECTSUPPORT__NEW_PROJECT);
//    newProjectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    newProjectAction->setShortcutContext(Qt::WindowShortcut);
    connect(newProjectAction, SIGNAL(triggered()), SLOT(sl_newProject()));

    newDocumentFromtext = new QAction(QIcon(), tr("New document from text..."), this);
    newDocumentFromtext->setObjectName("NewDocumentFromText");
    newDocumentFromtext->setShortcutContext(Qt::WindowShortcut);
    connect(newDocumentFromtext, SIGNAL(triggered()), SLOT(sl_newDocumentFromText()));

    openProjectAction = new QAction(QIcon(":ugene/images/project_open.png"), tr("Open"), this);
	openProjectAction->setObjectName(ACTION_PROJECTSUPPORT__OPEN_PROJECT);
    openProjectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    openProjectAction->setShortcutContext(Qt::WindowShortcut);
    connect(openProjectAction, SIGNAL(triggered()), SLOT(sl_openProject()));
    
    downloadRemoteFileAction = new QAction(tr("Access remote database..."), this);
    downloadRemoteFileAction->setObjectName(ACTION_PROJECTSUPPORT__ACCESS_REMOTE_DB);
    downloadRemoteFileAction->setIcon(QIcon(":ugene/images/world_go.png"));
    connect(downloadRemoteFileAction, SIGNAL(triggered()), SLOT(sl_downloadRemoteFile()));

    separatorAction1  = new QAction("-", this);
    separatorAction1->setSeparator(true);

	separatorAction2 = new QAction("-", this);
	separatorAction2->setSeparator(true);

    //add load/close actions to menu and toolbar
    MainWindow* mw = AppContext::getMainWindow();
    QMenu* fileMenu = mw->getTopLevelMenu(MWMENU_FILE);

	recentProjectsMenu = new QMenu(tr("Recent Projects"));
	recentProjectsMenu->menuAction()->setObjectName(ACTION_PROJECTSUPPORT__RECENT_PROJECTS_MENU);
	updateRecentProjectsMenu();

    recentItemsMenu = new QMenu(tr("Recent Files"));
    recentItemsMenu->menuAction()->setObjectName("recent_docs_menu_action");
    updateRecentItemsMenu();

	QList<QAction*> actions;
    actions << newProjectAction << newDocumentFromtext << downloadRemoteFileAction 
        << openProjectAction << separatorAction1 <<  recentItemsMenu->menuAction() 
        << recentProjectsMenu->menuAction() << separatorAction2;
	
	fileMenu->insertActions(fileMenu->actions().first(), actions);

	QToolBar* tb = mw->getToolbar(MWTOOLBAR_MAIN);
	tb->addAction(newProjectAction);
	tb->addAction(openProjectAction);

	updateState();
}


void ProjectLoaderImpl::updateState() {
	recentProjectsMenu->setDisabled(recentProjectsMenu->isEmpty());
}

#define MAX_RECENT_FILES 7


void ProjectLoaderImpl::sl_newProject() {
    QWidget *p = (QWidget*)AppContext::getMainWindow()->getQMainWindow();
    ProjectDialogController d(ProjectDialogController::New_Project,p);
	int rc = d.exec();
	AppContext::getSettings()->setValue(SETTINGS_DIR + "last_dir",d.projectFolderEdit->text());

	if (rc == QDialog::Rejected) {
		updateState();
		return;
	}

	QString fileName = d.projectFolderEdit->text() + "/" + d.projectFileEdit->text();
	if (!fileName.endsWith(PROJECTFILE_EXT)) {
		fileName.append(PROJECTFILE_EXT);
	}
	QFileInfo fi(fileName);
	if (fi.exists()) {
		QFile::remove(fileName);
	}

	QString projectName = d.projectNameEdit->text();
	AppContext::getTaskScheduler()->registerTopLevelTask(new OpenProjectTask(fileName, projectName));
}

void ProjectLoaderImpl::sl_openProject() {
    LastUsedDirHelper h;
    QString filter = DialogUtils::prepareDocumentsFileFilter(true);

    filter.append("\n"+tr("UGENE project file") + " (*" + PROJECTFILE_EXT + ")");

    QStringList files;

#ifdef Q_OS_MAC
    if (qgetenv("UGENE_GUI_TEST").toInt() == 1 && qgetenv("UGENE_USE_NATIVE_DIALOGS").toInt() == 0) {
        files = QFileDialog::getOpenFileNames(QApplication::activeWindow(), tr("Select files to open..."), h.dir,  filter, 0, QFileDialog::DontUseNativeDialog);
    } else
#endif

    files = QFileDialog::getOpenFileNames(QApplication::activeWindow(), tr("Select files to open..."), h.dir,  filter);

    if (files.isEmpty()) {
        return;
    }

    if (QFileInfo(files.first()).exists()) {
        h.url = files.first();
    }
    QList<GUrl> urls;
    foreach(QString file, files) {
        urls << GUrl(file, GUrl_File);
    }
    //updateRecentItemsMenu();
    Task* openTask = openWithProjectTask(urls);
    if (openTask != NULL) {
        AppContext::getTaskScheduler()->registerTopLevelTask(openTask);	
    }
}

void ProjectLoaderImpl::sl_openRecentProject() {
    QAction *action = qobject_cast<QAction *>(sender());
    assert(action);
    QString url = action->data().toString();
    AppContext::getTaskScheduler()->registerTopLevelTask(new OpenProjectTask(url));	
}

void ProjectLoaderImpl::sl_openRecentFile() {
	QAction *action = qobject_cast<QAction *>(sender());
	assert(action);
	GUrl url = action->data().toString();
    Task* task = ProjectLoader::openWithProjectTask(url);
    if (task == NULL) {
        return;
    }
    AppContext::getTaskScheduler()->registerTopLevelTask(task);	
    prependToRecentItems(url.getURLString());
#ifdef Q_OS_LINUX
    if(QString("4.5.0") == qVersion())
    {
        QTimer::singleShot(0,this,SLOT(sl_updateRecentItemsMenu()));
    }
    else
    {
#endif // Q_OS_LINUX
        updateRecentItemsMenu();
#ifdef Q_OS_LINUX
    }
#endif // Q_OS_LINUX
 }

void ProjectLoaderImpl::prependToRecentProjects(const QString& url) {
    assert(!url.isEmpty());
	QStringList recentFiles = AppContext::getSettings()->getValue(SETTINGS_DIR + RECENT_PROJECTS_SETTINGS_NAME).toStringList();
    recentFiles.removeAll(QString()); //remove all empty tokens if fount (a kind of cleanup)
	recentFiles.removeAll(url); // remove URL from the old position
	recentFiles.prepend(url); // make URL first
	while(recentFiles.size() > MAX_RECENT_FILES) {
		recentFiles.pop_back();
	}
	AppContext::getSettings()->setValue(SETTINGS_DIR + RECENT_PROJECTS_SETTINGS_NAME, recentFiles);
}

void ProjectLoaderImpl::updateRecentProjectsMenu() {
    assert(recentProjectsMenu!=NULL);
    recentProjectsMenu->clear();
    QStringList recentFiles =AppContext::getSettings()->getValue(SETTINGS_DIR + RECENT_PROJECTS_SETTINGS_NAME).toStringList();
    Project* p = AppContext::getProject();
    foreach (QString f, recentFiles) {
        if ((p==NULL || f != p->getProjectURL()) && !f.isEmpty()) {
            QAction* a = recentProjectsMenu->addAction(f, this, SLOT(sl_openRecentProject()));
            a->setData(f);
            a->setDisabled(!QFile::exists(f));
        }
    }
}

#define MAX_DOCS_TO_OPEN_VIEWS 5
#define MAX_OBJECT_PER_DOC 5000000

Task* ProjectLoaderImpl::openWithProjectTask(const QList<GUrl>& _urls, const QVariantMap& hints) {
    QList<GUrl> urls = _urls;
    // detect if we open real UGENE project file
    bool projectsOnly = true;
    foreach(const GUrl & url, urls) {
        projectsOnly = projectsOnly && url.lastFileSuffix() == PROJECT_FILE_PURE_EXT;
        if (!projectsOnly) {
            break;
        }
    }
    if (projectsOnly) {
        GUrl projectUrl = urls.isEmpty() ? QString() : urls.last();
        QVariantMap h2 = hints;
        h2[ProjectLoaderHint_CloseActiveProject] = true;
        return createProjectLoadingTask(projectUrl, h2);
    }
	bool abilityUniteDocuments = true;
	
	QVariantMap hintsOverDocuments;
    QMap<QString, qint64> headerSequenceLengths;

    if(urls.size() >= 2){
	    foreach(const GUrl& url, urls){
		    FormatDetectionResult dr;
		    FormatDetectionConfig conf;
		    conf.useImporters = true;
		    conf.bestMatchesOnly = false;
		    QList<FormatDetectionResult> formats = DocumentUtils::detectFormat(url, conf);
		    if(formats.isEmpty()){
			    abilityUniteDocuments = false;
			    break;
		    }
		    dr =  formats[0];
		    bool matchCurrentDocument = MultipleDocumentsReadingModeSelectorController::mergeDocumentOption(dr, &headerSequenceLengths);

		    if(!matchCurrentDocument){
			    abilityUniteDocuments = false;
			    break;
		    }
	    }
    }
    else{
         abilityUniteDocuments = false;
    }

	if(abilityUniteDocuments){
		bool ok  = MultipleDocumentsReadingModeSelectorController::adjustReadingMode(hintsOverDocuments, urls, headerSequenceLengths);
		if(!ok){
			return NULL;
		}
	}
	
    // detect all formats from urls list and add files to project
    QList<AD2P_DocumentInfo> docsInfo;
    QList<AD2P_ProviderInfo> docProviders;
    int nViews = 0;
    foreach(const GUrl& url, urls) {
        if (url.lastFileSuffix() == PROJECT_FILE_PURE_EXT) {
            // skip extra project files
            coreLog.info(tr("Project file '%1' ignored").arg(url.getURLString()));
            continue;
        }
        Project* project = AppContext::getProject();
        Document * doc = project == NULL ? NULL : project->findDocumentByURL(url);
        if (doc != NULL) {
            QWidget *p = AppContext::getMainWindow()->getQMainWindow();
            QString message = tr("The document with the same URL is already added to the project");
            coreLog.details(message);
            QMessageBox::warning(p, tr("warning"), message);
            if (doc->isLoaded()) {
                const QList<GObject*>& docObjects = doc->getObjects();
                QList<GObjectViewWindow*> viewsList = GObjectViewUtils::findViewsWithAnyOfObjects(docObjects);
                if (!viewsList.isEmpty()) {
                    AppContext::getMainWindow()->getMDIManager()->activateWindow(viewsList.first());
                } else {
                    AppContext::getProjectView()->highlightItem(doc);
                }
                coreLog.info(tr("The document is already loaded and added to project: %1").arg(url.fileName()));
            } else if(!doc->isLoaded() && AppContext::getProjectView()) {
                AppContext::getProjectView()->highlightItem(doc);
            }
        } else {
            QList<FormatDetectionResult> formats;
            if(hintsOverDocuments.value(ProjectLoaderHint_MultipleFilesMode_Flag, false).toBool() == false){
                FormatDetectionConfig conf;
                conf.useImporters = true;
                conf.bestMatchesOnly = false;
                formats = DocumentUtils::detectFormat(url, conf);
            }
            else{
                FormatDetectionResult result;
                result.format = AppContext::getDocumentFormatRegistry()->getFormatById(hintsOverDocuments[ProjectLoaderHint_MultipleFilesMode_RealDocumentFormat].toString());
                formats << result;
            }

            if (!formats.isEmpty()) {
                int idx = 0;
                if (formats.size() > 1 && 
                    (formats[0].score() == formats[1].score() 
                    || (formats[1].score() > FormatDetection_AverageSimilarity && formats[0].score() < FormatDetection_Matched) 
                    || (formats[0].score() <= FormatDetection_AverageSimilarity)
                    || (hints.value(ProjectLoaderHint_ForceFormatOptions).toBool() == true))) 
                {
                    idx = DocumentFormatSelectorController::selectResult(url, formats.first().rawData, formats);
                }
                if (idx >= 0) {
                    FormatDetectionResult& dr =  formats[idx];
                    dr.rawDataCheckResult.properties.unite(hints);
					dr.rawDataCheckResult.properties.unite(hintsOverDocuments);
                    if (dr.format != NULL ) {
                        bool forceReadingOptions = hints.value(ProjectLoaderHint_ForceFormatOptions, false).toBool();
                        bool optionsAlreadyChoosed = hints.value((ProjectLoaderHint_MultipleFilesMode_Flag), false).toBool();
                        bool ok = DocumentReadingModeSelectorController::adjustReadingMode(dr, forceReadingOptions, optionsAlreadyChoosed);
                        if (!ok) {
                            continue;
                        }
                        bool documentProcessingFinished = processHints(dr);
                        if (documentProcessingFinished) {
                            continue;
                        }
                        AD2P_DocumentInfo info;
                        if(hints.value(ProjectLoaderHint_LoadWithoutView, true).toBool() == false){
                            info.openView = false;
                        }else{
                            info.openView = nViews++ < MAX_DOCS_TO_OPEN_VIEWS;
                        }
                        if(hints.value(ProjectLoaderHint_LoadUnloadedDocument, true).toBool() == false){
                            info.loadDocuments = false;
                        }
						else{
							info.loadDocuments = true;
						}

                        
                        
                        info.url = url;
                        info.hints = dr.rawDataCheckResult.properties;
                        if (!info.hints.contains(DocumentReadingMode_MaxObjectsInDoc)) {
                            info.hints[DocumentReadingMode_MaxObjectsInDoc] = MAX_OBJECT_PER_DOC;
                        }
                        info.formatId = dr.format->getFormatId(); 
                        info.iof = AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(IOAdapterUtils::url2io(url));
                        docsInfo << info;
                    } else {
                        assert(dr.importer != NULL);
                        AD2P_ProviderInfo info;
                        info.openView = nViews++ < MAX_DOCS_TO_OPEN_VIEWS;
                        QVariantMap hints;
                        info.dp = dr.importer->createImportTask(dr, true, hints);
                        docProviders << info;
                    }
                }
            } else {
                QString message = tr("Failed to detect file format: %1").arg(url.getURLString());
                coreLog.error(message);
                QMessageBox::critical(AppContext::getMainWindow()->getQMainWindow(), L10N::errorTitle(), message);
            }
        }
    }
    if (docsInfo.isEmpty() && docProviders.isEmpty()) {
        return NULL;
    }
    return new AddDocumentsToProjectTask(docsInfo, docProviders);
}


bool ProjectLoaderImpl::processHints(FormatDetectionResult& dr) {
    bool alignAsShortReads = dr.rawDataCheckResult.properties.value(DocumentReadingMode_SequenceAsShortReadsHint).toBool();
    if (alignAsShortReads) {
        DnaAssemblyGUIUtils::runAssembly2ReferenceDialog(QStringList() << dr.url.getURLString());
        return true;
    }
    return false;
}

Task* ProjectLoaderImpl::createNewProjectTask(const GUrl& url) {
    return createProjectLoadingTask(url);
}

Task* ProjectLoaderImpl::createProjectLoadingTask(const GUrl& url, const QVariantMap& hints) {
    Project* p = AppContext::getProject();
    if (p == NULL) {
        return new OpenProjectTask(url.getURLString());
    }
    if (url == p->getProjectURL()) {
        QString message = tr("Project is already opened");
        QMessageBox::critical(AppContext::getMainWindow()->getQMainWindow(),"UGENE", message);
        return NULL;
    }
    QMessageBox msgBox(AppContext::getMainWindow()->getQMainWindow());
    msgBox.setWindowTitle(U2_APP_TITLE);
    msgBox.setText(tr("New project can either be opened in a new window or replace the project in the existing. How would you like to open the project?"));
    QPushButton *newWindow = msgBox.addButton(tr("New Window"), QMessageBox::ActionRole);
    QPushButton *oldWindow = msgBox.addButton(tr("This Window"), QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Abort);
    msgBox.exec();

    if (msgBox.clickedButton() == newWindow) {
        QStringList params =  CMDLineRegistryUtils::getPureValues(0);
        params.append("--" + CMDLineCoreOptions::INI_FILE + "=" + AppContext::getSettings()->fileName());        
        bool b = QProcess::startDetached(params.first(), QStringList() << url.getURLString() << params[1]);
        if (!b) {
            coreLog.error(tr("Failed to open new instance of UGENE"));
        }
        return NULL;
    } else if (msgBox.clickedButton() == oldWindow) {
        bool closeActiveProject = hints.value(ProjectLoaderHint_CloseActiveProject, QVariant::fromValue(false)).toBool();
        if (!closeActiveProject) {
            coreLog.error(tr("Stopped loading project: %1. Reason: active project found").arg(url.getURLString()));
            return NULL;
        }
    } else {
        return NULL;
    }   
    return new OpenProjectTask(url.getURLString());
}


void ProjectLoaderImpl::sl_projectURLChanged(const QString& oldURL) {
    if (!oldURL.isEmpty()) {
        prependToRecentProjects(oldURL);
    }
    rememberProjectURL();
}

void ProjectLoaderImpl::rememberProjectURL() {
    Project* p = AppContext::getProject();
    QString url = p == NULL ? QString() : p->getProjectURL();
    if (!url.isEmpty()) {
        prependToRecentProjects(url);
    }
    updateRecentProjectsMenu();
}

void ProjectLoaderImpl::sl_serviceStateChanged(Service* s, ServiceState prevState) {
    Q_UNUSED(prevState);

	if (s->getType()!=Service_Project) {
		return;
	}
    if (s->isEnabled()) {
        Project* p = AppContext::getProject();
        connect(p, SIGNAL(si_projectURLChanged(const QString&)), SLOT(sl_projectURLChanged(const QString&)));
        connect(p, SIGNAL(si_documentAdded(Document*)), SLOT(sl_documentAdded(Document*)));
    } 
	rememberProjectURL();
    updateState();
}


QString ProjectLoaderImpl::getLastProjectURL() {
    QStringList recentFiles =AppContext::getSettings()->getValue(SETTINGS_DIR + RECENT_PROJECTS_SETTINGS_NAME).toStringList();
    if (!recentFiles.isEmpty()) {
        return recentFiles.first();
    }
    return QString();
}

void ProjectLoaderImpl::prependToRecentItems( const QString& url )
{
    assert(!url.isEmpty());
    QStringList recentFiles =AppContext::getSettings()->getValue(SETTINGS_DIR + RECENT_ITEMS_SETTINGS_NAME).toStringList();
    recentFiles.removeAll(url);
    recentFiles.prepend(url);
    while(recentFiles.size() > MAX_RECENT_FILES) {
        recentFiles.pop_back();
    }
    AppContext::getSettings()->setValue(SETTINGS_DIR + RECENT_ITEMS_SETTINGS_NAME, recentFiles);
    
}

// QT 4.5.0 bug workaround
void ProjectLoaderImpl::sl_updateRecentItemsMenu()
{
    updateRecentItemsMenu();
}

void ProjectLoaderImpl::updateRecentItemsMenu()
{
    assert(recentItemsMenu!=NULL);
    recentItemsMenu->clear();
    QStringList recentFiles =AppContext::getSettings()->getValue(SETTINGS_DIR + RECENT_ITEMS_SETTINGS_NAME).toStringList();
    recentItemsMenu->menuAction()->setEnabled(!recentFiles.isEmpty());
    Project* p = AppContext::getProject();
    foreach (QString f, recentFiles) {
        if ((p==NULL || f != p->getProjectURL()) && !f.isEmpty()) {
            QAction* a = recentItemsMenu->addAction(f, this, SLOT(sl_openRecentFile()));
            a->setData(f);
            a->setDisabled(!QFile::exists(f));
        }
    }

}

void ProjectLoaderImpl::sl_documentAdded( Document* doc )
{
    if (!doc->isModified()) {
        prependToRecentItems(doc->getURLString());
        updateRecentItemsMenu();
    } else {
        connect(doc, SIGNAL(si_modifiedStateChanged()), SLOT(sl_documentStateChanged()));
    }
}

void ProjectLoaderImpl::sl_documentStateChanged() {
    Document* doc = qobject_cast<Document*>( QObject::sender() );
    if (doc != NULL) {
        if (!doc->isModified()) {
            prependToRecentItems(doc->getURLString());
            updateRecentItemsMenu();
        }
    }
}

void ProjectLoaderImpl::sl_newDocumentFromText(){
    QWidget *p = (QWidget*)AppContext::getMainWindow()->getQMainWindow();
    CreateDocumentFromTextDialogController *dialog = new CreateDocumentFromTextDialogController(p);
    dialog->exec();
    delete dialog;
}

void ProjectLoaderImpl::sl_downloadRemoteFile()
{
    QWidget *p = (QWidget*)(AppContext::getMainWindow()->getQMainWindow());
    DownloadRemoteFileDialog dlg(p);
    dlg.exec();
}


//////////////////////////////////////////////////////////////////////////
//ProjectDialogController
//////////////////////////////////////////////////////////////////////////
ProjectDialogController::ProjectDialogController(ProjectDialogController::Mode m, QWidget *p):QDialog(p) {
    setupUi(this);
    setModal(true);
    fileEditIsEmpty = false;
    QString lastDir =AppContext::getSettings()->getValue(SETTINGS_DIR + "last_dir", QString("")).toString();
    projectFolderEdit->setText(lastDir);

    if (m == Save_Project) {
        setWindowTitle(ProjectLoaderImpl::tr("Save project as"));
        createButton->setText(ProjectLoaderImpl::tr("Save"));
        titleLabel->setVisible(false);
        resize(width(), height() - titleLabel->pixmap()->height());
        frame->setFrameShape(QFrame::NoFrame);
        projectNameEdit->setText(AppContext::getProject()->getProjectName());
        QString url = AppContext::getProject()->getProjectURL();
        if (!url.isEmpty()) {
            QFileInfo fi(url);
            projectFileEdit->setText(fi.completeBaseName());
            projectFolderEdit->setText(fi.absolutePath());
        } else {
            setupDefaults();
        }
    } else {
        setupDefaults();

    }
    //projectFolderEdit->setReadOnly(true);
    if (projectFileEdit->text().isEmpty()) {
        fileEditIsEmpty = true;
    }
    connect(folderSelectButton, SIGNAL(clicked()), SLOT(sl_folderSelectClicked()));
    connect(projectFileEdit, SIGNAL(textEdited(const QString&)), SLOT(sl_fileNameEdited(const QString&)));
    connect(projectNameEdit, SIGNAL(textEdited(const QString&)), SLOT(sl_projectNameEdited(const QString&)));
    updateState();
}

void ProjectDialogController::updateState() { 
    bool ready = true;
    
    const QString& folder = projectFolderEdit->text();
    const QString& file = projectFileEdit->text();
    const QString& name = projectNameEdit->text();
    
    //todo: improve check
    if (folder.isEmpty() || file.isEmpty() || name.isEmpty()) {
        ready = false;
    }
    createButton->setEnabled(ready);
}

void ProjectDialogController::keyPressEvent(QKeyEvent* event) {
    int key = event->key();
    if (event->modifiers() == Qt::NoModifier && (key == Qt::Key_Enter || key == Qt::Key_Return)) {
        createButton->animateClick();
    }
    QDialog::keyPressEvent(event);
    
}

void ProjectDialogController::sl_folderSelectClicked() {
    QString folder = QFileDialog::getExistingDirectory(this, tr("Choose directory"), projectFolderEdit->text());
    if (folder.isEmpty()) return;
    projectFolderEdit->setText(folder);
    updateState();
}

void ProjectDialogController::sl_fileNameEdited(const QString&) {
	//TODO: warn about overwrite
    fileEditIsEmpty = false;
    updateState();
}

void ProjectDialogController::sl_projectNameEdited(const QString& text) {
    if (fileEditIsEmpty) {
        projectFileEdit->setText(text);
    }
    updateState();
    
}

void ProjectDialogController::setupDefaults()
{
    projectNameEdit->setText(ProjectLoaderImpl::tr("new_project_default_name"));
    projectFolderEdit->setText(QDir::home().absolutePath());
    projectFileEdit->setText(ProjectLoaderImpl::tr("new_project_default_file"));
}

void ProjectDialogController::accept()
{
    QString projUrl = projectFolderEdit->text() + "/" + projectFileEdit->text()+".uprj";
    QFileInfo info(projUrl);
    QString absPath = info.absoluteFilePath();
    if (info.exists()) {
        if (QMessageBox::Yes != QMessageBox::question(this, windowTitle(), 
            tr("<html><body align=\"center\"><br>Project file already exists.<br>Are you sure you want to overwrite it?<body></html>"),
            QMessageBox::Yes, QMessageBox::No) ) {
                return;
        } 
    }
    QDialog::accept();
}

Project* ProjectLoaderImpl::createProject(const QString& name, const QString& url, QList<Document*>& documents, QList<GObjectViewState*>& states) {
    ProjectImpl* pi = new ProjectImpl(name, url, documents, states);
    return pi;
}


//////////////////////////////////////////////////////////////////////////
// Add documents to project task

AddDocumentsToProjectTask::AddDocumentsToProjectTask(const QList<AD2P_DocumentInfo>& _docsInfo, const QList<AD2P_ProviderInfo>& _provInfo) 
: Task(tr("Loading documents"), TaskFlags_NR_FOSE_COSC), docsInfo(_docsInfo), providersInfo(_provInfo), loadTasksAdded(false)
{    
    setMaxParallelSubtasks(MAX_PARALLEL_SUBTASKS_AUTO);

    Project* p = AppContext::getProject();
    if (!p) {
        // create anonymous project
        Task* rpt = AppContext::getProjectLoader()->createNewProjectTask();
        rpt->setSubtaskProgressWeight(0);
        addSubTask(rpt);
    } else {
        QList<Task*> tasks = prepareLoadTasks();
        foreach(Task* t, tasks) {
            addSubTask(t);
        }
        loadTasksAdded = true;
    }
}

AddDocumentsToProjectTask::~AddDocumentsToProjectTask() {
    if (!loadTasksAdded) {
        foreach(const AD2P_ProviderInfo& info, providersInfo) {
            delete info.dp;
        }
    }
}

QList<Task*> AddDocumentsToProjectTask::onSubTaskFinished(Task* t) {
    QList<Task*> res;
    if (!loadTasksAdded) {
        res = prepareLoadTasks();
        loadTasksAdded = true;
    } else if (t->hasError()) {
        coreLog.error(t->getError());
    }
    return res;   
};

QList<Task*> AddDocumentsToProjectTask::prepareLoadTasks() {
    QList<Task*> res;

    Project* p = AppContext::getProject();
    SAFE_POINT(p != NULL, tr("No active project found!"), res);

    foreach(const AD2P_DocumentInfo& info, docsInfo) {
        Document* doc = p->findDocumentByURL(info.url);
        bool unsupportedObjectType = false;
        if (doc == NULL) {
            DocumentFormat* df = AppContext::getDocumentFormatRegistry()->getFormatById(info.formatId);
            GObjectType t = df->getSupportedObjectTypes().toList().first();
            if (GObjectTypes::getTypeInfo(t).type == GObjectTypes::UNKNOWN) {
                unsupportedObjectType = true;
            }
            U2OpStatus2Log os;
            doc = df->createNewUnloadedDocument(info.iof, info.url, os, info.hints);
            if (doc == NULL) {
                continue;
            }
        }
        if (unsupportedObjectType) {
            if (info.openView) {
                res << new LoadUnloadedDocumentAndOpenViewTask(doc);
            } else {
                coreLog.trace(QString("View limit exceed for the document: %1").arg(info.url.getURLString()));
                delete doc;
            }
        } else {
            if (info.openView) {
                res << new AddDocumentAndOpenViewTask(doc);
            } else {
                res<<new AddDocumentTask(doc);
                if(info.loadDocuments){
                    res<<new LoadUnloadedDocumentTask(doc);
                }
            }
        }
        
    }

    AddDocumentTaskConfig conf;
    conf.unloadExistingDocument = true;// -> re-import kills old document version
    foreach(const AD2P_ProviderInfo& info, providersInfo) {
        if (info.openView) {
            res << new AddDocumentAndOpenViewTask(info.dp, conf);
        } else {
            res << new AddDocumentTask(info.dp, conf);
        }
    }

    return res;
}

OpenWithProjectTask::OpenWithProjectTask(const QStringList& _urls) 
: Task(tr(""), TaskFlags_NR_FOSCOE)
{
    foreach(const QString& u, _urls) {
        urls << GUrl(u);
    }

    if (urls.size() == 1) {
        setTaskName(tr("Opening document: %1").arg(urls.first().getURLString()));
    } else {
        setTaskName(tr("Opening %1 documents").arg(urls.size()));
    }
}

void OpenWithProjectTask::prepare() {
    Task * t = AppContext::getProjectLoader()->openWithProjectTask(urls);
    if (t != NULL) {
        addSubTask(t);
    }
}


}//namespace
