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

#include <U2Core/AppContext.h>
#include <U2Core/Log.h>
#include <U2Core/U2DbiRegistry.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/AppSettings.h>
#include <U2Core/UserApplicationsSettings.h>
#include <U2Core/GUrlUtils.h>
#include <U2Core/U2SafePoints.h>
#include <U2Core/U2DbiUtils.h>

#include <QtCore/QFile>
#include <QtCore/QCoreApplication>

namespace U2 {

    U2DbiRegistry::U2DbiRegistry(QObject *parent) : QObject(parent), lock(QMutex::Recursive) {
    pool = new U2DbiPool(this);
    sessionDbiConnection = NULL;
    sessionDbiInitDone = false;
}

void U2DbiRegistry::initSessionDbi(TmpDbiRef& tmpDbiRef) {
    sessionDbiInitDone = true;

    U2OpStatus2Log os;
    sessionDbiConnection = new DbiConnection(tmpDbiRef.dbiRef, os);

    tmpDbiRef.nUsers++;
}

U2DbiRegistry::~U2DbiRegistry() {
    if (sessionDbiConnection != NULL) {
        U2DbiRef ref = sessionDbiConnection->dbi->getDbiRef();
        delete sessionDbiConnection;
        U2OpStatus2Log os;
        detachTmpDbi(SESSION_TMP_DBI_ALIAS, os);
    }

    coreLog.trace("Deallocating U2DbiRegistry");
    for (int i = 0; i < tmpDbis.size(); i++) {
        TmpDbiRef& ref = tmpDbis[i];
        coreLog.trace(QString("BUG: tmp DBI was not deallocated: %1 %2 [%3]").arg(ref.alias).arg(ref.dbiRef.dbiId).arg(ref.nUsers));
        ref.nUsers = 1;
    }
    foreach(const TmpDbiRef& ref, tmpDbis) {
        U2OpStatus2Log os;
        detachTmpDbi(ref.alias, os);
    }
    qDeleteAll(factories.values());
}

QList<U2DbiRef> U2DbiRegistry::listTmpDbis() const {
    QList<U2DbiRef> res;
    foreach(const TmpDbiRef& ref, tmpDbis) {
        res << ref.dbiRef;
    }
    return res;
}

U2DbiRef U2DbiRegistry::attachTmpDbi(const QString& alias, U2OpStatus& os) {
    QMutexLocker m(&lock);

    for (int i = 0; i < tmpDbis.size(); i++) {
        TmpDbiRef& ref = tmpDbis[i];
        if (ref.alias == alias) {
            ref.nUsers++;
            return ref.dbiRef;
        }
    }

    coreLog.trace("Allocating a tmp dbi with alias: " + alias);
    U2DbiRef dbiRef = allocateTmpDbi(alias, os);
    CHECK_OP(os, U2DbiRef());

    coreLog.trace("Allocated tmp dbi: " + dbiRef.dbiId);
    TmpDbiRef tmpDbiRef= TmpDbiRef(alias, dbiRef, 1);

    if (alias == SESSION_TMP_DBI_ALIAS && !sessionDbiInitDone) { //once used -> cache connection
        initSessionDbi(tmpDbiRef);
    }

    tmpDbis << tmpDbiRef;

    return dbiRef;
}

void U2DbiRegistry::detachTmpDbi(const QString& alias, U2OpStatus& os) {
    QMutexLocker l(&lock);

    int i = 0;
    bool found = false;
    for (;i < tmpDbis.size(); i++) {
        TmpDbiRef& ref = tmpDbis[i];
        if (ref.alias == alias) {
            found = true;
            ref.nUsers--;
            if (ref.nUsers > 0) {
                return;
            }
            break;
        }
    }
    if (!found) {
        coreLog.error(tr("The tmp dbi is not found: %1").arg(alias));
        return;
    }

    const TmpDbiRef& tmpDbiRef = tmpDbis.at(i);

    coreLog.trace("Deallocating a tmp dbi " + tmpDbiRef.dbiRef.dbiId + " with alias: " + tmpDbiRef.alias);
    deallocateTmpDbi(tmpDbiRef, os);

    tmpDbis.removeAt(i);
}

U2DbiRef U2DbiRegistry::getSessionTmpDbiRef(U2OpStatus &os) {
    TmpDbiHandle dh(SESSION_TMP_DBI_ALIAS, os);
    CHECK_OP(os, U2DbiRef());

    return dh.getDbiRef();
}

void U2DbiRegistry::deallocateTmpDbi(const TmpDbiRef& ref, U2OpStatus& os) {
    QMutexLocker l(&lock);

    pool->closeAllConnections(ref.dbiRef.dbiId, os);
    if (QFile::exists(ref.dbiRef.dbiId)) {
        QFile::remove(ref.dbiRef.dbiId);
    }
}

U2DbiRef U2DbiRegistry::allocateTmpDbi(const QString& alias, U2OpStatus& os) {
    QMutexLocker m(&lock);

    U2DbiRef res;
    QString tmpDirPath = AppContext::getAppSettings()->getUserAppsSettings()->getCurrentProcessTemporaryDirPath();
    QString url = GUrlUtils::prepareTmpFileLocation(tmpDirPath, alias, "ugenedb", os);
    CHECK_OP(os, res);

    res.dbiId = url;
    res.dbiFactoryId = DEFAULT_DBI_ID;

    {
        // Create a tmp dbi file (the DbiConnection is opened with "bool create = true", and released)
        DbiConnection con(res, true, os);
        CHECK_OP(os, U2DbiRef());
    }

    return res;
}

bool U2DbiRegistry::registerDbiFactory(U2DbiFactory *factory) {
    if (factories.contains(factory->getId())) {
        return false;
    }
    factories.insert(factory->getId(), factory);
    return true;
}


QList<U2DbiFactoryId> U2DbiRegistry::getRegisteredDbiFactories() const {
    return factories.keys();
}

U2DbiFactory *U2DbiRegistry::getDbiFactoryById(U2DbiFactoryId id) const {
    return factories.value(id, NULL);
}


//////////////////////////////////////////////////////////////////////////
// U2DbiPool

U2DbiPool::U2DbiPool(QObject* p) : QObject(p) {
}

U2Dbi* U2DbiPool::openDbi(const U2DbiRef& ref, bool create, U2OpStatus& os) {
    QMutexLocker m(&lock);

    U2Dbi* dbi = NULL;
    QString url = ref.dbiId;

    ioLog.trace(QString("DbiPool: Opening DBI. Url: %1, factory: %2").arg(url).arg(ref.dbiFactoryId));

    if (url.isEmpty()) {
        os.setError(tr("No URL provided!"));
        return NULL;
    }
    if (dbiByUrl.contains(url)) {
        dbi = dbiByUrl[url];
        int cnt = dbiCountersByUrl[url];
        dbiCountersByUrl[url] = cnt + 1;
    } else {
        U2DbiFactory* f = AppContext::getDbiRegistry()->getDbiFactoryById(ref.dbiFactoryId);
        if (f == NULL) {
            os.setError(tr("Invalid database type: %1").arg(ref.dbiFactoryId));
            return NULL;
        }
        dbi = f->createDbi();
        QHash<QString, QString> initProperties;
        initProperties[U2_DBI_OPTION_URL] = url;
        if (create) {
            initProperties[U2_DBI_OPTION_CREATE] = U2_DBI_VALUE_ON;
        }
        dbi->init(initProperties, QVariantMap(), os);
        if (os.hasError()) {
            delete dbi;
            return NULL;
        }
        dbiByUrl[url] = dbi;
        dbiCountersByUrl[url] = 1;
    }
    return dbi;
}

void U2DbiPool::addRef(U2Dbi * dbi, U2OpStatus & os) {
    QMutexLocker m(&lock);

    QString url = dbi->getInitProperties().value(U2_DBI_OPTION_URL);
    if (!dbiByUrl.contains(url)) {    
        os.setError(tr("DbiPool: DBI not found! URL: %1").arg(url));
        return;
    }
    assert(dbiCountersByUrl[url] > 0);
    int cnt = ++dbiCountersByUrl[url];
    ioLog.trace(QString("DbiPool: Increasing reference count. Url: %1, ref-count: %2").arg(url).arg(cnt));
}

void U2DbiPool::releaseDbi(U2Dbi* dbi, U2OpStatus& os) {
    QMutexLocker m(&lock);

    QString url = dbi->getInitProperties().value(U2_DBI_OPTION_URL);
    if (!dbiByUrl.contains(url)) {    
        os.setError(tr("DbiPool: DBI not found! URL: %1").arg(url));
        return;
    }
    int cnt = --dbiCountersByUrl[url];
    ioLog.trace(QString("DbiPool: decreasing reference count. Url: %1, ref-count: %2").arg(url).arg(cnt));
    if (cnt > 0) {
        return;
    }
    dbi->shutdown(os);
    delete dbi;

    dbiByUrl.remove(url);
    dbiCountersByUrl.remove(url);
    ioLog.trace(QString("DBIPool: resource is released. Url: %1").arg(url));
}


void U2DbiPool::closeAllConnections(const QString& url, U2OpStatus& os) {
    QMutexLocker m(&lock);

    if (!dbiByUrl.contains(url)) {
        return;
    }
    U2Dbi* dbi = dbiByUrl[url];
    dbi->shutdown(os);
    delete dbi;

    dbiByUrl.remove(url);
    int nActive  = dbiCountersByUrl.value(url, 0);
    dbiCountersByUrl.remove(url);
    ioLog.trace(QString("DBIPool: closing all connections. Url: %1, active references: %2 ").arg(url).arg(nActive));
}

} // namespace U2
