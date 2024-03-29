include(external_tool_support.pri)

HEADERS += src/ETSProjectViewItemsContoller.h \
           src/ExternalToolRunTask.h \
           src/ExternalToolSupportL10N.h \
           src/ExternalToolSupportPlugin.h \
           src/ExternalToolSupportSettings.h \
           src/ExternalToolSupportSettingsController.h \
           src/RnaSeqCommon.h \
           src/TaskLocalStorage.h \
           src/blast/BlastAllSupport.h \
           src/blast/BlastAllSupportRunDialog.h \
           src/blast/BlastAllSupportTask.h \
           src/blast/BlastAllWorker.h \
           src/blast/FormatDBSupport.h \
           src/blast/FormatDBSupportRunDialog.h \
           src/blast/FormatDBSupportTask.h \
           src/blast_plus/BlastNPlusSupportTask.h \
           src/blast_plus/BlastPlusSupport.h \
           src/blast_plus/BlastPlusSupportCommonTask.h \
           src/blast_plus/BlastPlusSupportRunDialog.h \
           src/blast_plus/BlastPlusWorker.h \
           src/blast_plus/BlastPPlusSupportTask.h \
           src/blast_plus/BlastXPlusSupportTask.h \
           src/blast_plus/RPSBlastSupportTask.h \
           src/blast_plus/TBlastNPlusSupportTask.h \
           src/blast_plus/TBlastXPlusSupportTask.h \
           src/bowtie/BowtieSettingsWidget.h \
           src/bowtie/BowtieSupport.h \
           src/bowtie/BowtieTask.h \
           src/bowtie2/Bowtie2Support.h \
           src/bwa/BwaSettingsWidget.h \
           src/bwa/BwaSupport.h \
           src/bwa/BwaTask.h \
           src/cap3/CAP3Support.h \
           src/cap3/CAP3SupportDialog.h \
           src/cap3/CAP3SupportTask.h \
           src/cap3/CAP3Worker.h \
           src/ceas/CEASReportWorker.h \
           src/ceas/CEASSettings.h \
           src/ceas/CEASSupport.h \
           src/ceas/CEASSupportTask.h \
           src/clustalw/ClustalWSupport.h \
           src/clustalw/ClustalWSupportRunDialog.h \
           src/clustalw/ClustalWSupportTask.h \
           src/clustalw/ClustalWWorker.h \
           src/clustalo/ClustalOSupport.h \
           src/clustalo/ClustalOSupportRunDialog.h \
           src/clustalo/ClustalOSupportTask.h \
           src/clustalo/ClustalOWorker.h \
           src/cufflinks/CuffdiffWorker.h \
           src/cufflinks/CufflinksSettings.h \
           src/cufflinks/CufflinksSupport.h \
           src/cufflinks/CufflinksSupportTask.h \
           src/cufflinks/CufflinksWorker.h \
           src/cufflinks/CuffmergeWorker.h \
           src/mafft/MAFFTSupport.h \
           src/mafft/MAFFTSupportRunDialog.h \
           src/mafft/MAFFTSupportTask.h \
           src/mafft/MAFFTWorker.h \
           src/mrbayes/MrBayesDialogWidget.h \
           src/mrbayes/MrBayesSupport.h \
           src/mrbayes/MrBayesTask.h \
           src/mrbayes/MrBayesTests.h \
           src/samtools/SamToolsExtToolSupport.h \
           src/spidey/SpideySupport.h \
           src/spidey/SpideySupportTask.h \
           src/tcoffee/TCoffeeSupport.h \
           src/tcoffee/TCoffeeSupportRunDialog.h \
           src/tcoffee/TCoffeeSupportTask.h \
           src/tcoffee/TCoffeeWorker.h \
           src/tophat/TopHatSupport.h \
           src/tophat/TopHatWorker.h \
           src/utils/BlastRunCommonDialog.h \
           src/utils/BlastTaskSettings.h \
           src/utils/ExportTasks.h \
           src/utils/ExternalToolSupportAction.h \
           src/utils/ExternalToolValidateTask.h \
           src/bowtie/bowtie_tests/bowtieTests.h \
           src/bwa/bwa_tests/bwaTests.h \
           src/cufflinks/CufflinksTests.h \
           src/tophat/TopHatSupportTask.h \
           src/tophat/TopHatSettings.h
FORMS += src/ui/BlastAllSupportDialog.ui \
         src/ui/BowtieBuildSettings.ui \
         src/ui/BowtieSettings.ui \
         src/ui/BwaBuildSettings.ui \
         src/ui/BwaSettings.ui \
         src/ui/CAP3SupportDialog.ui \
         src/ui/ClustalWSupportRunDialog.ui \
         src/ui/ClustalOSupportRunDialog.ui \
         src/ui/ETSSettingsWidget.ui \
         src/ui/FormatDBSupportRunDialog.ui \
         src/ui/MAFFTSupportRunDialog.ui \
         src/ui/MrBayesDialog.ui \
         src/ui/SelectPathDialog.ui \
         src/ui/TCoffeeSupportRunDialog.ui
