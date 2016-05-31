/***************************************************************************
 *   Copyright (C) 2008 by Chris Browet                                    *
 *   cbro@semperpax.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef YAHOOTILEDMAPADAPTER_H
#define YAHOOTILEDMAPADAPTER_H

#include "IMapAdapterFactory.h"
#include "IMapAdapter.h"

#include <QLocale>

//! MapAdapter for WMS servers
/*!
 * Use this derived MapAdapter to display maps from WMS servers
 *	@author Kai Winter <kaiwinter@gmx.de>
*/
class YahooTiledMapAdapter : public IMapAdapter
{
    Q_OBJECT
    Q_INTERFACES(IMapAdapter)

public:
    //! constructor
    /*!
     * This construct a Yahoo Adapter
     */
    YahooTiledMapAdapter();
    virtual ~YahooTiledMapAdapter();

    //! returns the unique identifier (Uuid) of this MapAdapter
    /*!
     * @return  the unique identifier (Uuid) of this MapAdapter
     */
    virtual QUuid	getId		() const;

    //! returns the type of this MapAdapter
    /*!
     * @return  the type of this MapAdapter
     */
    virtual IMapAdapter::Type	getType		() const;

    //! returns the name of this MapAdapter
    /*!
     * @return  the name of this MapAdapter
     */
    virtual QString	getName		() const;

    //! returns the host of this MapAdapter
    /*!
     * @return  the host of this MapAdapter
     */
    virtual QString	getHost		() const;

    //! returns the size of the tiles
    /*!
     * @return the size of the tiles
     */
    virtual int		getTileSizeW	() const;
    virtual int		getTileSizeH	() const;

    //! returns the min zoom value
    /*!
     * @return the min zoom value
     */
    virtual int 		getMinZoom	(const QRectF &) const;

    //! returns the max zoom value
    /*!
     * @return the max zoom value
     */
    virtual int		getMaxZoom	(const QRectF &) const;

    //! returns the current zoom
    /*!
     * @return the current zoom
     */
    virtual int 		getZoom		() const;

    //! returns the source tag to be applied when drawing over this map
    /*!
     * @return the source tag
     */
    virtual QString	getSourceTag		() const { return "Yahoo"; }
    virtual void setSourceTag (const QString& ) {};

    //! returns the Url of the usage license
    /*!
     * @return the Url of the usage license
     */
    virtual QString	getLicenseUrl() const {return QString();}

    virtual int		getAdaptedZoom()   const;
    virtual int 	getAdaptedMinZoom	(const QRectF &) const;
    virtual int		getAdaptedMaxZoom	(const QRectF &) const;

    virtual void	zoom_in();
    virtual void	zoom_out();

    virtual bool	isValid(int x, int y, int z) const;
    virtual QString getQuery(int x, int y, int z) const;
    virtual QString getQuery(const QRectF& /* wgs84Bbox */, const QRectF& /* projBbox */, const QRect& /* size */) const  { return QString(); }
    virtual QPixmap getPixmap(const QRectF& /* wgs84Bbox */, const QRectF& /* projBbox */, const QRect& /* size */) const { return QPixmap(); }

    virtual QRectF	getBoundingbox() const;

    virtual bool isTiled() const { return true; }
    virtual int getTilesWE(int zoom) const;
    virtual int getTilesNS(int zoom) const;
    virtual QString projection() const;

    virtual QMenu* getMenu() const { return NULL; }

    virtual IImageManager* getImageManager();
    virtual void setImageManager(IImageManager* anImageManager);

    virtual void cleanup() {}

    virtual bool toXML(QXmlStreamWriter& /*stream*/) { return true; }
    virtual void fromXML(QXmlStreamReader& /*xParent*/) {}
    virtual QString toPropertiesHtml() {return QString();}

    virtual void setSettings(QSettings* /*aSet*/) {}

private:
    QLocale loc;
    IImageManager* theImageManager;

    QString	host;
    QString	serverPath;
    int	tilesize;
    int min_zoom;
    int max_zoom;
    int current_zoom;

    virtual QString getQ(QPointF ul, QPointF br) const;
};

class YahooTiledMapAdapterFactory : public QObject, public IMapAdapterFactory
{
    Q_OBJECT
    Q_INTERFACES(IMapAdapterFactory)
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    Q_PLUGIN_METADATA(IID "be.merkaartor.YahooTiledMapAdapter" FILE "yahootiledmapadapter.json")
#endif

public:
    //! Creates an instance of the actual plugin
    /*!
     * @return  a pointer to the MapAdapter
     */
    IMapAdapter* CreateInstance() {return new YahooTiledMapAdapter(); }

    //! returns the unique identifier (Uuid) of this MapAdapter
    /*!
     * @return  the unique identifier (Uuid) of this MapAdapter
     */
    virtual QUuid	getId		() const;

    //! returns the name of this MapAdapter
    /*!
     * @return  the name of this MapAdapter
     */
    virtual QString	getName		() const;
};

#endif
