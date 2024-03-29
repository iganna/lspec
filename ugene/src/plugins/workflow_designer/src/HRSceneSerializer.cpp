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

#include <U2Core/Log.h>
#include <U2Lang/Aliasing.h>
#include <U2Lang/WorkflowUtils.h>
#include <U2Lang/HRSchemaSerializer.h>

#include "ItemViewStyle.h"
#include "HRSceneSerializer.h"

#include <QtCore/QTextStream>

namespace U2 {

static const QString DESCRIPTION_ATTR   = "description";
static const QString TOOLTIP_ATTR       = "tooltip";
static const QString POSITION_ATTR      = "pos";
static const QString STYLE_ATTR         = "style";
static const QString BG_COLOR           = "bg-color-";
static const QString FONT               = "font-";
static const QString BOUNDS             = "bounds";
static const QString PORT_ANGLE         = "angle";
static const QString TEXT_POS_ATTR      = "text-pos";

const QString UNEXPECTED_END_OF_FILE = HRSceneSerializer::tr("Unexpected end of file");

/***************************************
 * Writing support
 ***************************************/
using namespace U2;

static const int VISUAL_BLOCK_TABS_NUM = 4;

static QString point2String(const QPointF & point) {
    return QString("%1 %2").arg(point.x()).arg(point.y());
}

static QString rect2String(const QRectF & rect) {
    return QString("%1 %2").arg(point2String(rect.topLeft())).arg(point2String(rect.bottomRight()));
}

static QString extendedStyleData(ExtendedProcStyle * style) {
    QString res;
    if(style == NULL) {
        return res;
    }
    if( !style->isAutoResized() ) {
        res += HRSchemaSerializer::makeEqualsPair(BOUNDS, rect2String(style->boundingRect()), VISUAL_BLOCK_TABS_NUM);
    }
    return res;
}

static QString color2String(const QColor & color) {
    return QString("%1 %2 %3 %4").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
}

static QString styleData(ItemViewStyle * style) {
    QString res;
    if( style == NULL ) {
        return res;
    }

    if( style->getBgColor() != style->defaultColor() ) {
        res += HRSchemaSerializer::makeEqualsPair(BG_COLOR + style->getId(), 
            color2String(style->getBgColor()), VISUAL_BLOCK_TABS_NUM);
    }
    if( style->defaultFont() != QFont() ) {
        res += HRSchemaSerializer::makeEqualsPair(FONT + style->getId(),
            style->defaultFont().toString(), VISUAL_BLOCK_TABS_NUM);
    }
    return res;
}

static QString actorVisualData(WorkflowProcessItem * procItem) {
    assert(procItem != NULL);
    QString res;
    QString descStr = procItem->getProcess()->getDescription()->toHtml().replace(HRSchemaSerializer::NEW_LINE, " ");
    res += HRSchemaSerializer::makeEqualsPair(DESCRIPTION_ATTR, descStr, VISUAL_BLOCK_TABS_NUM);
    res += HRSchemaSerializer::makeEqualsPair(TOOLTIP_ATTR, procItem->toolTip().replace(HRSchemaSerializer::NEW_LINE, " "), 
                                                VISUAL_BLOCK_TABS_NUM);
    res += HRSchemaSerializer::makeEqualsPair(POSITION_ATTR, point2String(procItem->pos()), VISUAL_BLOCK_TABS_NUM);
    res += HRSchemaSerializer::makeEqualsPair(STYLE_ATTR, procItem->getStyle(), VISUAL_BLOCK_TABS_NUM);
    res += styleData(procItem->getStyleById(ItemStyles::SIMPLE));
    res += styleData(procItem->getStyleById(ItemStyles::EXTENDED));
    res += extendedStyleData(qobject_cast<ExtendedProcStyle*>(procItem->getStyleByIdSafe(ItemStyles::EXTENDED)));
    foreach(WorkflowPortItem * pit, procItem->getPortItems()) {
        res += HRSchemaSerializer::makeEqualsPair(pit->getPort()->getId() + HRSchemaSerializer::DOT + 
            PORT_ANGLE, QString::number(pit->getOrientarion()), VISUAL_BLOCK_TABS_NUM );
    }
    return res;
}

static QString linkVisualData(WorkflowBusItem * link) {
    return HRSchemaSerializer::makeEqualsPair(TEXT_POS_ATTR, point2String(link->getText()->pos()), 4);
}

static QString visualData(const QList<QGraphicsItem*> & items, const HRSchemaSerializer::NamesMap& nmap) {
//    assert(!items.isEmpty());
    QString res;
    
    foreach( QGraphicsItem * item, items ) {
        switch(item->type()) {
        case WorkflowProcessItemType:
        {
            WorkflowProcessItem * procItem = qgraphicsitem_cast<WorkflowProcessItem*>(item);
            res += HRSchemaSerializer::makeBlock(nmap[procItem->getProcess()->getId()], 
                                                    HRSchemaSerializer::NO_NAME, actorVisualData(procItem), 3);
            break;
        }
        case WorkflowBusItemType:
        {
            WorkflowBusItem * busItem = static_cast<WorkflowBusItem*>(item);
            Port * src = busItem->getBus()->source();
            Port * dst = busItem->getBus()->destination();
            res += HRSchemaSerializer::makeBlock(HRSchemaSerializer::makeArrowPair(nmap[src->owner()->getId()] + HRSchemaSerializer::DOT + 
                src->getId(), nmap[dst->owner()->getId()] + HRSchemaSerializer::DOT + dst->getId(), 0), 
                HRSchemaSerializer::NO_NAME, linkVisualData(busItem), 3);
            break;
        }
        }
    }
    return res;
}

static QString metaData(WorkflowScene * scene, const HRSchemaSerializer::NamesMap& nmap) {
    assert(scene != NULL);
    QString res;
    Schema schema = scene->getSchema();
    if(schema.hasParamAliases()) {
        res += HRSchemaSerializer::makeBlock(HRSchemaSerializer::PARAM_ALIASES_START, HRSchemaSerializer::NO_NAME, 
                                             HRSchemaSerializer::schemaParameterAliases(schema.getProcesses(), nmap), 2);
    }
    if(schema.hasPortAliases()) {
        res += HRSchemaSerializer::makeBlock(HRSchemaSerializer::PORT_ALIASES_START, HRSchemaSerializer::NO_NAME, 
            HRSchemaSerializer::schemaPortAliases(nmap, scene->getPortAliases()), 2);
    }
    res += HRSchemaSerializer::makeBlock(HRSchemaSerializer::VISUAL_START, HRSchemaSerializer::NO_NAME, visualData(scene->items(), nmap), 2);
    return res;
}

static QString bodyItself(WorkflowScene * scene) {
    assert(scene);
    QString res;
    Schema schema = scene->getSchema();
    HRSchemaSerializer::NamesMap nmap = HRSchemaSerializer::generateElementNames(schema.getProcesses());
    res += HRSchemaSerializer::elementsDefinition(schema.getProcesses(), nmap);
    res += HRSchemaSerializer::markersDefinition(schema.getProcesses(), nmap);
    res += HRSchemaSerializer::actorBindings(schema.getActorBindingsGraph(), nmap);
    res += HRSchemaSerializer::dataflowDefinition(schema.getProcesses(), nmap);
    res += HRSchemaSerializer::iterationsDefinition(schema.getIterations(), nmap);
    res += HRSchemaSerializer::makeBlock(HRSchemaSerializer::META_START, scene->getTypeName(), metaData(scene, nmap));
    return res;
}

QString HRSceneSerializer::scene2String(WorkflowScene * scene, const Metadata & meta) {
    assert(scene != NULL);
    QString res;
    Schema schema = scene->getSchema();
    HRSchemaSerializer::addPart(res, HRSchemaSerializer::header2String(&meta));
    HRSchemaSerializer::addPart(res, HRSchemaSerializer::includesDefinition(schema.getProcesses()));
    HRSchemaSerializer::addPart(res, HRSchemaSerializer::makeBlock(HRSchemaSerializer::BODY_START, meta.name, bodyItself(scene), 0, true));
    return res;
}

static QString itemsMeta(const QList<Actor*> & procs, const QList<QGraphicsItem*> & items, const HRSchemaSerializer::NamesMap& nmap) {
    QString res;
    bool hasParameterAliases = false;
    foreach(Actor * a, procs) {
        if(a->hasParamAliases()) {
            hasParameterAliases = true; break;
        }
    }
    if(hasParameterAliases) {
        res += HRSchemaSerializer::makeBlock(HRSchemaSerializer::PARAM_ALIASES_START, HRSchemaSerializer::NO_NAME,
                                                HRSchemaSerializer::schemaParameterAliases(procs, nmap), 2);
    }

    return res + HRSchemaSerializer::makeBlock(HRSchemaSerializer::VISUAL_START, HRSchemaSerializer::NO_NAME, visualData(items, nmap));
}

static QString itemsItself(const QList<QGraphicsItem*> & items, const QList<Iteration> & iterations) {
    QList<Actor*> procs;
    foreach(QGraphicsItem * it, items) {
        if(it->type() == WorkflowProcessItemType) {
            procs << qgraphicsitem_cast<WorkflowProcessItem*>(it)->getProcess();
        }
    }
    
    QString res;
    HRSchemaSerializer::NamesMap nmap = HRSchemaSerializer::generateElementNames(procs);
    res += HRSchemaSerializer::elementsDefinition(procs, nmap);
    res += HRSchemaSerializer::markersDefinition(procs, nmap);
    res += HRSchemaSerializer::dataflowDefinition(procs, nmap);
    res += HRSchemaSerializer::iterationsDefinition(iterations, nmap, false);
    res += HRSchemaSerializer::makeBlock(HRSchemaSerializer::META_START, HRSchemaSerializer::NO_NAME, itemsMeta(procs, items, nmap));
    return res;
}

QString HRSceneSerializer::items2String(const QList<QGraphicsItem*> & items, const QList<Iteration> & iterations) {
    assert(!items.isEmpty());
    QString res;
    HRSchemaSerializer::addPart(res, HRSchemaSerializer::header2String(NULL));
    HRSchemaSerializer::addPart(res, HRSchemaSerializer::makeBlock(HRSchemaSerializer::BODY_START, HRSchemaSerializer::NO_NAME,
                                                                    itemsItself(items, iterations), 0, true));
    return res;
}

/***************************************
 * Reading support
 ***************************************/
struct WorkflowSceneReaderData {
    WorkflowSceneReaderData(const QString & bytes, WorkflowScene * s, Metadata * m, bool se, bool ni) 
    : scene(s), meta(m), select(se), pasteMode(ni), graph(NULL) {
        tokenizer.tokenize(bytes); 
    }
    