SOURCES += src/ETSProjectViewItemsContoller.cpp \
           src/ExternalToolRunTask.cpp \
           src/ExternalToolSupportPlugin.cpp \
           src/ExternalToolSupportSettings.cpp \
           src/ExternalToolSupportSettingsController.cpp \
           src/RnaSeqCommon.cpp \
           src/TaskLocalStorage.cpp \
           src/blast/BlastAllSupport.cpp \
           src/blast/BlastAllSupportRunDialog.cpp \
           src/blast/BlastAllSupportTask.cpp \
           src/blast/BlastAllWorker.cpp \
           src/blast/FormatDBSupport.cpp \
           src/blast/FormatDBSupportRunDialog.cpp \
           src/blast/FormatDBSupportTask.cpp \
           src/blast_plus/BlastNPlusSupportTask.cpp \
           src/blast_plus/BlastPlusSupport.cpp \
           src/blast_plus/BlastPlusSupportCommonTask.cpp \
           src/blast_plus/BlastPlusSupportRunDialog.cpp \
           src/blast_plus/BlastPlusWorker.cpp \
           src/blast_plus/BlastPPlusSupportTask.cpp \
           src/blast_plus/BlastXPlusSupportTask.cpp \
           src/blast_plus/RPSBlastSupportTask.cpp \
           src/blast_plus/TBlastNPlusSupportTask.cpp \
           src/blast_plus/TBlastXPlusSupportTask.cpp \
           src/bowtie/BowtieSettingsWidget.cpp \
           src/bowtie/BowtieSupport.cpp \
           src/bowtie/BowtieTask.cpp \
           src/bowtie2/Bowtie2Support.cpp \
           src/bwa/BwaSettingsWidget.cpp \
           src/bwa/BwaSupport.cpp \
           src/bwa/BwaTask.cpp \
           src/cap3/CAP3Support.cpp \
           src/cap3/CAP3SupportDialog.cpp \
           src/cap3/CAP3SupportTask.cpp \
           src/cap3/CAP3Worker.cpp \
           src/ceas/CEASReportWorker.cpp \
           src/ceas/CEASSettings.cpp \
           src/ceas/CEASSupport.cpp \
           src/ceas/CEASSupportTask.cpp \
           src/clustalw/ClustalWSupport.cpp \
           src/clustalw/ClustalWSupportRunDialog.cpp \
           src/clustalw/ClustalWSupportTask.cpp \
           src/clustalw/ClustalWWorker.cpp \
           src/clustalo/ClustalOSupport.cpp \
           src/clustalo/ClustalOSupportRunDialog.cpp \
           src/clustalo/ClustalOSupportTask.cpp \
           src/clustalo/ClustalOWorker.cpp \
           src/cufflinks/CuffdiffWorker.cpp \
           src/cufflinks/CufflinksSettings.cpp \
           src/cufflinks/CufflinksSupport.cpp \
           src/cufflinks/CufflinksSupportTask.cpp \
           src/cufflinks/CufflinksWorker.cpp \
           src/cufflinks/CuffmergeWorker.cpp \
           src/mafft/MAFFTSupport.cpp \
           src/mafft/MAFFTSupportRunDialog.cpp \
           src/mafft/MAFFTSupportTask.cpp \
           src/mafft/MAFFTWorker.cpp \
           src/mrbayes/MrBayesDialogWidget.cpp \
           src/mrbayes/MrBayesSupport.cpp \
           src/mrbayes/MrBayesTask.cpp \
           src/mrbayes/MrBayesTests.cpp \
           src/samtools/SamToolsExtToolSupport.cpp \
           src/spidey/SpideySupport.cpp \
           src/spidey/SpideySupportTask.cpp \
           src/tcoffee/TCoffeeSupport.cpp \
           src/tcoffee/TCoffeeSupportRunDialog.cpp \
           src/tcoffee/TCoffeeSupportTask.cpp \
           src/tcoffee/TCoffeeWorker.cpp \
           src/tophat/TopHatSupport.cpp \
           src/tophat/TopHatWorker.cpp \
           src/utils/BlastRunCommonDialog.cpp \
           src/utils/BlastTaskSettings.cpp \
           src/utils/ExportTasks.cpp \
           src/utils/ExternalToolSupportAction.cpp \
           src/utils/ExternalToolValidateTask.cpp \
           src/bowtie/bowtie_tests/bowtieTests.cpp \
           src/bwa/bwa_tests/bwaTests.cpp \
           src/cufflinks/CufflinksTests.cpp \
           src/tophat/TopHatSupportTask.cpp \
           src/tophat/TopHatSettings.cpp
RESOURCES += external_tool_support.qrc
TRANSLATIONS += transl/chinese.ts \
                transl/czech.ts \
                transl/english.ts \
                transl/russian.ts
