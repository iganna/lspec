#ifndef _U2_GUI_TEST_H_
#define _U2_GUI_TEST_H_

#include <U2Core/global.h>
#include <QtCore/QTimer>
#include <U2Core/Task.h>

namespace U2 {

class GUITestIgnorable {
public:
    // not ignored test, ignored by all, ignored on windows platforms, ignored on linux platforms
    enum IgnoreStatus {NotIgnored, Ignored, IgnoredWindows, IgnoredLinux};

    GUITestIgnorable() : ignoreStatus(NotIgnored), ignoreMessage("") {}

    void setIgnored(IgnoreStatus status, const QString& message = "") { ignoreStatus = status; ignoreMessage = message; }
    IgnoreStatus getIgnoreStatus() const {return ignoreStatus; }
    QString getIgnoreMessage() const {return ignoreMessage; }

    bool isIgnored() const {
        bool ignored = ignoreStatus == Ignored;
        bool platformIgnore = false;

#ifdef _WIN32
        platformIgnore = (ignoreStatus == IgnoredWindows);
#endif

#ifdef __linux__
        platformIgnore = (ignoreStatus == IgnoredLinux);
#endif

        return ignored || platformIgnore;
    }

private:
    IgnoreStatus ignoreStatus;
    QString ignoreMessage;
};

class U2TEST_EXPORT GUITest: public QObject, public GUITestIgnorable {
    Q_OBJECT
public:
    GUITest(const QString &_name = "") : name(_name) {}

    QString getName() const { return name; }
    void setName(const QString &n) { name = n; }

    static const QString testDir;
    static const QString dataDir;

    virtual void run(U2OpStatus &os) = 0;
private:
    QString name;
};

typedef QList<GUITest*> GUITests;

} //U2

#endif