    HRSchemaSerializer::Tokenizer tokenizer;
    WorkflowScene * scene;
    Metadata * meta;
    bool select;
    // (actor name in file, created actor)
    QMap<QString, Actor*> actorMap;
    // (actor name in file, created proc item)
    QMap<QString, WorkflowProcessItem*> procMap;
    QList<Iteration> iterations;
    QList<QPair<Port*, Port*> > dataflowLinks;
    bool pasteMode;
    ActorBindingsGraph *graph;
    QList<PortAlias> portAliases;
    
    struct LinkData {
        LinkData(WorkflowPortItem * s, WorkflowPortItem * d) : src(s), dst(d) {}
        bool operator ==(const LinkData & data) const {return src == data.src && dst == data.dst;}
        WorkflowPortItem * src;
        WorkflowPortItem * dst;
        HRSchemaSerializer::ParsedPairs pairs;
    };
    QList<LinkData> links;
}; // WorkflowSceneReaderData

static void parseActorDefinition(WorkflowSceneReaderData & data, const QString & actorName) {
    Actor * proc = HRSchemaSerializer::parseElementsDefinition(data.tokenizer, actorName, data.actorMap);
    WorkflowProcessItem * procIt = new WorkflowProcessItem(proc);
    data.scene->addItem(procIt);
    data.procMap[actorName] = procIt;
    if(data.select) {
        procIt->setSelected(true);
    }
}

static QPointF string2Point( const QString & str ) {
    QStringList list = str.split(QRegExp("\\s"), QString::SkipEmptyParts);
    if(list.size() != 2) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot parse coordinates from '%1'").arg(str));
    }
    bool ok = false;
    qreal x = list.at(0).toDouble(&ok);
    if(!ok) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot parse real value from '%1'").arg(list.at(0)));
    }
    ok = false;
    qreal y = list.at(1).toDouble(&ok);
    if(!ok) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot parse real value from '%1'").arg(list.at(1)));
    }
    return QPointF(x, y);
}

