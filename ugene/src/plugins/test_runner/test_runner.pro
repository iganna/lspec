include (test_runner.pri)

# Input
HEADERS += resource.h \
           src/GTestScriptWrapper.h \
           src/TestRunnerPlugin.h \
           src/TestViewController.h \
           src/TestViewReporter.h
FORMS += src/ui/Reporter.ui src/ui/TestView.ui
SOURCES += src/GTestScriptWrapper.cpp \
           src/TestRunnerPlugin.cpp \
           src/TestViewController.cpp \
           src/TestViewReporter.cpp
RESOURCES += test_runner.qrc
TRANSLATIONS += transl/english.ts transl/russian.ts
