//
// C++ Interface: ExportGPX
//
// Description: 
//
//
// Author: cbro <cbro@semperpax.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef ExportGPX_H
#define ExportGPX_H

#include "IImportExport.h"

/**
	@author cbro <cbro@semperpax.com>
*/
class ExportGPX : public IImportExport
{
public:
    ExportGPX(Document* doc);

    ~ExportGPX();

	//export
	virtual bool export_(const QList<Feature *>& featList);
};

#endif