static QColor string2Color(const QString & str) {
    int r = 0, g = 0, b = 0, a = 0;
    QTextStream stream(str.toAscii());
    stream >> r >> g >> b >> a;
    if(stream.status() != QTextStream::Ok) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot parse 4 integer numbers from '%1'").arg(str));
    }
    return QColor(r, g, b, a);
}

static void parseStyleData(HRSchemaSerializer::ParsedPairs & pairs, ItemViewStyle * style) {
    if( style == NULL ) {
        return;
    }
    
    QString bgColor = pairs.equalPairs.take(BG_COLOR + style->getId());
    if(!bgColor.isEmpty()) {
        style->setBgColor(string2Color(bgColor));
    }
    QString font = pairs.equalPairs.take(FONT + style->getId());
    if(!font.isEmpty()) {
        QFont f;
        if(!f.fromString(font)) {
            throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot parse font from '%1'").arg(font));
        }
        style->setDefaultFont(f);
    }
}

static QRectF string2Rect(const QString & str) {
    QStringList list = str.split(QRegExp("\\s"));
    if( list.size() != 4 ) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot parse rectangle from '%1'").arg(str));
    }
    return QRectF(string2Point(list.at(0) + " " + list.at(1)), string2Point(list.at(2) + " " + list.at(3)));
}

