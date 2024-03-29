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

#ifndef _U2_OPTIONS_PANEL_H_
#define _U2_OPTIONS_PANEL_H_

#include "OptionsPanelGroup.h"
#include "OptionsPanelWidget.h"

#include <U2Core/global.h>

#include <U2Gui/ObjectViewModel.h>

#include <U2Gui/OPWidgetFactory.h>


namespace U2 {

/**
 * Provides interface between instances that want to use an options panel and
 * low-level implementation of the panel (including widgets, their styles, etc.)
 * To use this class add the Options Panel's main widget to a layout and add the required groups.
 */
class U2GUI_EXPORT OptionsPanel : public QObject
{
    Q_OBJECT
public:
    /** Creates a new OptionsPanelWidget */
    OptionsPanel(GObjectView*);

    /**
     * Normally, the OptionsPanelWidget is added to another widget and should be deleted
     * when this widget is deleted, but if this hasn't happened by some reason, then
     * the destructor deletes the object.
     */
    ~OptionsPanel();

    /** Add a new options panel group instance and corresponding widgets*/
    void addGroup(OPWidgetFactory* factory);
    //void addGroup(const QPixmap& headerImage, const QString& title, QWidget* optionsWidget);

    /** Returns the main Options Panel widget */
    QWidget* getMainWidget();

    /** Open a group with the specified group ID */
    void openGroupById(const QString& groupId);


public slots:
    /** Catches signals that a group header has been pressed
        and implements the behavior of groups selection (one or several groups at a time) */
    void sl_groupHeaderPressed(QString groupId, bool ctrlHold);


private:
    GObjectView* objView;

    /** Shows the options widget */
    void openOptionsGroup(const QString& groupId);

    /** Hides the options widget  */
    void closeOptionsGroup(const QString& groupId);

    /** Returns the Options Panel widget factory by the specified groupId, or NULL. */
    OPWidgetFactory* findFactoryByGroupId(const QString& groupId);

    /** All added groups */
    QList<OPWidgetFactory*> opWidgetFactories;

    /** The widget that displays options groups */
    OptionsPanelWidget* widget;

    /** IDs of opened groups */
    QList<QString> activeGroupsIds;
};

} // namespace


#endif
