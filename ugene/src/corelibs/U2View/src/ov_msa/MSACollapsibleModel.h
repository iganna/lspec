#ifndef _U2_MSA_COLLAPSIBLE_MODEL_H_
#define _U2_MSA_COLLAPSIBLE_MODEL_H_

#include <QtCore/QObject>
#include <QtCore/QVector>


namespace U2 {

#define MODIFIER "modifier"
#define MAROW_SIMILARITY_SORT "marow_similarity_sort"

class MSACollapsableItem {
public:
    MSACollapsableItem() : row(-1), numRows(-1), isCollapsed(false) {}
    MSACollapsableItem(int startPos, int length) : row(startPos), numRows(length), isCollapsed(false) {}

    int row;
    int numRows;
    bool isCollapsed;
};

class U2Region;
class MSAEditorUI;
class MAlignment;
class MAlignmentModInfo;

class MSACollapsibleItemModel : public QObject {
    Q_OBJECT
public:
    MSACollapsibleItemModel(MSAEditorUI* p);

    // creates model with every item collapsed
    // 'itemRegions' has to be sorted list of non-intersecting regions
    void reset(const QVector<U2Region>& itemRegions);

    void reset();
    
    void toggle(int pos);

    void collapseAll(bool collapse);

    int mapToRow(int pos) const;

    U2Region mapToRows(int pos) const;

    void getVisibleRows(int startPos, int endPos, QVector<U2Region>& rows) const;

    bool isTopLevel(int pos) const;

    int getLastPos() const;

    int itemAt(int pos) const;

    int getItemPos(int index) const { return positions.at(index); }

    MSACollapsableItem getItem(int index) const { return items.at(index); }

signals:
    void toggled();

private slots:
    void sl_alignmentChanged(const MAlignment& maBefore, const MAlignmentModInfo& modInfo);

private:
    void triggerItem(int index);
    int mapToRow(int lastItem, int pos) const;
    void tracePositions();

private:
    MSAEditorUI* ui;
    QVector<MSACollapsableItem> items;
    QVector<int> positions;
};

} //namespace

#endif