static void parseExtendedStyleData(HRSchemaSerializer::ParsedPairs & pairs, ExtendedProcStyle * style) {
    if(style == NULL) {
        return;
    }
    QString bounds = pairs.equalPairs.take(BOUNDS);
    if(!bounds.isEmpty()) {
        style->setFixedBounds(string2Rect(bounds));
    }
}

static void parseVisualActorParams(WorkflowSceneReaderData & data, const QString & actorName) {
    if(!data.actorMap.contains(actorName)) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("%1 element undefined in visual block").arg(actorName));
    }
    
    HRSchemaSerializer::ParsedPairs pairs(data.tokenizer);
    if(!pairs.blockPairs.isEmpty()) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("No other blocks allowed in visual block '%1'").arg(actorName));
    }
    WorkflowProcessItem * procItem = data.procMap.value(actorName);
    assert(procItem != NULL);
    
    // TODO: support custom descriptions
    QString description = pairs.equalPairs.take(DESCRIPTION_ATTR); Q_UNUSED(description);
    /*if( !description.isEmpty() ) {
        
    }*/
    QString tooltip = pairs.equalPairs.take(TOOLTIP_ATTR);
    if(!tooltip.isEmpty()) {
        procItem->setToolTip(tooltip);
    }
    QString position = pairs.equalPairs.take(POSITION_ATTR);
    if( !position.isEmpty() ) {
        procItem->setPos(string2Point(position));
    }
    QString style = pairs.equalPairs.take(STYLE_ATTR);
    if(!style.isEmpty()) {
        if( !procItem->containsStyle(style) ) {
            coreLog.details(HRSceneSerializer::tr("%1 does not has %2 style").arg(actorName).arg(style));
        } else {
            procItem->setStyle(style);
        }
    }
    parseStyleData(pairs, procItem->getStyleById(ItemStyles::SIMPLE));
    parseStyleData(pairs, procItem->getStyleById(ItemStyles::EXTENDED));
    parseExtendedStyleData(pairs, qobject_cast<ExtendedProcStyle*>(procItem->getStyleById(ItemStyles::EXTENDED)));
    
    foreach( const QString & key, pairs.equalPairs.keys() ) {
        QStringList list = key.split(HRSchemaSerializer::DOT);
        if(list.size() == 2 && list.at(1) == PORT_ANGLE) {
            QString portId = list.at(0);
            WorkflowPortItem * portItem = procItem->getPort(portId);
            if(portItem == NULL) {
                throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot find port '%1' at %2 element").arg(portId).arg(actorName));
            }
            bool ok = false;
            QString strVal = pairs.equalPairs.value(key);
            qreal orientation = strVal.toDouble(&ok);
            if(!ok) {
                throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot parse real number from: '%1'").arg(strVal));
            }
            portItem->setOrientation(orientation);
        } else {
            coreLog.details(HRSceneSerializer::tr("Undefined visual key: '%1' for actor %2").arg(key).arg(actorName));
        }
    }
}

