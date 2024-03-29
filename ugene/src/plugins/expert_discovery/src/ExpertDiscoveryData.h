#ifndef _EXPERTDISCOVERIDATAHEADER_
#define _EXPERTDISCOVERIDATAHEADER_

#include "DDisc/Sequence.h"
#include "DDisc/MetaInfo.h"

#include "ExpertDiscoveryCSUtil.h"
#include "ExpertDiscoveryTreeWidgets.h"

#include <U2Core/GObject.h>
#include <QObject>

namespace U2 {

using namespace DDisc;

class EDProjectItem;
class EDPICS;
class EDPISequence;
class MAlignmentRow;
class DNATranslation;

const int ED_UPDATE_ALL             = 0;
const int ED_CURRENT_ITEM_CHANGED   = 1;
const int ED_ITEM_NAME_CHANGED      = 2;
const int ED_ITEM_STATE_CHANGED     = 3;
const int ED_ITEM_ADDED             = 4;
const int ED_ITEM_DELETED           = 5;
const int ED_UPDATE_CHILDREN        = 6;
const int ED_PROPERTY_CHANGED       = 7;
const int ED_MRK_UPDATE             = 8;

enum SequenceType {POSITIVE_SEQUENCE, NEGATIVE_SEQUENCE, CONTROL_SEQUENCE, UNKNOWN_SEQUENCE};

class ExpertDiscoveryData : public QObject{
    Q_OBJECT
public:
    static const std::string FAMILY_LETTERS;
    static const std::string FAMILY_LETTERS_METHOD;
    static const QString FAMILY_ED_SIGNAL;
    static const QString FAMILY_ED_METHOD;

    ExpertDiscoveryData ();

    void setPosBase(const QList<GObject*> &);
    void setNegBase(const QList<GObject*> &);
    void setConBase(const QList<GObject*> &);

    void markupLetters(void);
    void markupLetters(SequenceBase& rBase, MarkingBase& rAnn);
    void markupLetters(Sequence& seq);
    bool isLettersMarkedUp(void) const;
    void clearScores();

    const MetaInfoBase& getDescriptionBase() const { return desc;}
    MetaInfoBase& getDescriptionBaseNoConst() { return desc;}
    SequenceBase& getPosSeqBase() {return posBase;}
    SequenceBase& getNegSeqBase() {return negBase;}
    SequenceBase& getConSeqBase() {return conBase;}

    MarkingBase& getPosMarkBase() {return posAnn;}
    MarkingBase& getNegMarkBase() {return negAnn;}
    MarkingBase& getConMarkBase() {return conAnn;}

    void clearContrBase();
    void clearContrAnnot();

    CSFolder& getRootFolder(){return rootFolder;}

    bool updateScore(Sequence& rSeq);
    double getRecognizationBound() const { return recognizationBound; }
    void setRecBound(double val){recognizationBound = val;}
    void optimizeRecognizationBound();

    void setRecBound();
    bool updateScores();

    bool isSignalSelected(const EDProjectItem* pItem);
    void switchSelection(EDProjectItem* pItem, bool upd);
    SelectedSignalsContainer& getSelectedSignalsContainer() { return selectedSignals; }

    void onSetCurrentSignalParamsAsPrior(EDPICS *pItem, bool bUpdate);
    void onClearSignalPriorParams(EDPICS *pItem);

    SequenceType getSequenceTypeByName(const QString& seqName);
    int getSequenceIndex(const QString& seqName, SequenceType type);
    RecognizationData getRecognitionData(int seqIndex, SequenceType type);

    bool loadMarkup(const QString& firstF, const QString& secondF, const QString& thirdF, bool generateDescr);
    bool loadAnnotation(MarkingBase& base, const SequenceBase& seqBase, QString strFileName);
    bool generateDescription(bool clearDescr = true);
    void loadControlSequenceAnnotation(const QString& fileName);

    void cleanup();

    void addSequenceToSelected(EDPISequence* seq);
    void clearSelectedSequencesList();
    bool isSequenceSelected(EDPISequence* seq);
    QList<EDPISequence*> getSelectetSequencesList();

    void generateRecognitionReportFull();
    bool generateRecognizationReportHeader(QString& resultText) const;
    bool generateRecognizationReportFooter(QString& resultText) const;
    bool generateRecognizationReportSignals(QString& resultText) const;
    bool generateRecognizationReport(const SequenceBase& rBase, QString strName, bool bSuppressNulls, QString& resultText);
    bool generateRecognizationReportPositive(QString strName, bool bSuppressNulls, QString& resultText);
    int  getSequencesCountWithScoreMoreThan(double dScore, const SequenceBase& rBase) const;
    void generateRecognizationReport(EDProjectItem* pItem);

    void setBaseFilename(const SequenceBase& base, const QString& fileName);

    bool isModified() {return modified;}
    void setModifed(bool modFlag = true){modified = modFlag;}

    int getMaxPosSequenceLen();

    static float calculateSequenceScore(const char* seq, int seqLen, ExpertDiscoveryData& edData, DNATranslation* complTT);
    
    double recognizationBound;
    RecognizationDataStorage recDataStorage;
    QList<EDPISequence*> selSequences;
    
private:
    SequenceBase                posBase;
    SequenceBase                negBase;
    SequenceBase                conBase;
    MetaInfoBase                desc;
    MarkingBase                 posAnn;
    MarkingBase                 negAnn;
    MarkingBase                 conAnn;
    CSFolder                    rootFolder;
    bool                        modified;
    SelectedSignalsContainer    selectedSignals;

    std::map<const SequenceBase*, std::string> fileNamesMap;

    inline Sequence prepareSequence(const GObject* obj) const;
    inline Sequence prepareSequence(MAlignmentRow& row) const;
    static std::string char2string(char ch);
    void setBase(const QList<GObject*> &objects, SequenceBase& base);
};
}//namespace

#endif
