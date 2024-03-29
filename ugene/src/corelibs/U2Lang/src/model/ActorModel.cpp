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

#include "ActorModel.h"
#include "ConfigurationEditor.h"

namespace U2 {
namespace Workflow {

/**************************
 * Actor
 **************************/
Actor::Actor(const ActorId &actorId, ActorPrototype* proto, AttributeScript * _script)
: id(actorId), proto(proto), doc(NULL), script(_script) {
    if(script == NULL) {
        if(proto->isScriptFlagSet()) {
            script = new AttributeScript();
            script->setScriptText("");   
        }
        else {
            script = NULL;
        }
    }

    if(script != NULL) {
        setupVariablesForPort();
        setupVariablesForAttribute();
    }
}

Actor::Actor(const Actor&) : QObject(), Configuration(), Peer() {
    assert(false);
}

Actor::~Actor() {
    qDeleteAll(ports.values()); 
    delete doc;
    delete script;
}

void Actor::setupVariablesForPort() {
    foreach(const PortDescriptor *descr, proto->getPortDesciptors()) {
        QString prefix;
        if(descr->isInput()) {
            prefix = "in_";
        }
        else {
            prefix = "out_";
        }

        DataTypePtr dataTypePtr = descr->getType();
        if(dataTypePtr->isMap()) {
            QMap<Descriptor, DataTypePtr> map = dataTypePtr->getDatatypesMap();
            foreach(const Descriptor & d, map.keys()) {
                Descriptor var(prefix + d.getId(), d.getDisplayName(), d.getDocumentation());
                script->setScriptVar(var, QVariant());
            }
        }
        else  if(dataTypePtr->isList()) {
            foreach(const Descriptor & typeDescr, dataTypePtr->getAllDescriptors()) {
                Descriptor var(prefix + typeDescr.getId(), typeDescr.getDisplayName(), typeDescr.getDocumentation());
                script->setScriptVar(var, QVariant());
            }
        }
        else {
            QString id = prefix + dataTypePtr->getId();
            QString displayName = dataTypePtr->getDisplayName();
            QString doc = prefix + dataTypePtr->getDocumentation();
            script->setScriptVar(Descriptor(id,displayName,doc), QVariant());
        }
    }
}

void Actor::setupVariablesForAttribute() {
    foreach( Attribute * attribute, proto->getAttributes() ) {
        assert(attribute != NULL);
        QString attrVarName = attribute->getDisplayName();
        script->setScriptVar(Descriptor(attribute->getId().replace(".","_"), attrVarName.replace(".", "_"), attribute->getDocumentation()), QVariant());
    }
}

AttributeScript * Actor::getScript() const {
    return script;
} 

void Actor::setScript(AttributeScript* _script) {
    script->setScriptText(_script->getScriptText());
}

ActorId Actor::getOwner() const {
    return owner;
}

void Actor::setOwner(const ActorId &owner) {
    this->owner = owner;
}

void Actor::updateActorIds(const QMap<ActorId, ActorId> &actorIdsMap) {
    if (actorIdsMap.contains(owner)) {
        owner = actorIdsMap[owner];
    }

    foreach (Attribute *a, this->getAttributes()) {
        a->updateActorIds(actorIdsMap);
    }
}

ActorPrototype * Actor::getProto() const {
    return proto;
}

ActorId Actor::getId() const {
    return id;
}

void Actor::setId(const ActorId &id) {
    this->id = id;
}

QString Actor::getLabel() const {
    if( label.isEmpty() ) {
        return QString("%1 %2").arg(getProto()->getDisplayName()).arg(getId());
    } else {
        return label;
    }
}

void Actor::setLabel(const QString& l) {
    label = l; 
    emit si_labelChanged();
}

Port * Actor::getPort(const QString& id) const {
    return ports.value(id);
}

QList<Port*> Actor::getPorts() const {
    return ports.values();
}

QList<Port*> Actor::getInputPorts() const {
    QList<Port*> l; foreach (Port* p, ports.values()) if (p->isInput()) l<<p; 
    return l;
}

QList<Port*> Actor::getOutputPorts() const {
    QList<Port*> l; foreach (Port* p, ports.values()) if (p->isOutput()) l<<p; 
    return l;
}

void Actor::setParameter(const QString& name, const QVariant& val) {
    Configuration::setParameter(name, val);
    emit si_modified();
}

ActorDocument * Actor::getDescription() const {
    return doc;
}

void Actor::setDescription(ActorDocument* d) {
    assert(d != NULL);
    doc = d;
}

const QMap<QString, QString> & Actor::getParamAliases() const {
    return paramAliases;
}

QMap<QString, QString> & Actor::getParamAliases() {
    return paramAliases;
}

bool Actor::hasParamAliases() const {
    return !paramAliases.isEmpty();
}

const QMap<QString, QString> & Actor::getAliasHelp() const {
    return aliasHelpDescs;
}

QMap<QString, QString> & Actor::getAliasHelp() {
    return aliasHelpDescs;
}

bool Actor::hasAliasHelp() const {
    foreach(const QString & alias, paramAliases.values()) {
        if(aliasHelpDescs.contains(alias)) {
            return true;
        }
    }
    return false;
}

void Actor::remap(const QMap<ActorId, ActorId>& m) {
    foreach(Port* p, ports) {
        p->remap(m);
    }
}

/**************************
 * ActorPrototype
 **************************/
QList<PortDescriptor*> ActorPrototype::getPortDesciptors() const {
    return ports;
}

QList<Attribute*> ActorPrototype::getAttributes() const {
    return attrs;
}

void ActorPrototype::addAttribute( Attribute * a ) {
    assert(a != NULL);
    attrs << a;
}

int ActorPrototype::removeAttribute( Attribute * attr ) {
    assert(attr != NULL);
    return attrs.removeAll( attr );
}

void ActorPrototype::setEditor(ConfigurationEditor* e) {
    assert(e != NULL);
    ed = e;
}

ConfigurationEditor * ActorPrototype::getEditor()const {
    return ed;
}

void ActorPrototype::setValidator(ConfigurationValidator* v) {
    assert(v != NULL);
    val = v;
}

void ActorPrototype::setPrompter(Prompter* p) {
    assert(p != NULL);
    prompter = p;
}

void ActorPrototype::setPortValidator(const QString& id, ConfigurationValidator* v) {
    assert(v != NULL);
    portValidators[id] = v;
}

bool ActorPrototype::isAcceptableDrop(const QMimeData*, QVariantMap* ) const {
    return false;
}

Port* ActorPrototype::createPort(const PortDescriptor& d, Actor* p) {
    return new Port(d, p);
}

Actor* ActorPrototype::createInstance(const ActorId &actorId, AttributeScript *script, const QVariantMap& params) {
    Actor* proc = new Actor(actorId, this, script);

    foreach(PortDescriptor* pd, getPortDesciptors()) {
        Port* p = createPort(*pd, proc);
        QString pid = pd->getId();
        if (portValidators.contains(pid)) {
            p->setValidator(portValidators.value(pid));
        }
        proc->ports[pid] = p;
    }
    foreach(Attribute* a, getAttributes()) {
        proc->addParameter(a->getId(), a->clone());
    }
    if (val) {
        proc->setValidator(val);
    }
    if (prompter) {
        proc->setDescription(prompter->createDescription(proc));
    }

    QMapIterator<QString, QVariant> i(params);
    while (i.hasNext()) {
        i.next();
        proc->setParameter(i.key(), i.value());
    }
    if (ed) {
        ActorConfigurationEditor *actorEd = dynamic_cast<ActorConfigurationEditor*>(ed);
        if (NULL != actorEd) {
            ActorConfigurationEditor *editor = dynamic_cast<ActorConfigurationEditor*>(ed->clone());
            editor->setConfiguration(proc);
            proc->setEditor(editor);
        } else {
            proc->setEditor(ed);
        }
    }
    return proc;
}


Attribute * ActorPrototype::getAttribute( const QString & id ) const {
    Attribute * res = NULL;
    foreach( Attribute * a, attrs ) {
        if( a->getId() == id ) {
            res = a;
            break;
        }
    }
    return res;
}

void ActorPrototype::setScriptFlag(bool flag) {
    isScript = flag;
}

void ActorPrototype::setNonStandard(const QString &path) {
    isStandard = false;
    actorFilePath = path;
}

void ActorPrototype::setSchema(const QString &path) {
    isStandard = false;
    isSchema = true;
    actorFilePath = path;
}

void ActorPrototype::addSlotRelation(const QString &headPortId, const QString headSlot, const QString &depPortId, const QString &depSlot) {
    QString dep = depPortId + "." + depSlot;
    QString head = headPortId + "." + headSlot;
    slotRelations[dep] = head;
}

QStrStrMap ActorPrototype::getSlotRelations() const {
    return slotRelations;
}

ActorPrototype::ActorPrototype(const Descriptor& d, 
                               const QList<PortDescriptor*>& ports, 
                               const QList<Attribute*>& attrs)
: VisualDescriptor(d), attrs(attrs), ports(ports), ed(NULL), val(NULL), prompter(NULL),
isScript(false), isStandard(true), isSchema(false), allowsEmptyPorts(false), influenceOnPathFlag(false) {
}

ActorPrototype::~ActorPrototype()
{
    qDeleteAll(attrs);
    qDeleteAll(ports);
    delete ed;
    delete val;
    delete prompter;
    qDeleteAll(portValidators);
}

/**************************
 * ActorDocument
 **************************/
ActorDocument::ActorDocument(Actor* a) : QTextDocument(a), target(a) {
}

//void ActorDocument::update(const QVariantMap& ) {
//}

ActorConfigurationEditor::ActorConfigurationEditor(const ActorConfigurationEditor &other)
: ConfigurationEditor(other)
{
    cfg = other.cfg;
}

}//Workflow namespace
}//GB2namespace