static void parseLinkVisualBlock(WorkflowSceneReaderData & data, const QString & from, const QString & to) {
    bool hasBlock = data.tokenizer.look() == HRSchemaSerializer::BLOCK_START;
    QString srcActorName = HRSchemaSerializer::parseAt(from, 0);
    Actor * srcActor = data.actorMap.value(srcActorName);
    if(srcActor == NULL) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Undefined element id: '%1'").arg(srcActorName));
    }
    QString srcPortId = HRSchemaSerializer::parseAt(from, 1);
    Port * srcPort = srcActor->getPort(srcPortId);
    if(srcPort == NULL) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot find '%1' port at '%2'").arg(srcPortId).arg(srcActorName));
    }
    
    QString dstActorName = HRSchemaSerializer::parseAt(to, 0);
    Actor * dstActor = data.actorMap.value(dstActorName);
    if(dstActor == NULL) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Undefined element id: '%1'").arg(dstActorName));
    }
    QString dstPortId = HRSchemaSerializer::parseAt(to, 1);
    Port * dstPort = dstActor->getPort(dstPortId);
    if(dstPort == NULL) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot find '%1' port at '%2'").arg(dstPortId).arg(dstActorName));
    }
    
    WorkflowSceneReaderData::LinkData linkData(data.procMap[srcActorName]->getPort(srcPortId), data.procMap[dstActorName]->getPort(dstPortId));
    if(hasBlock) {
        data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_START);
        HRSchemaSerializer::ParsedPairs pairs(data.tokenizer);
        data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        
        if( !pairs.blockPairs.isEmpty() ) {
            throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("No other blocks allowed in link parameters block '%1'").
                arg(HRSchemaSerializer::makeArrowPair(from, to, 0)));
        }
        linkData.pairs = pairs;
    }
    if (NULL == data.graph) {
        data.links << linkData;
    } else {
        if (!data.graph->contains(srcPort, dstPort)) {
            throw HRSchemaSerializer::ReadFailed(HRSchemaSerializer::tr("Undefined data-flow link: '%1'. Define it in actor-bindings").arg(from + HRSchemaSerializer::DATAFLOW_SIGN + to));
        }
        int pos = data.links.indexOf(linkData);
        assert(pos >= 0);
        data.links.replace(pos, linkData);
    }
}

static void parseVisual(WorkflowSceneReaderData & data) {
    while(data.tokenizer.look() != HRSchemaSerializer::BLOCK_END) {
        QString tok = data.tokenizer.take();
        QString next = data.tokenizer.take();
        if(next == HRSchemaSerializer::BLOCK_START) {
            QString actorName = str2aid(tok);
            parseVisualActorParams(data, actorName);
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        }
        else if(next == HRSchemaSerializer::DATAFLOW_SIGN) {
            QString to = data.tokenizer.take();
            parseLinkVisualBlock(data, tok, to);
        }
    }
}

