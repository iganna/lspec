#ifndef _SPB_FULL_INDEX_COMPARER_H_
#define _SPB_FULL_INDEX_COMPARER_H_

#include "ComparingAlgorithm.h"

namespace SPB {

class SequenceMapping {
public:
    SequenceMapping();
    ~SequenceMapping();

    void initMapping(int seqCount);
    void clearMapping();
    void createMapping(quint64 start, int num);
    int getSeqByGlobalPos(quint64 pos);

private:
    quint64 *mappingArray;
    int *seqNumsForMapping;
    int mappingSize;
};

class BaseSequencesStorage;

class BaseFullIndexComparer {
public:
    BaseFullIndexComparer(double accuracy, BaseSequencesStorage &sequences, const QString &algoId = ComparingAlgorithmFactory::DEFAULT);
    virtual ~BaseFullIndexComparer();

    void initialize();
    bool isInitialized() const;

    void findSimilars(const QByteArray &seq);
    bool hasSimilars(const QByteArray &seq);
    QList<int> getSimilars(const QByteArray &seq, bool checkRemoved = false);

    struct Similar {
        QString seqName;
        double accuracy;
    };
    QList<Similar> removeSimilars(int seqNum);

    BaseSequencesStorage & getSequences();

private:
    void calculateWindowSize(quint64 minLength, quint64 maxLength);
    void createIndex();
    void clearAlignInfo();
    QList<Similar> removeFoundedSimilars();

    int getWindowsSizeByLength(quint64 len);
    int getMatchCountByLength(quint64 len);

protected:
    bool initialized;
    BaseSequencesStorage &sequences;
    ComparingAlgorithm *comparer;
    double accuracy;
    QByteArray fullSeq;
    int w;
    SArrayIndex *index;
    SequenceMapping mapping;
    char *founded;
    short *currentSym;
    QByteArray leaderSeq;
};

class SequencesStorage;

class FullIndexComparer : public BaseFullIndexComparer {
public:
    FullIndexComparer(double accuracy, SequencesStorage &sequences, const QString &algoId);

    SequencesStorage & getSequences();

private:
    SequencesStorage &seqStorage;
};

class BaseSequencesStorage {
public:
    BaseSequencesStorage();
    virtual ~BaseSequencesStorage();

    virtual int getInitialSeqCount() const = 0;

    bool isInitialized() const;
    int getSeqCount() const;
    bool isEmpty() const;
    const QMap<int, DNASequence> & getSeqMap() const;
    QByteArray & getSequence(int seqNum);
    QString getName(int seqNum) const;
    bool isRemoved(int seqNum) const;
    void removeSequence(int seqNum);
    int getRandomSeqNum() const;

protected:
    bool initialized;
    QMap<int, DNASequence> seqMap;
    QList<int> removed;
};

class SequencesStorage : public BaseSequencesStorage {
public:
    SequencesStorage(const QList<SharedDbiDataHandler> &seqIds);
    void initialize(DbiDataStorage *dbiStorage, U2OpStatus &os);

    int getInitialSeqCount() const;

private:
    QList<SharedDbiDataHandler> seqIds;
};

class FastaSequenceStorage : public BaseSequencesStorage {
public:
    FastaSequenceStorage(const QString &file);
    void initialize(U2OpStatus &os);

    int getInitialSeqCount() const;

private:


private:
    QString file;
    int initialSeqCount;
};

} // SPB

#endif // _SPB_FULL_INDEX_COMPARER_H_
