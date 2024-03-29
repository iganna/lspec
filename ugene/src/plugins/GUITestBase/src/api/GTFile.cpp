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

#include "GTFile.h"
#include <QtCore/QFile>

namespace U2 {

#define GT_CLASS_NAME "GTFile"

const QString GTFile::backupPostfix = "_GT_backup";

#define GT_METHOD_NAME "equals"
bool GTFile::equals(U2OpStatus &os, const QString& path1, const QString& path2) {

    QFile f1(path1);
    QFile f2(path2);

    bool ok = f1.open(QIODevice::ReadOnly) && f2.open(QIODevice::ReadOnly);
    GT_CHECK_RESULT(ok, f1.errorString() + " " + f2.errorString(), false);

    QByteArray byteArray1 = f1.readAll();
    QByteArray byteArray2 = f2.readAll();

    GT_CHECK_RESULT((f1.error() == QFile::NoError) && (f2.error() == QFile::NoError), f1.errorString() + " " + f2.errorString(), false);

    return byteArray1 == byteArray2;
}
#undef GT_METHOD_NAME

#define GT_METHOD_NAME "copy"
void GTFile::copy(U2OpStatus &os, const QString& from, const QString& to) {

    QFile f2(to);
    bool ok = f2.open(QIODevice::ReadOnly);
    if (ok) {
        f2.remove();
    }

    bool copied = QFile::copy(from, to);
    GT_CHECK(copied == true, "can't copy <" + from + "> to <" + to + ">");
}
#undef GT_METHOD_NAME

#define GT_METHOD_NAME "backup"
void GTFile::backup(U2OpStatus &os, const QString& path) {

    copy(os, path, path + backupPostfix);
}
#undef GT_METHOD_NAME

#define GT_METHOD_NAME "restore"
void GTFile::restore(U2OpStatus &os, const QString& path) {

    QFile backupFile(path + backupPostfix);

    bool ok = backupFile.open(QIODevice::ReadOnly);
    GT_CHECK(ok, "There is no backup file for <" + path + ">");

    QFile file(path);
    ok = file.open(QIODevice::ReadOnly);
    if (ok) {
        file.remove();
    }

    bool renamed = backupFile.rename(path);
    GT_CHECK(renamed == true, "restore of <" + path + "> can't be done");
}
#undef GT_METHOD_NAME

#undef GT_CLASS_NAME

} //namespace