static void parseMeta(WorkflowSceneReaderData & data) {
    QString tok = data.tokenizer.look();
    if (HRSchemaSerializer::BLOCK_START != tok) {
        data.scene->setTypeName(tok);
        data.tokenizer.take();
    }

    data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_START);
    while (data.tokenizer.look() != HRSchemaSerializer::BLOCK_END) {
        QString tok = data.tokenizer.take();
        if (HRSchemaSerializer::PARAM_ALIASES_START == tok) {
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_START);
            HRSchemaSerializer::parseParameterAliases(data.tokenizer, data.actorMap);
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        } else if (HRSchemaSerializer::PORT_ALIASES_START == tok) {
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_START);
            HRSchemaSerializer::parsePortAliases(data.tokenizer, data.actorMap, data.portAliases);
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        } else if (HRSchemaSerializer::VISUAL_START == tok) {
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_START);
            parseVisual(data);
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        } else if (HRSchemaSerializer::OLD_ALIASES_START == tok) {
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_START);
            HRSchemaSerializer::parseOldAliases(data.tokenizer, data.actorMap);
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        } else if (HRSchemaSerializer::ALIASES_HELP_START == tok) {
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_START);
            HRSchemaSerializer::parseAliasesHelp(data.tokenizer, data.actorMap.values());
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        } else {
            throw HRSchemaSerializer::ReadFailed(HRSchemaSerializer::UNDEFINED_META_BLOCK.arg(tok));
        }
    }
}

static void parseActorBindings(WorkflowSceneReaderData &data) {
    ActorBindingsGraph graph;

    if (!data.links.isEmpty()) {
        throw HRSchemaSerializer::ReadFailed(HRSchemaSerializer::tr("Links list is not empty. Maybe .meta is defined earlier than actor-bindings"));
    }

    while (data.tokenizer.look() != HRSchemaSerializer::BLOCK_END) {
        QString from = data.tokenizer.take();
        QString srcActorName = HRSchemaSerializer::parseAt(from, 0);
        Actor * srcActor = data.actorMap.value(srcActorName);
        if(srcActor == NULL) {
            throw HRSchemaSerializer::ReadFailed(HRSchemaSerializer::tr("Undefined element id: '%1'").arg(srcActorName));
        }
        QString srcPortId = HRSchemaSerializer::parseAt(from, 1);
        Port * srcPort = srcActor->getPort(srcPortId);
        if(srcPort == NULL) {
            throw HRSchemaSerializer::ReadFailed(HRSchemaSerializer::tr("Cannot find '%1' port at '%2'").arg(srcPortId).arg(srcActorName));
        }

        data.tokenizer.assertToken(HRSchemaSerializer::DATAFLOW_SIGN);
        QString to = data.tokenizer.take();
        QString dstActorName = HRSchemaSerializer::parseAt(to, 0);
        Actor * dstActor = data.actorMap.value(dstActorName);
        if(dstActor == NULL) {
            throw HRSchemaSerializer::ReadFailed(HRSchemaSerializer::tr("Undefined element id: '%1'").arg(dstActorName));
        }
        QString dstPortId = HRSchemaSerializer::parseAt(to, 1);
        Port * dstPort = dstActor->getPort(dstPortId);
        if(dstPort == NULL) {
            throw HRSchemaSerializer::ReadFailed(HRSchemaSerializer::tr("Cannot find '%1' port at '%2'").arg(dstPortId).arg(dstActorName));
        }

        WorkflowSceneReaderData::LinkData linkData(data.procMap[srcActorName]->getPort(srcPortId), data.procMap[dstActorName]->getPort(dstPortId));
        data.links << linkData;

        if (!graph.addBinding(srcPort, dstPort)) {
            throw HRSchemaSerializer::ReadFailed(HRSchemaSerializer::tr("Duplicate binding at '%1'").arg(from + HRSchemaSerializer::DATAFLOW_SIGN + to));
        }
    }

    QString message;
    if (!graph.validateGraph(message)) {
        throw HRSchemaSerializer::ReadFailed(HRSchemaSerializer::tr("Validating actor bindings graph failed: '%1'").arg(message));
    }
    data.graph = new ActorBindingsGraph(graph);
}

