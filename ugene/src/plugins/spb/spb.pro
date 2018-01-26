include (spb.pri)

# Input
HEADERS += src/ComparingAlgorithm.h \
           src/DereplicateSequencesWorker.h \
           src/DereplicationTask.h \
           src/DistanceReportWorker.h \
           src/FilterSequencesWorker.h \
           src/FullIndexComparer.h \
           src/GenerateUrlWorker.h \
           src/IntersectionsWorker.h \
           src/RandomDereplicationTask.h \
           src/RandomFilterWorker.h \
           src/SpbPlugin.h
SOURCES += src/ComparingAlgorithm.cpp \
           src/DereplicateSequencesWorker.cpp \
           src/DereplicationTask.cpp \
           src/DistanceReportWorker.cpp \
           src/FilterSequencesWorker.cpp \
           src/FullIndexComparer.cpp \
           src/GenerateUrlWorker.cpp \
           src/IntersectionsWorker.cpp \
           src/RandomDereplicationTask.cpp \
           src/RandomFilterWorker.cpp \
           src/SpbPlugin.cpp
