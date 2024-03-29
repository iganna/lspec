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

#include "AnnotationTableObject.h"
#include "GObjectTypes.h"

#include <U2Core/Timer.h>
#include <U2Core/DNATranslation.h>
#include <U2Core/TextUtils.h>
#include <U2Core/U2SafePoints.h>
#include <U2Core/DNASequenceObject.h>
#include <U2Core/AppContext.h>
#include <U2Core/U2FeatureUtils.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/U2DbiRegistry.h>
#include <U2Core/U2FeatureDbi.h>

#include <QtCore/QBitArray>
// for Qt::escape
#include <QtGui/QTextDocument>




namespace U2 {

Annotation::Annotation(SharedAnnotationData _d): obj(NULL), d(_d), caseAnnotation(false)
{
}

Annotation::~Annotation() {
    //todo: add state checks?
}

bool Annotation::isValidQualifierName(const QString& s) {
    return !s.isEmpty() && s.length() < 20 && TextUtils::fits(TextUtils::QUALIFIER_NAME_CHARS, s.toAscii().data(), s.length());
}

bool Annotation::isValidQualifierValue(const QString& s) {
    Q_UNUSED(s); //todo: check whitespaces!
    return true;
}

bool Annotation::isValidAnnotationName(const QString& n) {
    //todo: optimize
    if (n.isEmpty() || n.length() > 100) {
        return false;
    }

    QBitArray validChars = TextUtils::ALPHA_NUMS;
    validChars['_'] = true;
    validChars['-'] = true;
    validChars[' '] = true;
    validChars['\''] = true;
    validChars['*']  = true;
    
    QByteArray name = n.toLocal8Bit();
    if (!TextUtils::fits(validChars, name.constData(), name.size())) {
        return false;
    }
    if (name[0] == ' ' || name[name.size() - 1] == ' ') {
        return false;
    }
    return true;
}


static QList<U2CigarToken> parceCigar(const QString& cigar) {
    QList<U2CigarToken> cigarTokens;

    QRegExp rx("(\\d+)(\\w)");

    int pos = 0;
    while ((pos = rx.indexIn(cigar, pos)) != -1) {
        if (rx.numCaptures() != 2) {
            break;
        }
        int count = rx.cap(1).toInt();
        QString cigarChar = rx.cap(2);

        if (cigarChar == "M") {
            cigarTokens.append( U2CigarToken( U2CigarOp_M, count) );
        } else if (cigarChar == "I") {
            cigarTokens.append( U2CigarToken( U2CigarOp_I, count));
        } else if (cigarChar == "D") {
            cigarTokens.append( U2CigarToken( U2CigarOp_D, count));
        } else if (cigarChar == "X") {
            cigarTokens.append( U2CigarToken( U2CigarOp_X, count));
        } else {
            break;
        }

        pos += rx.matchedLength();
    }


    return cigarTokens;

}


static QString getAlignmentTip(const QString& ref, const QList<U2CigarToken>& tokens, int maxVisibleSymbols) {
    QString alignmentTip;

    if (tokens.size() == 0) {
        return ref;
    }

    int pos = 0;

    QList<int> mismatchPositions;

    foreach (const U2CigarToken& t, tokens) {
        if (t.op == U2CigarOp_M) {
            alignmentTip += ref.mid(pos, t.count);
            pos += t.count;
        } else if (t.op == U2CigarOp_X) {
            alignmentTip += ref.mid(pos, t.count);
            mismatchPositions.append(pos);
            pos += t.count;
        } else if (t.op == U2CigarOp_I) {
            // gap already present in sequence?
            pos += t.count;
        }
    }

    if (alignmentTip.length()  > maxVisibleSymbols) {
        alignmentTip = alignmentTip.left(maxVisibleSymbols);
        alignmentTip += " ... ";
    }

    // make mismatches bold
    int offset = 0;
    static const int OFFSET_LEN = QString("<b></b>").length();
    foreach(int pos, mismatchPositions) {
        int newPos = pos + offset;
        if (newPos + 1 >= alignmentTip.length() ) {
            break;
        }
        alignmentTip.replace(newPos, 1,  QString("<b>%1</b>").arg( alignmentTip.at(newPos) ) );
        offset += OFFSET_LEN;
    }

    return alignmentTip;
}


QString Annotation::getQualifiersTip(int maxRows, U2SequenceObject* seq, DNATranslation* comlTT, DNATranslation* aminoTT) const {
    QString tip;
    int rows = 0;
    const int QUALIFIER_VALUE_CUT = 40;

    QString cigar, ref;
    if (d->qualifiers.size() != 0) {
        tip += "<nobr>";
        bool first = true;
        foreach (const U2Qualifier& q, d->qualifiers) {
            if (++rows > maxRows) {
                break;
            }
            if (q.name == QUALIFIER_NAME_CIGAR) {
                cigar = q.value;
            } else if (q.name == QUALIFIER_NAME_SUBJECT) {
                ref = q.value;
                continue;
            }
            QString val = q.value;
            if(val.length() > QUALIFIER_VALUE_CUT) {
                val = val.left(QUALIFIER_VALUE_CUT) + " ...";
            }
            if (first) {
                first = false;
            } else {
                tip +=  "<br>";
            }
            tip += "<b>" + Qt::escape(q.name) + "</b> = " + Qt::escape(val);
        }
        tip += "</nobr>";
    }

    if (cigar.size() > 0 && ref.size() > 0) {
        QList<U2CigarToken> tokens = parceCigar(cigar);
        QString alignmentTip = getAlignmentTip(ref, tokens, QUALIFIER_VALUE_CUT);
        tip += "<br><b>Reference</b> = " + alignmentTip;
        rows++;
    }

    bool canShowSeq = true;
    int seqLen = seq ? seq->getSequenceLength() : 0;
    foreach(const U2Region& r, d->getRegions()) {
        if (r.endPos() > seqLen) {
            canShowSeq = false;
        }
    }

    if (seq && rows <= maxRows && (getStrand().isCompementary() || comlTT != NULL) && canShowSeq) {
        QVector<U2Region> loc = getRegions();
        if (getStrand().isCompementary()) {
            qStableSort(loc.begin(), loc.end(), qGreater<U2Region>()); 
        }
        QString seqVal;
        QString aminoVal;
        bool complete = true;
        for (int i = 0; i < loc.size(); i++)
        {
            if (!seqVal.isEmpty()) {
                seqVal += "^";
            }
            if (!aminoVal.isEmpty()) {
                aminoVal += "^";
            }
            const U2Region& r = loc.at(i);
            int len = qMin(int(r.length), QUALIFIER_VALUE_CUT - seqVal.length());
            if (len != r.length) {
                complete = false;
            }
            if (getStrand().isCompementary() && comlTT != NULL) {
                QByteArray ba = seq->getSequenceData(U2Region(r.endPos() - len, len));
                comlTT->translate(ba.data(), len);
                TextUtils::reverse(ba.data(), len);
                seqVal += QString::fromLocal8Bit(ba.data(), len);
                if (aminoTT!=NULL) {
                    int aminoLen = aminoTT->translate(ba.data(), len);
                    aminoVal += QString::fromLocal8Bit(ba.data(), aminoLen);
                }
            } else {
                QByteArray ba = seq->getSequenceData(U2Region(r.startPos, len));
                seqVal += QString::fromLocal8Bit(ba.constData(), len);
                if (aminoTT!=NULL) {
                    int aminoLen = aminoTT->translate(ba.data(), len);
                    aminoVal += QString::fromLocal8Bit(ba.data(), aminoLen);
                }
            }
            if (seqVal.length() >= QUALIFIER_VALUE_CUT) {
                complete &= (i == loc.size() - 1);
                break;
            }
        }
        if(!complete || seqVal.length() > QUALIFIER_VALUE_CUT) {
            seqVal = seqVal.left(QUALIFIER_VALUE_CUT) + " ...";
        }
        if(!complete || aminoVal.length() > QUALIFIER_VALUE_CUT) {
            aminoVal = aminoVal.left(QUALIFIER_VALUE_CUT) + " ...";
        }
        if (!tip.isEmpty()) {
            tip+="<br>";
        }
        assert(seqVal.length() > 0);
        tip += "<nobr><b>" + U2::AnnotationTableObject::tr("Sequence") + "</b> = " + Qt::escape(seqVal) + "</nobr>";
        rows++;
        
        if (rows <= maxRows && aminoTT!=NULL) {
            tip+="<br>";
            tip += "<nobr><b>" + U2::AnnotationTableObject::tr("Translation") + "</b> = " + Qt::escape(aminoVal) + "</nobr>";
        }
    }
    return tip;
}

QStringList Annotation::getFullGroupsNames() const {
    QStringList res;
    foreach(AnnotationGroup* g, getGroups()) {
        res << g->getGroupPath();
    }
    return res;
}

bool Annotation::annotationLessThan(Annotation *first, Annotation *second) {
    QListIterator<AnnotationGroup *> firstIterator(first->getGroups());
    QListIterator<AnnotationGroup *> secondIterator(second->getGroups());
    while(firstIterator.hasNext() && secondIterator.hasNext()) {
        if (firstIterator.peekNext()->getGroupName() < secondIterator.peekNext()->getGroupName()) {
            return true;
        }
        if (firstIterator.peekNext()->getGroupName() > secondIterator.peekNext()->getGroupName()) {
            return false;
        }
        firstIterator.next();
        secondIterator.next();
    }
    if (secondIterator.hasNext()) {
        if(first->getAnnotationName() < secondIterator.peekNext()->getGroupName()) {
            return true;
        }
        if(first->getAnnotationName() > secondIterator.peekNext()->getGroupName()) {
            return false;
        }
        secondIterator.next();
    }
    if (firstIterator.hasNext()) {
        if(firstIterator.peekNext()->getGroupName() < second->getAnnotationName()) {
            return true;
        }
        if(firstIterator.peekNext()->getGroupName() > second->getAnnotationName()) {
            return false;
        }
        firstIterator.next();
    }
    if (secondIterator.hasNext()) {
        return true;
    }
    if (firstIterator.hasNext()) {
        return false;
    }
    return (first->getAnnotationName() < second->getAnnotationName());
}

const QVector<U2Qualifier>& Annotation::getQualifiers() const {
    return d->qualifiers;
}

void Annotation::setAnnotationName(const QString& newName) {
    if (newName == d->name) {
        return;
    }
    SAFE_POINT(!newName.isEmpty(), "Annotation name is empty!",);
    QString oldName = d->name;
    d->name = newName;
    if (obj!=NULL) {
        obj->setModified(true);
        AnnotationModification md(AnnotationModification_NameChanged, this);
        obj->emit_onAnnotationModified(md);
    }
}

void Annotation::setLocationOperator(U2LocationOperator op) {
    if (op == getLocationOperator()) {
        return;
    }
    d->setLocationOperator(op);
    if (obj!=NULL) {
        obj->setModified(true);
        AnnotationModification md(AnnotationModification_LocationChanged, this);
        obj->emit_onAnnotationModified(md);
    }
}

void Annotation::setStrand(U2Strand s) {
    if (s == getStrand()) {
        return;
    }
    d->setStrand(s);
    if (obj!=NULL) {
        obj->setModified(true);
        AnnotationModification md(AnnotationModification_LocationChanged, this);
        obj->emit_onAnnotationModified(md);
    }
}


void Annotation::addLocationRegion(const U2Region& reg) {
    d->location->regions.append(reg);
    if (obj!=NULL) {
        obj->setModified(true);
        AnnotationModification md(AnnotationModification_LocationChanged, this);
        obj->emit_onAnnotationModified(md);
    }
}

void Annotation::addQualifier(const U2Qualifier& q) {
    d->qualifiers.append(q);
    if (obj!=NULL) {
        obj->setModified(true);
        QualifierModification md(AnnotationModification_QualifierAdded, this, q);
        obj->emit_onAnnotationModified(md);
    }
}


void Annotation::removeQualifier(const U2Qualifier& q) {
    assert(d->qualifiers.contains(q));
    int idx = d->qualifiers.indexOf(q);
    d->qualifiers.remove(idx);
    if (obj!=NULL) {
        obj->setModified(true);
        QualifierModification md(AnnotationModification_QualifierRemoved, this, q);
        obj->emit_onAnnotationModified(md);
    }
}

void Annotation::setLocation(const U2Location& location ) {
    d->location = location;
    if (obj!=NULL) {
        AnnotationModification md(AnnotationModification_LocationChanged, this);
        obj->emit_onAnnotationModified(md);
    }
}

void Annotation::replaceRegions(const QVector<U2Region>& regions) {
    if (d->location->regions == regions) {
        return;
    }
    d->location->regions = regions;
    if (obj!=NULL) {
        AnnotationModification md(AnnotationModification_LocationChanged, this);
        obj->emit_onAnnotationModified(md);
    }
}

bool Annotation::isCaseAnnotation() const {
    return caseAnnotation;
}

void Annotation::setCaseAnnotation(bool caseAnnotation) {
    this->caseAnnotation = caseAnnotation;
}

//////////////////////////////////////////////////////////////////////////
// Group

AnnotationGroup::AnnotationGroup(AnnotationTableObject* p, const QString& _name, AnnotationGroup* parentGrp) 
: name(_name), obj(p), parentGroup(parentGrp)
{
    assert(!name.isEmpty() && (!name.contains('/') || name == AnnotationGroup::ROOT_GROUP_NAME));
}

AnnotationGroup::~AnnotationGroup() {
    //annotations are not removed here -> contract with ~AnnotationTableObject
    foreach(AnnotationGroup* g, subgroups) {
        delete g;
    }
}
void AnnotationGroup::findAllAnnotationsInGroupSubTree(QSet<Annotation*>& set) const {
    set+=QSet<Annotation*>::fromList(annotations);
    foreach(AnnotationGroup* g, subgroups) {
        g->findAllAnnotationsInGroupSubTree(set);
    }
}

void AnnotationGroup::addAnnotation(Annotation* a) {
    if (a->getGObject() == NULL) { 
        //adding annotation to the object by adding it to the group.
        //usually this method is called for annotation with groupName!=annotationName
        obj->addAnnotation(a, getGroupPath());
        return;
    }

    SAFE_POINT(a->getGObject() == obj, "Illegal object!",);
    assert(/*!annotations.contains(a) && */! a->groups.contains(this));

    obj->setModified(true);

    annotations.append(a);
    a->groups.append(this);
    
    if (obj!=NULL && a->groups.size() > 1) {
        obj->setModified(true);
        AnnotationGroupModification md(AnnotationModification_AddedToGroup, a, this);
        obj->emit_onAnnotationModified(md);
    }
}

void AnnotationGroup::removeAnnotations(const QList<Annotation*>& ans) {
    QList<Annotation*> toRemoveFromObj;
    foreach(Annotation* a, ans) {
        assert(annotations.contains(a) && a->groups.contains(this));
        if (a->groups.size() == 1) {
            toRemoveFromObj.append(a);
        } else {
            annotations.removeOne(a);
            a->groups.removeOne(this);
            if (obj!=NULL) {
                obj->setModified(true);
                AnnotationGroupModification md(AnnotationModification_RemovedFromGroup, a, this);
                obj->emit_onAnnotationModified(md);
            }
        }
    }
    if (!toRemoveFromObj.isEmpty()) {
        obj->removeAnnotations(toRemoveFromObj);
    }
}

void AnnotationGroup::removeAnnotation(Annotation* a) {
    assert(annotations.contains(a) && a->groups.contains(this));
    if (a->groups.size() == 1) {
        SAFE_POINT(a->groups.first() == this, "Illegal group!",);
        obj->removeAnnotation(a);
    } else {
        annotations.removeOne(a);
        a->groups.removeOne(this);
        if (obj!=NULL) {
            obj->setModified(true);
            AnnotationGroupModification md(AnnotationModification_RemovedFromGroup, a, this);
            obj->emit_onAnnotationModified(md);
        }
    }
}

bool AnnotationGroup::isValidGroupName(const QString& n, bool pathMode) {
    if (n.isEmpty()) {
        return false;
    }
    //todo: optimize
    QBitArray validChars = TextUtils::ALPHA_NUMS;
    validChars['_'] = true;
    validChars['-'] = true;
    validChars[' '] = true;
    validChars['\''] = true;

    if (pathMode) {
        validChars['/'] = true;
    }
    QByteArray groupName = n.toLocal8Bit();
    if (!TextUtils::fits(validChars, groupName.constData(), groupName.size())) {
        return false;
    }
    if (groupName[0] == ' ' || groupName[groupName.size()-1] == ' ') {
        return false;
    }
    return true;
}

AnnotationGroup* AnnotationGroup::getSubgroup(const QString& path, bool create) {
    if (path.isEmpty()) {
        return this;
    }
    int idx = path.indexOf('/');
    QString name = idx < 0 ? path : (idx == 0 ? path.mid(idx+1) : path.left(idx));
    AnnotationGroup* group = NULL;
    foreach (AnnotationGroup *g, subgroups) {
        if (g->getGroupName() == name) {
            group = g;
            break;
        }
    }
    if (group == NULL  && create) {
        group = new AnnotationGroup(obj, name, this);
        subgroups.push_back(group);
        obj->emit_onGroupCreated(group);
    }
    if (idx <= 0 || group == NULL) {
        return group;
    }
    AnnotationGroup* result = group->getSubgroup(path.mid(idx+1), create);
    return result;
}

void AnnotationGroup::removeSubgroup(AnnotationGroup* g) {
    SAFE_POINT(g->getParentGroup() == this, "Illegal parent group!",);
    
    obj->setModified(true);
    g->clear();
    subgroups.removeOne(g);

    obj->emit_onGroupRemoved(this, g);
    delete g;
}

void AnnotationGroup::clear() {
    while (!subgroups.isEmpty()) {
        removeSubgroup(subgroups.first());
    }
    if (!annotations.isEmpty()) {
        removeAnnotations(annotations);
    }
}

bool AnnotationGroup::isParentOf(AnnotationGroup* g) const {
    if (g->getGObject() != obj || g == this) {
        return false;
    }
    for (AnnotationGroup* pg = g->getParentGroup(); pg!=NULL; pg = pg->getParentGroup()) {
        if ( pg == this) {
            return true;
        }
    }
    return false;
}

void AnnotationGroup::setGroupName(const QString& newName) {
    if (name == newName) {
        return;
    }
    QString oldName = name;
    name = newName;
    obj->setModified(true);
    obj->emit_onGroupRenamed(this, oldName);
}

QString AnnotationGroup::getGroupPath() const {
    if (parentGroup == NULL) {
        return QString("");
    }
    if (parentGroup->parentGroup == NULL) {
        return name;
    }
    return parentGroup->getGroupPath() + "/" + name;
}


void AnnotationGroup::getSubgroupPaths(QStringList& res) const {
    if (!isRootGroup()) {
        res.append(getGroupPath());
    }
    foreach(const AnnotationGroup* g, subgroups) {
        g->getSubgroupPaths(res);
    }
}

QDataStream& operator>>(QDataStream& dataStream, AnnotationGroup* parentGroup) {
    QString name;
    dataStream >> name;
    AnnotationGroup *group = parentGroup->getSubgroup(name, true);
    int s;
    dataStream >> s;
    for (int i = 0; i < s; ++i) {
        AnnotationData *adata = new AnnotationData;
        dataStream >> *adata;
        Annotation *a = new Annotation(QSharedDataPointer<AnnotationData>(adata));
        group->addAnnotation(a);
    }
    dataStream >> s;
    for (int i = 0; i < s; ++i)
        dataStream >> group;
    return dataStream;
}

QDataStream& operator<<(QDataStream& dataStream, const AnnotationGroup& group) {
    dataStream << group.name;
    int s = group.annotations.size();
    dataStream << s;
    for (int i = 0; i < s; ++i)
        dataStream << *group.annotations[i]->data();
    s = group.subgroups.size();
    dataStream << s;
    for (int i = 0; i < s; ++i)
        dataStream << *group.subgroups[i];
    return dataStream;
}

//////////////////////////////////////////////////////////////////////////
/// Annotation table object

const QString AnnotationGroup::ROOT_GROUP_NAME("/");

AnnotationTableObject::AnnotationTableObject(const QString& objectName, const QVariantMap& hintsMap) 
: GObject(GObjectTypes::ANNOTATION_TABLE, objectName, hintsMap) 
{
    rootGroup = new AnnotationGroup(this, AnnotationGroup::ROOT_GROUP_NAME, NULL);
}

AnnotationTableObject::~AnnotationTableObject() {
    foreach(Annotation* a, annotations) {
        delete a;
    }
    delete rootGroup;
}

GObject* AnnotationTableObject::clone(const U2DbiRef&, U2OpStatus&) const {
    GTIMER(c2,t2,"AnnotationTableObject::clone");
    AnnotationTableObject* cln = new AnnotationTableObject(getGObjectName(), getGHintsMap());
    cln->setIndexInfo(getIndexInfo());
    QMap<AnnotationGroup*, AnnotationGroup*>remap;
    cln->rootGroup = new AnnotationGroup(cln, rootGroup->getGroupName(), NULL);
    remap[rootGroup] = cln->rootGroup;
    QList<AnnotationGroup*> lst;
    lst << rootGroup->getSubgroups();
    while(!lst.isEmpty()){
        AnnotationGroup* gr = lst.takeFirst();
        AnnotationGroup* newParent = remap.value(gr->getParentGroup());
        assert(newParent);
        AnnotationGroup* newGr = new AnnotationGroup(cln, gr->getGroupName(), newParent);
        newParent->subgroups << newGr;
        remap[gr] = newGr;
        lst << gr->getSubgroups();
    }
    foreach(Annotation* a, annotations) {
        Annotation* newA = new Annotation(a->d);
        newA->obj = cln;
        cln->annotations << newA;
        foreach(AnnotationGroup* gr, a->getGroups()) {
            AnnotationGroup* newGr = remap.value(gr);
            assert(newGr);
            newA->groups << newGr;
            newGr->annotations << newA;
        }
    }
    cln->setModified(false);
    return cln;
}

void AnnotationTableObject::addAnnotation(Annotation* a, const QString& groupName) {
    SAFE_POINT(a->obj == NULL, "Annotation belongs to another object", );

    a->obj = this;
    const QString& aName = a->getAnnotationName();
    AnnotationGroup* defaultGroup = rootGroup->getSubgroup(groupName.isEmpty() ? aName : groupName, true); 
    defaultGroup->addAnnotation(a);
    annotations.append(a);
    setModified(true);

    emit si_onAnnotationsAdded(QList<Annotation*>()<<a);
}

void AnnotationTableObject::addAnnotation(Annotation* a, const QList<QString>& groupsNames) {
    SAFE_POINT(a->obj == NULL, "Annotation belongs to another object", );
    if(groupsNames.isEmpty()){
        addAnnotation(a);
        return;
    }
    a->obj = this;
    const QString& aName = a->getAnnotationName();
    foreach(QString groupName, groupsNames){
        AnnotationGroup* defaultGroup = rootGroup->getSubgroup(groupName.isEmpty() ? aName : groupName, true); 
        defaultGroup->addAnnotation(a);
    }
    annotations.append(a);
    setModified(true);

    emit si_onAnnotationsAdded(QList<Annotation*>()<<a);
}

void AnnotationTableObject::addAnnotations(const QList<Annotation*>& list, const QString& groupName) {
    if (list.isEmpty()) {
        return;
    }
    annotations << list;
    GTIMER(c1,t1,"AnnotationTableObject::addAnnotations [populate data tree]");
    AnnotationGroup* gr = NULL;
    
    if (groupName.isEmpty()) {
        QString prevName;
        foreach(Annotation* a, list) {
            assert(a->obj == NULL);
            a->obj = this;
            const QString& grName = a->getAnnotationName();
            if (grName != prevName) {
                gr = rootGroup->getSubgroup(grName, true);
                prevName = grName;
            }
            gr->addAnnotation(a);
        }
    } else {
        gr = rootGroup->getSubgroup(groupName, true);
        foreach(Annotation* a, list) {
            assert(a->obj == NULL);
            a->obj = this;
            gr->addAnnotation(a);
        }
    }
    
    t1.stop();
    setModified(true);

    GTIMER(c2,t2,"AnnotationTableObject::addAnnotations [notify]");
    emit si_onAnnotationsAdded(list);
}


void AnnotationTableObject::removeAnnotations(const QList<Annotation*>& annotations) {
    
    foreach(Annotation* a, annotations) {
        _removeAnnotation(a);
    }
    emit si_onAnnotationsRemoved(annotations);
    setModified(true);
    qDeleteAll(annotations);
}

void AnnotationTableObject::removeAnnotationsInGroup(const QList<Annotation*>& _annotations, AnnotationGroup *group) {
    /*annotations = annotations.toSet().subtract(_annotations.toSet()).toList();
    foreach(Annotation* a, _annotations) {
        a->obj = NULL;
        foreach(AnnotationGroup* ag, a->getGroups()) {
            ag->annotations.removeOne(a);
        }
    }*/
    int recv = receivers(SIGNAL(si_onAnnotationsInGroupRemoved(const QList<Annotation*>&, AnnotationGroup*)));
    annLocker.setToDelete(_annotations, group, recv);
    DeleteAnnotationsFromObjectTask *task = new DeleteAnnotationsFromObjectTask(_annotations, this, group);
    AppContext::getTaskScheduler()->registerTopLevelTask(task);
    
    /*int recv = receivers(SIGNAL(si_onAnnotationsInGroupRemoved(const QList<Annotation*>&, AnnotationGroup*)));
    annLocker.setToDelete(_annotations, group, recv);
    emit si_onAnnotationsInGroupRemoved(_annotations, group);
    setModified(true);*/
    //qDeleteAll(annotations);
}

void DeleteAnnotationsFromObjectTask::run() {
    /*aobj->annotations = aobj->annotations.toSet().subtract(anns.toSet()).toList();
    foreach(Annotation* a, anns) {
        a->obj = NULL;
        foreach(AnnotationGroup* ag, a->getGroups()) {
            ag->annotations.removeOne(a);
        }
    }*/
    foreach(Annotation* a, anns) {
        aobj->_removeAnnotation(a);
    }
}

Task::ReportResult DeleteAnnotationsFromObjectTask::report() {
    aobj->emit_onAnnotationsInGroupRemoved(anns, group);
    aobj->setModified(true);
    return ReportResult_Finished;
}

void AnnotationTableObject::releaseLocker() {
    annLocker.releaseLocker();
}

bool AnnotationTableObject::isLocked() const {
    return annLocker.isLocked();
}

void AnnotationTableObject::removeAnnotation(Annotation* a) {
    QList<Annotation*> v; 
    v<<a;
    _removeAnnotation(a);
    setModified(true);
    emit si_onAnnotationsRemoved(v);
    delete a;
}

void AnnotationTableObject::_removeAnnotation(Annotation* a) {
    SAFE_POINT(a->getGObject() == this, "Illegal annotation object!",);
    a->obj = NULL;
    annotations.removeOne(a);
    foreach(AnnotationGroup* ag, a->getGroups()) {
        ag->annotations.removeOne(a);
    }
}

void AnnotationTableObject::selectAnnotationsByName(const QString& name, QList<Annotation*>& res) {
    foreach(Annotation* a, annotations) {
        if (a->getAnnotationName() == name) {
            res.append(a);
        }
    }
}


bool AnnotationTableObject::checkConstraints(const GObjectConstraints* c) const {
    const AnnotationTableObjectConstraints* ac = qobject_cast<const AnnotationTableObjectConstraints*>(c);
    SAFE_POINT(ac != NULL, "Illegal constraints type!", false);

    int fitSize = ac->sequenceSizeToFit;
    foreach(Annotation* a, annotations) {
        foreach(const U2Region& r, a->getRegions()) {
            if (r.endPos() > fitSize) {
                return false;
            }
        }
    }
    return true;
}

void AnnotationTableObject::cleanAnnotations() {
    assert(!annLocker.isLocked());
    annLocker.sl_Clean();

}
AnnotationTableObjectConstraints::AnnotationTableObjectConstraints(const AnnotationTableObjectConstraints& c, QObject* p) 
: GObjectConstraints(GObjectTypes::ANNOTATION_TABLE, p), sequenceSizeToFit(c.sequenceSizeToFit)
{

}

AnnotationTableObjectConstraints::AnnotationTableObjectConstraints(QObject* p) 
: GObjectConstraints(GObjectTypes::ANNOTATION_TABLE, p), sequenceSizeToFit(0) 
{
}


bool annotationLessThanByRegion(const Annotation* a1, const Annotation* a2) {
    assert(!a1->getLocation()->isEmpty());
    assert(!a2->getLocation()->isEmpty());

    const U2Region& r1 = a1->getRegions().first();
    const U2Region& r2 = a2->getRegions().first();
    return r1 < r2;
}

bool annotationGreaterThanByRegion( const Annotation* a1, const Annotation* a2 ) {
    return annotationLessThanByRegion(a2, a1);
}


void AnnotationsLocker::setToDelete( const QList<Annotation*>& _anns, AnnotationGroup *_parentGroup, int counter ) {
    anns = _anns;
    parentGroup = _parentGroup;
    deleteCounter = counter;
}

void AnnotationsLocker::releaseLocker(){
    if(deleteCounter) {
        deleteCounter--;
    }
}

bool AnnotationsLocker::isLocked() const{
    return deleteCounter != 0;
}

void AnnotationsLocker::sl_Clean(){
    if(deleteCounter == 0) {
        qDeleteAll(anns);
        anns.clear();
        parentGroup->getParentGroup()->removeSubgroup(parentGroup);
    }
}

AnnotationsLocker::AnnotationsLocker(): parentGroup(NULL), deleteCounter(0) {
}


//////////////////////////////////////////////////////////////////////////
/// Features table object

FeaturesTableObject::FeaturesTableObject( const QString& objectName, const U2DbiRef& dbiRef, const QVariantMap& hintsMap)
: GObject(GObjectTypes::ANNOTATION_TABLE, objectName+"_features", hintsMap) {

    aObject = new AnnotationTableObject(objectName, hintsMap);
    initRootFeature(dbiRef);
    entityRef = U2EntityRef(dbiRef, rootFeature.id);

    connect(aObject, SIGNAL( si_onAnnotationsRemoved(const QList<Annotation*>& ) ), SLOT( sl_onAnnotationsRemoved(const QList<Annotation*>& )));
    connect(aObject, SIGNAL( si_onAnnotationsInGroupRemoved(const QList<Annotation*>&, AnnotationGroup*) ), 
        SLOT( sl_onAnnotationsInGroupRemoved(const QList<Annotation*>&, AnnotationGroup*)));
    connect(aObject, SIGNAL( si_onAnnotationModified(const AnnotationModification& ) ), SLOT( sl_onAnnotationModified(const AnnotationModification& ) ));
    connect(aObject, SIGNAL( si_onGroupRemoved(AnnotationGroup* , AnnotationGroup* ) ), SLOT( sl_onGroupRemoved(AnnotationGroup* , AnnotationGroup* ) ));
    connect(aObject, SIGNAL( si_onGroupRenamed(AnnotationGroup*, const QString& ) ), SLOT( sl_onGroupRenamed(AnnotationGroup*, const QString& ) ));
}

void FeaturesTableObject::initRootFeature(const U2DbiRef& dbiRef){
    rootFeature.name = getGObjectName();

    U2OpStatus2Log os;

    DbiConnection con(dbiRef, os);
    CHECK_OP(os, );

    U2FeatureDbi* fdbi = con.dbi->getFeatureDbi();
    SAFE_POINT(fdbi!=NULL, "Features dbi is NULL", );
    
    fdbi->createFeature(rootFeature, QList<U2FeatureKey>(), os);
    CHECK_OP(os, );
}

FeaturesTableObject::~FeaturesTableObject(){
    delete aObject;
    //TODO: remove root feature
}

void FeaturesTableObject::addAnnotation( Annotation* a, const QString& groupName ){
    aObject->addAnnotation(a, groupName);

    importToDbi(a);

    emit si_onAnnotationsAdded(QList<Annotation*>()<<a);
}

void FeaturesTableObject::addAnnotation( Annotation* a, const QList<QString>& groupsNames ){
    aObject->addAnnotation(a, groupsNames);

    importToDbi(a);

    emit si_onAnnotationsAdded(QList<Annotation*>()<<a);
}

void FeaturesTableObject::addAnnotations( const QList<Annotation*>& annotations, const QString& groupName ){
    aObject->addAnnotations(annotations, groupName);

    foreach(Annotation* a, annotations){
        importToDbi(a);
    }

    emit si_onAnnotationsAdded(annotations);
}

void FeaturesTableObject::removeAnnotation( Annotation* a ){
    U2OpStatus2Log os;
    synchronizer.removeFeature(a, rootFeature.id, entityRef.dbiRef, os);
    //CHECK_OP(os, );  

    aObject->removeAnnotation(a);
}

void FeaturesTableObject::removeAnnotations( const QList<Annotation*>& annotations ){
    U2OpStatus2Log os;
    foreach(Annotation* a, annotations){
        synchronizer.removeFeature(a, rootFeature.id, entityRef.dbiRef, os);
        //CHECK_OP(os, );
    }

    aObject->removeAnnotations(annotations);
}

GObject* FeaturesTableObject::clone( const U2DbiRef& ref, U2OpStatus& os ) const{
    Q_UNUSED(os);
    GTIMER(c2,t2,"FeaturesTableObject::clone");
    FeaturesTableObject* cln = new FeaturesTableObject(getGObjectName(), ref, getGHintsMap());
    cln->setIndexInfo(getIndexInfo());
    QMap<AnnotationGroup*, AnnotationGroup*>remap;
    cln->aObject->rootGroup = new AnnotationGroup(cln->aObject, rootGroup->getGroupName(), NULL);
    remap[rootGroup] = cln->aObject->rootGroup;
    QList<AnnotationGroup*> lst;
    lst << rootGroup->getSubgroups();
    while(!lst.isEmpty()){
        AnnotationGroup* gr = lst.takeFirst();
        AnnotationGroup* newParent = remap.value(gr->getParentGroup());
        assert(newParent);
        AnnotationGroup* newGr = new AnnotationGroup(cln->aObject, gr->getGroupName(), newParent);
        newParent->subgroups << newGr;
        remap[gr] = newGr;
        lst << gr->getSubgroups();
    }
    foreach(Annotation* a, annotations) {
        Annotation* newA = new Annotation(a->d);
        newA->obj = cln->aObject;
        cln->aObject->annotations << newA;
        foreach(AnnotationGroup* gr, a->getGroups()) {
            AnnotationGroup* newGr = remap.value(gr);
            assert(newGr);
            newA->groups << newGr;
            newGr->annotations << newA;
        }
    }
    cln->aObject->setModified(false);
    return cln;
}

void FeaturesTableObject::selectAnnotationsByName( const QString& name, QList<Annotation*>& res ){
    aObject->selectAnnotationsByName(name, res);
}

bool FeaturesTableObject::checkConstraints( const GObjectConstraints* c ) const{
    return aObject->checkConstraints(c);
}

void FeaturesTableObject::removeAnnotationsInGroup( const QList<Annotation*>& _annotations, AnnotationGroup *group ){
   aObject->removeAnnotationsInGroup(_annotations, group); 
   //TODO: features?? connect to remove slot
}

void FeaturesTableObject::releaseLocker(){
    aObject->releaseLocker();
}

bool FeaturesTableObject::isLocked() const{
    return aObject->isLocked();
}

void FeaturesTableObject::cleanAnnotations(){
    U2OpStatus2Log os;
    synchronizer.removeFeature(rootFeature.id, entityRef.dbiRef, os);
    //CHECK_OP(os, );

    aObject->cleanAnnotations();
}

void FeaturesTableObject::_removeAnnotation( Annotation* a ){
    U2OpStatus2Log os;
    synchronizer.removeFeature(a, rootFeature.id, entityRef.dbiRef, os);
    //CHECK_OP(os, );  

    aObject->_removeAnnotation(a);
}

void FeaturesTableObject::importToDbi( Annotation* a ){
    SAFE_POINT(a->obj != NULL, "Annotation must be assigned to an object", );

    U2OpStatus2Log os;
    synchronizer.exportAnnotationToFeatures(a, rootFeature.id, entityRef.dbiRef, os);
    CHECK_OP(os, );
}

// direct interface to dbi

void FeaturesTableObject::addFeature(U2Feature &f, U2OpStatus &os) {
    addFeature(f, QList<U2FeatureKey>(), os);
}

void FeaturesTableObject::addFeature(U2Feature &f, QList<U2FeatureKey> keys, U2OpStatus &os) {
    if(f.parentFeatureId.isEmpty()) {
        f.parentFeatureId = rootFeature.id;
    }
    // TODO: should we set sequenceId too? Seems logical that all subfeatures has same sequenceId
    // f.sequenceId = rootFeature.sequenceId;

    DbiConnection con;
    con.open(entityRef.dbiRef, os);
    CHECK_OP(os, );

    con.dbi->getFeatureDbi()->createFeature(f, keys, os);
}

U2Feature FeaturesTableObject::getFeature(U2DataId id, U2OpStatus &os) {
    DbiConnection con;
    con.open(entityRef.dbiRef, os);
    CHECK_OP(os, U2Feature());

    return con.dbi->getFeatureDbi()->getFeature(id, os);
}

QList<U2Feature> FeaturesTableObject::getSubfeatures(U2DataId parentFeatureId, U2OpStatus &os, bool recursive) {
    DbiConnection con;
    con.open(entityRef.dbiRef, os);
    CHECK_OP(os, QList<U2Feature>());

    return U2FeaturesUtils::getSubFeatures(parentFeatureId, con.dbi->getFeatureDbi(), os, recursive);
}

//slots

void FeaturesTableObject::sl_onAnnotationsRemoved( const QList<Annotation*>& a ){
    emit si_onAnnotationsRemoved(a);
}

void FeaturesTableObject::sl_onAnnotationsInGroupRemoved( const QList<Annotation*>& a, AnnotationGroup* g){
    //TODO
    emit_onAnnotationsInGroupRemoved(a, g);
}

void FeaturesTableObject::sl_onAnnotationModified( const AnnotationModification& md ){
    U2OpStatus2Log os;
    synchronizer.modifyFeatures(md, rootFeature.id, entityRef.dbiRef, os);

    
    emit_onAnnotationModified(md);
}

void FeaturesTableObject::sl_onGroupCreated( AnnotationGroup* g){
    //TODO: how to create an empty group?

    emit_onGroupCreated(g);
}

void FeaturesTableObject::sl_onGroupRemoved( AnnotationGroup* p, AnnotationGroup* removed ){
    //delete groups from features
    U2OpStatus2Log os;
    synchronizer.removeGroup(p, removed, rootFeature.id, entityRef.dbiRef, os);

    emit_onGroupRemoved(p, removed);
}

void FeaturesTableObject::sl_onGroupRenamed( AnnotationGroup* g, const QString& oldName ){
    //rename group in features
    U2OpStatus2Log os;
    synchronizer.renameGroup(g, oldName, rootFeature.id, entityRef.dbiRef, os);

    emit_onGroupRenamed(g, oldName);
}

}//namespace