static void parseBodyItself(WorkflowSceneReaderData & data) {
    HRSchemaSerializer::Tokenizer & tokenizer = data.tokenizer;
    while(tokenizer.notEmpty() && tokenizer.look() != HRSchemaSerializer::BLOCK_END) {
        QString tok = tokenizer.take();
        QString nextTok = tokenizer.look();
        if(tok == HRSchemaSerializer::META_START) {
            parseMeta(data);
            tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        } else if(tok == HRSchemaSerializer::DOT_ITERATION_START) {
            QString itName = tokenizer.look() == HRSchemaSerializer::BLOCK_START ? "" : tokenizer.take();
            tokenizer.assertToken(HRSchemaSerializer::BLOCK_START);
            data.iterations << HRSchemaSerializer::parseIteration(tokenizer, itName, data.actorMap, data.pasteMode);
            tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        } else if (tok == HRSchemaSerializer::ACTOR_BINDINGS) {
            if (NULL != data.graph) {
                throw HRSchemaSerializer::ReadFailed("Double defining of actor bindings");
            }
            tokenizer.assertToken(HRSchemaSerializer::BLOCK_START);
            parseActorBindings(data);
            tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        } else if (tok == HRSchemaSerializer::FUNCTION_START) {
            HRSchemaSerializer::parseFunctionDefinition(tokenizer, data.actorMap);
            tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        } else if(nextTok == HRSchemaSerializer::DATAFLOW_SIGN) {
            data.dataflowLinks << HRSchemaSerializer::parseDataflow(tokenizer, tok, data.actorMap);
        } else if(nextTok == HRSchemaSerializer::BLOCK_START) {
            tokenizer.take();
            parseActorDefinition(data, tok);
            tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
        } else {
            throw HRSchemaSerializer::ReadFailed(HRSchemaSerializer::UNDEFINED_CONSTRUCT.arg(tok).arg(nextTok));
        }
    }

    foreach (Actor *proc, data.actorMap.values()) {
        ActorPrototype *proto = proc->getProto();
        if (NULL != proto->getEditor()) {
            ActorConfigurationEditor *actorEd = dynamic_cast<ActorConfigurationEditor*>(proto->getEditor());
            if (NULL != actorEd) {
                ActorConfigurationEditor *editor = dynamic_cast<ActorConfigurationEditor*>(proto->getEditor()->clone());
                editor->setConfiguration(proc);
                proc->setEditor(editor);
            }
        }
    }
}

void setFlowParameters(const HRSchemaSerializer::ParsedPairs & p, WorkflowBusItem * bit) {
    assert(bit != NULL);
    HRSchemaSerializer::ParsedPairs pairs = p;
    QString textPos = pairs.equalPairs.take(TEXT_POS_ATTR);
    if(!textPos.isEmpty()) {
        bit->getText()->setPos(string2Point(textPos));
    }
    
    foreach(const QString & key, pairs.equalPairs.keys()) {
        coreLog.details(HRSceneSerializer::tr("Key '%1' not recognized as flow parameter").arg(key));
    }
}

static void tryToConnect(WorkflowPortItem * input, WorkflowPortItem * output, const HRSchemaSerializer::ParsedPairs & pairs, bool select) {
    Port * srcPort = input->getPort();
    Port * destPort = output->getPort();
    if( !input || !output || !input->tryBind(output) ) {
        throw HRSchemaSerializer::ReadFailed(HRSceneSerializer::tr("Cannot bind %1:%2 to %3:%4").
            arg(srcPort->owner()->getId()).arg(srcPort->getId()).arg(destPort->owner()->getId()).arg(destPort->getId()));
    }
    WorkflowBusItem * bit = input->getDataFlow(output);
    if(select) {
        bit->setSelected(true);
    }
    setFlowParameters(pairs, bit);
}

