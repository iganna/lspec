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

#ifndef _U2_QVARUTILS_H_
#define _U2_QVARUTILS_H_

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <assert.h>
#include <U2Core/global.h>
#include <U2Core/AnnotationData.h>

namespace U2 {

class U2CORE_EXPORT QVariantUtils {
public:
    static QVariant String2Var(const QString& string) {
        QDataStream s(QByteArray::fromBase64(string.toAscii()));
        return QVariant(s);
    }

    static QVariantMap string2Map(const QString& string, bool emptyMapIfError) {
        QDataStream s(QByteArray::fromBase64(string.toAscii()));
        QVariant res(QVariant::Map);
        s >> res;
        if (res.type() == QVariant::Map) {
            return res.toMap();
        }
        assert(emptyMapIfError); Q_UNUSED(emptyMapIfError);
        return QVariantMap();
    }

    static QString var2String(const QVariant& v) {
        QByteArray a;
        QDataStream s(&a, QIODevice::WriteOnly);
        s << v;
        QString res(a.toBase64());
        return res;
    }

    static QString map2String(const QVariantMap& map) {
        return var2String(QVariant(map));
    }

    static QList<SharedAnnotationData> var2ftl(const QVariantList& lst) {
        QList<SharedAnnotationData> atl;
        foreach(QVariant v, lst) {
            atl += qVariantValue<QList<SharedAnnotationData> >(v);
        }
        return atl;
    }

    static QVariant addStr2List(const QVariant& v, const QString& s) {
        if (v.canConvert(QVariant::StringList)) {
            QStringList l = v.toStringList();
            return (l << s);
        } else {
            assert(v.type() == QVariant::Invalid);
        }
        return QVariant(s);
    }

    static QVariant addStr2List(const QVariant& v, const QStringList& sl) {
        if (v.canConvert(QVariant::StringList)) {
            QStringList l = v.toStringList();
            return QVariant(l += sl);
        } else {
            assert(v.type() == QVariant::Invalid);
        }
        return QVariant(sl);
    }
};

} //namespace
#endif
