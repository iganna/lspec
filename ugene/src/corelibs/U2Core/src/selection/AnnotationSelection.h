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

#ifndef _U2_ANNOTATION_SELECTION_H_
#define _U2_ANNOTATION_SELECTION_H_

#include "SelectionTypes.h"
#include <U2Core/U2Region.h>

namespace U2 {

class Annotation;
class AnnotationGroup;
class AnnotationTableObject;
class DNATranslation;
class U2EntityRef;
class U2OpStatus;

class U2CORE_EXPORT AnnotationSelectionData {
public:
    AnnotationSelectionData(Annotation* a, int lIdx) : annotation(a), locationIdx(lIdx){}
    // the annotation selected
    Annotation* annotation;

    //location idx that is preferable for visualization
    // -1 == not specified, 0..N == idx of U2Region in 'location' field
    //BUG:404: do we need a separate class for this feature?
    int         locationIdx;

    bool operator==(const AnnotationSelectionData& d) const {return d.annotation == annotation && d.locationIdx == locationIdx;}

    int getSelectedRegionsLen() const;
    
    // Returns all regions from selected annotations locations without any modification
    QVector<U2Region> getSelectedRegions() const;
};

class  U2CORE_EXPORT AnnotationSelection : public GSelection {
    Q_OBJECT
public:
    AnnotationSelection(QObject* p = NULL) : GSelection(GSelectionTypes::ANNOTATIONS, p) {}

    const QList<AnnotationSelectionData>& getSelection() const {return selection;}

    const AnnotationSelectionData* getAnnotationData(const Annotation*) const;

    // adds annotation to selection.
    // if annotation is already in selection and have a different locationIdx -> removes the old annotaiton selection data
    // and adds the annotation again with updated locationIdx
    void addToSelection(Annotation* a, int locationIdx = -1);

    void removeFromSelection(Annotation* a, int locationIdx = -1);

    virtual bool isEmpty() const {return selection.isEmpty();}

    virtual void clear();

    virtual void removeObjectAnnotations(AnnotationTableObject* obj);

    bool contains(const Annotation* a) const {return getAnnotationData(a)!=NULL;}
    
    bool contains(const Annotation* a, int locationIdx) const;

    static void getAnnotationSequence(QByteArray& res, const AnnotationSelectionData& ad, char gapSym, 
                                      const U2EntityRef& ref, DNATranslation* complTT, DNATranslation* aminoTT, U2OpStatus& os);

    // Returns list of locations of all selected annotations that belongs to the objects in list
    QVector<U2Region> getSelectedLocations(const QSet<AnnotationTableObject*>& objects) const;

signals:
    void si_selectionChanged(
        AnnotationSelection* thiz,
        const QList<Annotation*>& added,
        const QList<Annotation*>& removed);

private:
    QList<AnnotationSelectionData> selection;
};



//////////////////////////////////////////////////////////////////////////
// AnnotationGroupSelection

class  U2CORE_EXPORT AnnotationGroupSelection : public GSelection {
    Q_OBJECT
public:
    AnnotationGroupSelection(QObject* p = NULL) : GSelection(GSelectionTypes::ANNOTATION_GROUPS, p) {}

    const QList<AnnotationGroup*>& getSelection() const {return selection;}

    void addToSelection(AnnotationGroup* g);

    void removeFromSelection(AnnotationGroup* g);

    virtual bool isEmpty() const {return selection.isEmpty();}

    virtual void clear();

    virtual void removeObjectGroups(AnnotationTableObject* obj);

    bool contains(AnnotationGroup* g) const {return selection.contains(g);}

signals:
    void si_selectionChanged(
        AnnotationGroupSelection* thiz, 
        const QList<AnnotationGroup*>& added, 
        const QList<AnnotationGroup*>& removed);

private:
    QList<AnnotationGroup*> selection;
};


}//namespace

#endif