static void setFlows(WorkflowSceneReaderData & data) {
    if(!data.links.isEmpty()) {
        // try to create connections from visual links description
        foreach(const WorkflowSceneReaderData::LinkData & link, data.links) {
            tryToConnect(link.src, link.dst, link.pairs, data.select);
        }
    } else {
        // try to create connections from FlowGraph (TODO: do not works for all schemas: see sitecon search schema in samples)
        HRSchemaSerializer::FlowGraph graph(data.dataflowLinks);
        ActorBindingsGraph bindingsGraph;
        graph.minimize();
        foreach(Port * srcPort, graph.graph.keys()) {
            foreach(Port * destPort, graph.graph.value(srcPort)) {
                WorkflowPortItem * input = data.procMap[data.actorMap.key(srcPort->owner())]->getPort(srcPort->getId());
                WorkflowPortItem * output = data.procMap[data.actorMap.key(destPort->owner())]->getPort(destPort->getId());
                HRSchemaSerializer::ParsedPairs pairs;
                int ind = data.links.indexOf(WorkflowSceneReaderData::LinkData(input, output));
                if(ind != -1) {
                    pairs = data.links[ind].pairs;
                }
                tryToConnect(input, output, pairs, data.select);
                bindingsGraph.addBinding(input->getPort(), output->getPort());
            }
        }
    }
}

ActorId HRSceneSerializer::newActorId(const QString &id, const QList<Actor*> & procs) {
    QString result = id;
    int number = 0;
    bool found = false;
    foreach(Actor * actor, procs) {
        if(actor->getId() == id) {
            found = true;
            number = qMax(number, 1);
        } else {
            int idx = actor->getId().lastIndexOf("-");
            if (-1 != idx) {
                QString left = actor->getId().left(idx);
                if (id == left) {
                    QString right = actor->getId().mid(idx + 1);
                    bool ok = false;
                    int num = right.toInt(&ok);
                    if(ok) {
                        found = true;
                        number = qMax(number, num + 1);
                    }
                }
            }
        }
    }

    if (found) {
        result += QString("-%1").arg(number);
    }
    return str2aid(result);
}

QString HRSceneSerializer::string2Scene(const QString & bytes, WorkflowScene * scene, Metadata * meta, bool select, bool pasteMode, QList<QString> includedUrls) {
    try{
        WorkflowSceneReaderData data(bytes, scene, meta, select, pasteMode);
        HRSchemaSerializer::parseHeader(data.tokenizer, data.meta);
        data.tokenizer.removeCommentTokens();

        QString tok = data.tokenizer.look();
        while (HRSchemaSerializer::INCLUDE == tok) {
            HRSchemaSerializer::parseIncludes(data.tokenizer, includedUrls);
            tok = data.tokenizer.look();
        }
        HRSchemaSerializer::parseBodyHeader(data.tokenizer, data.meta, !pasteMode);

        if(scene != NULL) {
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_START);
            parseBodyItself(data);
            data.tokenizer.assertToken(HRSchemaSerializer::BLOCK_END);
            if( data.iterations.isEmpty() ) {
                if(!pasteMode) {
                    scene->setIterated(false, Iteration("Default iteration"));
                }
            } else {
                scene->setIterated(true);
            }
            setFlows(data);
            HRSchemaSerializer::addEmptyValsToBindings(data.actorMap.values());

            QPointF pnt = scene->sceneRect().center();
            foreach(WorkflowProcessItem * wItem, data.procMap.values()) {
                if(wItem->pos() == QPointF(0,0)) {
                    wItem->setPos(pnt);
                    pnt += QPointF(100, 0);
                }
            }
            scene->setIterations(data.iterations);
            scene->setPortAliases(data.portAliases);
        }
    } catch(const HRSchemaSerializer::ReadFailed & ex) {
        return ex.what;
    } catch(...) {
        return HRSchemaSerializer::UNKNOWN_ERROR;
    }
    return HRSchemaSerializer::NO_ERROR;
}

} // U2
