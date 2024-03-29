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

#pragma once

#include <ui/ui_TreeSettingsDialog.h>
#include <QtGui/QDialog>

namespace U2 {

struct TreeSettings
{
public:
    TreeSettings();

    enum TREE_TYPE { PHYLOGRAM, CLADOGRAM };

    TREE_TYPE type;
    int width_coef;
    int height_coef;

    static int default_width_coef;
    static int default_height_coef;
};

class TreeSettingsDialog : public QDialog, public Ui_TreeSettingsDialog{
    Q_OBJECT
public:
    TreeSettingsDialog(QWidget *parent, const TreeSettings &treeSettings, bool isRectLayout);
    
    virtual void accept();
    TreeSettings getSettings() const;

protected slots:
   // void sl_colorButton();

private:
    static QString treePhylogramText() { return tr("Phylogram"); }
    static QString treeCladogramText() { return tr("Cladogram"); }


    TreeSettings settings, changedSettings;

};

} //namespace


