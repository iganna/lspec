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

#ifndef _U2_DNA_ASSEMBLEY_GUI_EXTENSION_H_
#define _U2_DNA_ASSEMBLEY_GUI_EXTENSION_H_


#include <U2Core/global.h>
#include <U2Core/GUrl.h>
#include <U2Core/Task.h>

#include <QtGui/QWidget>

namespace U2 {


// These classes are intended for extending standard Dna Assembly dialog GUI
// with options specific to the assembly algorithm

class DnaAssemblyAlgorithmMainWidget : public QWidget {
public:
    DnaAssemblyAlgorithmMainWidget(QWidget* parent) : QWidget(parent) {}
    virtual QMap<QString,QVariant> getDnaAssemblyCustomSettings() = 0;
    virtual bool isParametersOk(QString &error) = 0;
    virtual bool buildIndexUrl(const GUrl&, bool, QString &) {return true;};
    virtual void prebuiltIndex(bool) {};
    virtual bool isIndexOk(QString &, GUrl) {return true;};
};

class DnaAssemblyAlgorithmBuildIndexWidget : public QWidget {
public:
    DnaAssemblyAlgorithmBuildIndexWidget(QWidget* parent) : QWidget(parent) {}
    virtual QMap<QString,QVariant> getBuildIndexCustomSettings() = 0;
    virtual QString getIndexFileExtension() = 0;
    virtual void buildIndexUrl(const GUrl& url) = 0;
};

class DnaAssemblyGUIExtensionsFactory {
public:
    virtual DnaAssemblyAlgorithmMainWidget* createMainWidget(QWidget* parent) = 0;
    virtual DnaAssemblyAlgorithmBuildIndexWidget* createBuildIndexWidget(QWidget* parent) = 0;
    virtual bool hasMainWidget() = 0;
    virtual bool hasBuildIndexWidget() = 0;
};

class U2VIEW_EXPORT DnaAssemblyGUIUtils : public QObject {
public:
    static void runAssembly2ReferenceDialog(const QStringList& shortReadUrls = QStringList(), const QString& refSeqUrl = QString());
};

} // U2


#endif // _U2_DNA_ASSEMBLEY_GUI_EXTENSION_H_
