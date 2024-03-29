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

#include "GSequenceGraphViewWithFactory.h"

#include <U2Core/U2SafePoints.h>

#include <U2Gui/GUIUtils.h>

#include <U2View/ADVConstants.h>
#include <U2View/ADVSequenceObjectContext.h>
#include <U2View/AnnotatedDNAView.h>


namespace U2 {

/**
 * Constructor of a graph view with factory
 */
GSequenceGraphViewWithFactory::GSequenceGraphViewWithFactory(
   ADVSingleSequenceWidget* sequenceWidget, GSequenceGraphFactory* _factory)
   : GSequenceGraphView(
      sequenceWidget,
      sequenceWidget->getSequenceContext(),
      sequenceWidget->getPanGSLView(),
     _factory->getGraphName()),
    factory(_factory)
{
}

/**
 * Adds an action to the graphs menu
 */
void GSequenceGraphViewWithFactory::addActionsToGraphMenu(QMenu* graphMenu)
{
    GSequenceGraphView::addActionsToGraphMenu(graphMenu);
}


} // namespace
