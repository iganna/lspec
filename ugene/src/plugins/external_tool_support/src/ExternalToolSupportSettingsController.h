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

#ifndef _U2_EXTERNAL_TOOL_SUPPORT_SETTINGS_CONTROLLER_H
#define _U2_EXTERNAL_TOOL_SUPPORT_SETTINGS_CONTROLLER_H

#include <U2Gui/AppSettingsGUI.h>
#include <U2Core/ExternalToolRegistry.h>
#include <ui/ui_ETSSettingsWidget.h>

#include <QtGui/QLineEdit>

namespace U2
{
#define ExternalToolSupportSettingsPageId QString("ets")
struct ExternalToolInfo{
    QString name;
    QString path;
    QString description;
    QString version;
    bool    valid;
};

class ExternalToolSupportSettingsPageController : public AppSettingsGUIPageController {
    Q_OBJECT
public:
    ExternalToolSupportSettingsPageController(QObject* p = NULL);

    AppSettingsGUIPageState* getSavedState();
    void saveState(AppSettingsGUIPageState* s);
    AppSettingsGUIPageWidget* createWidget(AppSettingsGUIPageState* state);

};

class ExternalToolSupportSettingsPageState : public AppSettingsGUIPageState {
    Q_OBJECT
public:
    QList<ExternalTool*>    externalTools;
};

class ExternalToolSupportSettingsPageWidget: public AppSettingsGUIPageWidget, public Ui_ETSSettingsWidget {
    Q_OBJECT
public:
    ExternalToolSupportSettingsPageWidget(ExternalToolSupportSettingsPageController* ctrl);

    virtual void setState(AppSettingsGUIPageState* state);

    virtual AppSettingsGUIPageState* getState(QString& err) const;
private:
    QWidget* createPathEditor(QWidget *parent, const QString& path) const;
    void insertChild(QTreeWidgetItem* rootItem, QString name, int pos);
private slots:
    void sl_toolPathCanged();//QString path);
    void sl_validateTaskStateChanged();
    void sl_itemSelectionChanged();
    void sl_onPathEditWidgetClick();
    void sl_onBrowseToolKitPath();
    void sl_onBrowseToolPackPath();
    void sl_linkActivated(QString);
private:
    QMap<QString, ExternalToolInfo> externalToolsInfo;
    mutable int buttonsWidth;
};

class PathLineEdit : public QLineEdit {
    Q_OBJECT
public:
    PathLineEdit(const QString& filter, const QString& type, bool multi, QWidget *parent)
        : QLineEdit(parent), FileFilter(filter), type(type), multi(multi) {}

private slots:
    void sl_onBrowse();
    void sl_clear();

private:
    QString FileFilter;
    QString type;
    bool    multi;
    QString path;
};
}//namespace

#endif // _U2_EXTERNAL_TOOL_SUPPORT_SETTINGS_CONTROLLER_H
