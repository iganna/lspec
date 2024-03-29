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

#include <U2Core/GUrl.h>
#include <U2Core/U2OpStatus.h>

#ifndef _U2_BAM_UTILS_H_
#define _U2_BAM_UTILS_H_

namespace U2 {

class Document;

class U2FORMATS_EXPORT BAMUtils : public QObject {
public:
    /**
     * Returns the url to the output BAM file
     */
    static void convertSamToBam(const GUrl &samUrl, const GUrl &bamUrl, U2OpStatus &os);

    static bool isSortedBam(const GUrl &bamUrl, U2OpStatus &os);

    /**
     * @sortedBamBaseName is the result file path without extension.
     * Returns @sortedBamBaseName.bam
     */
    static GUrl sortBam(const GUrl &bamUrl, const QString &sortedBamBaseName, U2OpStatus &os);

    static bool hasValidBamIndex(const GUrl &bamUrl);

    static void createBamIndex(const GUrl &bamUrl, U2OpStatus &os);

    static void writeDocument(Document *doc, U2OpStatus &os);
};

} // U2

#endif // _U2_BAM_UTILS_H_
