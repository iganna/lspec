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

#include "AppResources.h"

#include <U2Core/AppContext.h>
#include <U2Core/Settings.h>
#include <U2Core/AppSettings.h>
#include <U2Core/U2SafePoints.h>
#include <U2Test/GTest.h>

#include <QtCore/QThread>
#include <QtCore/QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#include <Psapi.h>
#include <Winbase.h> //for IsProcessorFeaturePresent
#endif

namespace U2 {

#define SETTINGS_ROOT QString("app_resource/")

AppResourcePool::AppResourcePool() {
    Settings* s = AppContext::getSettings();
    idealThreadCount = s->getValue(SETTINGS_ROOT + "idealThreadCount", QThread::idealThreadCount()).toInt();

    int maxThreadCount = s->getValue(SETTINGS_ROOT + "maxThreadCount", 1000).toInt();
    threadResource = new AppResource(RESOURCE_THREAD, maxThreadCount, tr("Threads"));
    registerResource(threadResource);

    int totalPhysicalMemory = getTotalPhysicalMemory();
    int maxMem = s->getValue(SETTINGS_ROOT + "maxMem", totalPhysicalMemory).toInt(); 
#if defined(Q_OS_MAC64) || defined(Q_OS_WIN64) || defined(UGENE_X86_64) || defined( __amd64__ ) || defined( __AMD64__ ) || defined( __x86_64__ ) || defined( _M_X64 )
    maxMem = maxMem > x64MaxMemoryLimitMb ? x64MaxMemoryLimitMb : maxMem;
#else
    maxMem = maxMem > x32MaxMemoryLimitMb ? x32MaxMemoryLimitMb : maxMem;
#endif

    memResource = new AppResource(RESOURCE_MEMORY, maxMem, tr("Memory"), tr("Mb"));
    registerResource(memResource);

    projectResouce = new AppResource(RESOURCE_PROJECT, 1, tr("Project"));
    registerResource(projectResouce);

    listenLogInGTest = new AppResource(RESOURCE_LISTEN_LOG_IN_TESTS, 1, "LogInTests");
    registerResource(listenLogInGTest);
}

AppResourcePool::~AppResourcePool() {
    qDeleteAll(resources.values());
}

int AppResourcePool::getTotalPhysicalMemory() {
    int totalPhysicalMemory = defaultMemoryLimitMb;

#if defined(Q_OS_WIN32)
    MEMORYSTATUSEX memory_status;
    ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
    memory_status.dwLength = sizeof(memory_status);
    if (GlobalMemoryStatusEx(&memory_status)) {
        totalPhysicalMemory = memory_status.ullTotalPhys / (1024 * 1024);
    } else {
        coreLog.error("Total physical memory: getting info error");
    }

#elif defined(Q_OS_LINUX)
    QProcess p;
    p.start("awk", QStringList() << "/MemTotal/ {print $2}" << "/proc/meminfo");
    p.waitForFinished();
    QString memory = p.readAllStandardOutput();
    p.close();
    totalPhysicalMemory = memory.toLong() / 1024;

#elif defined(Q_OS_MAC)
// TODO
//     QProcess p;
//     p.start("sysctl", QStringList() << "kern.version" << "hw.physmem");
//     p.waitForFinished();
//     QString system_info = p.readAllStandardOutput();
//     p.close();
#else
    coreLog.error("Total physical memory: Unsupported OS");
#endif

    return totalPhysicalMemory;
}

void AppResourcePool::setIdealThreadCount(int n) {
    SAFE_POINT(n > 0 && n <= threadResource->maxUse, QString("Invalid ideal threads count: %1").arg(n),);

    n = qBound(1, n, threadResource->maxUse);
    idealThreadCount = n;
    AppContext::getSettings()->setValue(SETTINGS_ROOT + "idealThreadCount", idealThreadCount);
}

void AppResourcePool::setMaxThreadCount(int n) {
    SAFE_POINT(n >= 1, QString("Invalid max threads count: %1").arg(n),);
    
    threadResource->maxUse = qMax(idealThreadCount, n);
    AppContext::getSettings()->setValue(SETTINGS_ROOT + "maxThreadCount", threadResource->maxUse );
}

void AppResourcePool::setMaxMemorySizeInMB(int n) {
    SAFE_POINT(n >= MIN_MEMORY_SIZE, QString("Invalid max memory size: %1").arg(n),);

    memResource->maxUse = qMax(n, MIN_MEMORY_SIZE);
    AppContext::getSettings()->setValue(SETTINGS_ROOT + "maxMem", memResource->maxUse);
}

bool AppResourcePool::getCurrentAppMemory(int& mb) {
#ifdef Q_OS_WIN
    HANDLE procHandle = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
    BOOL ok = GetProcessMemoryInfo(procHandle, &pmc, sizeof(procHandle));
    if (!ok) {
        return false;
    }
    mb = (int)(pmc.WorkingSetSize/(1024*1024));
    return true;    
#endif
    mb = MIN_MEMORY_SIZE;
    return false;
}

bool AppResourcePool::isSSE2Enabled() {
    bool answer = false;
#if defined( Q_OS_WIN )
    //Using WinAPI call on Windows.

    //Return Value
    //If the feature is supported, the return value is a nonzero value.
    //If the feature is not supported, the return value is zero.
    // 
    //If the HAL does not support detection of the feature, 
    //whether or not the hardware supports the feature, the return value is also zero. 
    //
    //Windows 2000: This feature is not supported.
    //
    //Header:  Winbase.h (include Windows.h)
    //Library: Kernel32.lib
    //DLL:     Kernel32.dll

    answer = (bool)IsProcessorFeaturePresent( PF_XMMI64_INSTRUCTIONS_AVAILABLE );
#elif defined( __amd64__ ) || defined( __AMD64__ ) || defined( __x86_64__ ) || defined( _M_X64 )
    answer = true;
#elif defined( __i386__ ) || defined( __X86__ ) || defined( _M_IX86 )
    //cpuid instruction: 
    //- takes 0x1 on eax,
    //- returns standard features flags in edx, bit 26 is SSE2 flag
    //- clobbers ebx, ecx
    unsigned int fflags = 0;
    unsigned int stub = 0;
    __asm__ __volatile__ (
        "push %%ebx; cpuid; mov %%ebx, %%edi; pop %%ebx" :
        "=a" (stub),
        "=D" (stub),
        "=c" (stub),
        "=d" (fflags) : "a" (0x1));
    answer = ((fflags & (1<<26))!=0);
#endif 
    return answer;
}

void AppResourcePool::registerResource(AppResource* r) {
    SAFE_POINT(!resources.contains(r->resourceId), QString("Duplicate resource: ").arg(r->resourceId),);

    resources[r->resourceId] = r;
}

AppResource* AppResourcePool::getResource(int id) const {
    return resources.value(id, NULL);
}


AppResourcePool* AppResourcePool::instance() {
    return AppContext::getAppSettings()->getAppResourcePool();
}

}//namespace
