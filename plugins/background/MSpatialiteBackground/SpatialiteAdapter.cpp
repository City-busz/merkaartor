//***************************************************************
// CLass: SpatialiteAdapter
//
// Description:
//
//
// Author: Chris Browet <cbro@semperpax.com> (C) 2010
//
// Copyright: See COPYING file that comes with this distribution
//
//******************************************************************

#include "SpatialiteAdapter.h"

#include <QCoreApplication>
#include <QtPlugin>
#include <QAction>
#include <QFileDialog>
#include <QPainter>
#include <QMessageBox>
#include <QTimer>
#include <QBuffer>
#include <QPair>
#include <QStringList>

#include <QDebug>

#include <math.h>
#include "spatialite/gaiageo.h"

#include "MasPaintStyle.h"

static const QUuid theUid ("{4509fe81-47d0-4487-b6d0-e8910f2f1f7d}");
static const QString theName("Spatialite");

QUuid SpatialiteAdapterFactory::getId() const
{
    return theUid;
}

QString	SpatialiteAdapterFactory::getName() const
{
    return theName;
}

/******/

#define FILTER_OPEN_SUPPORTED \
    tr("Supported formats")+" (*.sqlite *.spatialite)\n" \
    +tr("All Files (*)")

double angToRad(double a)
{
    return a*M_PI/180.;
}

QPointF mercatorProject(const QPointF& c)
{
    double x = angToRad(c.x()) / M_PI * 20037508.34;
    double y = log(tan(angToRad(c.y())) + 1/cos(angToRad(c.y()))) / M_PI * (20037508.34);

    return QPointF(x, y);
}

inline uint qHash(IFeature::FId id)
{
    uint h1 = qHash(id.type);
    uint h2 = qHash(id.numId);
    return ((h1 << 16) | (h1 >> 16)) ^ h2;
}

/*****/

SpatialiteAdapter::SpatialiteAdapter()
{
    QAction* loadFile = new QAction(tr("Load Spatialite db..."), this);
    loadFile->setData(theUid.toString());
    connect(loadFile, SIGNAL(triggered()), SLOT(onLoadFile()));
    theMenu = new QMenu();
    theMenu->addAction(loadFile);

    m_loaded = false;

    MasPaintStyle theStyle;
    theStyle.loadPainters(":/Styles/Mapnik.mas");
    for (int i=0; i<theStyle.painterSize(); ++i) {
        thePrimitivePainters.append(PrimitivePainter(*theStyle.getPainter(i)));
    }

    m_cache.setMaxCost(100000);
}


SpatialiteAdapter::~SpatialiteAdapter()
{
    if (m_loaded) {
        sqlite3_close(m_handle);
        spatialite_cleanup_ex(s_handle);
    }
}

void SpatialiteAdapter::onLoadFile()
{
    QString fileName = QFileDialog::getOpenFileName(
                    NULL,
                    tr("Open Spatialite db"),
                    "", FILTER_OPEN_SUPPORTED);
    if (fileName.isEmpty())
        return;

    setFile(fileName);
}

void SpatialiteAdapter::initTable(const QString& table)
{
    QString tag = table.mid(3);
    QString q = QString("select * from %1 where ROWID IN "
                        "(Select rowid from idx_%1_Geometry WHERE xmax > ? and ymax > ? and xmin < ? and ymin < ?);").arg(table);
    int ret = sqlite3_prepare_v2(m_handle, q.toUtf8().data(), q.size(), &m_stmtHandles[table], NULL);
    if (ret != SQLITE_OK) {
        qDebug() << "Sqlite: prepare error: " << ret;
        return;
    }
    if (getType() == IMapAdapter::DirectBackground) {
        q = QString("select distinct sub_type from %1").arg(table);
        sqlite3_stmt *pStmt;
        ret = sqlite3_prepare_v2(m_handle, q.toUtf8().data(), q.size(), &pStmt, NULL);
        if (ret != SQLITE_OK) {
            qDebug() << "Sqlite: prepare error: " << ret;
        }
        while (sqlite3_step(pStmt) == SQLITE_ROW) {
            QString sub_type((const char*)sqlite3_column_text(pStmt, 0));
            PrimitiveFeature f;
            f.Tags.append(qMakePair(tag, sub_type));
            for(int i=0; i< thePrimitivePainters.size(); ++i) {
                if (thePrimitivePainters[i].matchesTag(&f, 0)) {
                    myStyles.insert(QString("%1%2").arg(tag).arg(sub_type), &thePrimitivePainters[i]);
                    break;
                }
            }
        }
        sqlite3_finalize(pStmt);
    }
}

void SpatialiteAdapter::setFile(const QString& fn)
{
    if (m_loaded) {
        sqlite3_close(m_handle);
        spatialite_cleanup_ex(s_handle);
    }
    m_loaded = false;

    s_handle = spatialite_alloc_connection();
    int ret = sqlite3_open_v2 (fn.toUtf8().data(), &m_handle, SQLITE_OPEN_READONLY, NULL);
    if (ret != SQLITE_OK)
    {
        QMessageBox::critical(0,QCoreApplication::translate("SpatialiteBackground","No valid file"),QCoreApplication::translate("SpatialiteBackground","Cannot open db."));
        sqlite3_close (m_handle);
        return;
    }
    spatialite_init_ex(m_handle, s_handle, 0);
    QString q = QString("SELECT f_table_name FROM geometry_columns;");
    sqlite3_stmt *pStmt;
    ret = sqlite3_prepare_v2(m_handle, q.toUtf8().data(), q.size(), &pStmt, NULL);
    if (ret != SQLITE_OK) {
        qDebug() << "Sqlite: prepare error: " << ret;
    }
    while (sqlite3_step(pStmt) == SQLITE_ROW) {
        QString table((const char*)sqlite3_column_text(pStmt, 0));
        m_tables << table;
    }
    sqlite3_finalize(pStmt);

    if (!m_tables.size()) {
        QMessageBox::critical(0,QCoreApplication::translate("SpatialiteBackground","No valid file"),QCoreApplication::translate("SpatialiteBackground","geometry_columns table absent or invalid"));
        sqlite3_close (m_handle);
        spatialite_cleanup_ex(s_handle);
        return;
    }
    m_dbName = fn;
    m_loaded = true;

    foreach (const QString &s, m_tables)
        initTable(s);

    emit (forceRefresh());
}

QString	SpatialiteAdapter::getHost() const
{
    return QString();
}

QUuid SpatialiteAdapter::getId() const
{
    return QUuid(theUid);
}

QString	SpatialiteAdapter::getName() const
{
    return theName;
}

QMenu* SpatialiteAdapter::getMenu() const
{
    return theMenu;
}

QRectF SpatialiteAdapter::getBoundingbox() const
{
    return QRectF(QPointF(-180.00, -90.00), QPointF(180.00, 90.00));
}

QString SpatialiteAdapter::projection() const
{
    return "EPSG:4326";
}

const QList<IFeature*>* SpatialiteAdapter::getPaths(const QRectF& wgs84Bbox, const IProjection* projection) const
{
    if (!m_loaded)
        return NULL;

    theFeatures.clear();
    foreach (QString s, m_tables)
        buildFeatures(s, wgs84Bbox, projection);

    return &theFeatures;
}

QPixmap SpatialiteAdapter::getPixmap(const QRectF& wgs84Bbox, const QRectF& /*projBbox*/, const QRect& src) const
{
    if (!m_loaded)
        return QPixmap();

    QPixmap pix(src.size());
    pix.fill(Qt::transparent);
    QPainter P(&pix);
    P.setRenderHint(QPainter::Antialiasing);

    double ScaleLon = src.width() / wgs84Bbox.width();
    double ScaleLat = src.height() / wgs84Bbox.height();

    double PLon = wgs84Bbox.center().x() * ScaleLon;
    double PLat = wgs84Bbox.center().y() * ScaleLat;
    double DeltaLon = src.width() / 2 - PLon;
    double DeltaLat = src.height() - (src.height() / 2 - PLat);

    double LengthOfOneDegreeLat = 6378137.0 * M_PI / 180;
    double LengthOfOneDegreeLon = LengthOfOneDegreeLat * fabs(cos(angToRad(wgs84Bbox.center().y())));
    double lonAnglePerM =  1 / LengthOfOneDegreeLon;
    m_PixelPerM = src.width() / (double)wgs84Bbox.width() * lonAnglePerM;
    qDebug() << "ppm: " << m_PixelPerM;

    m_transform.setMatrix(ScaleLon, 0, 0, 0, -ScaleLat, 0, DeltaLon, DeltaLat, 1);

    theFeatures.clear();
    foreach (QString s, m_tables)
        buildFeatures(s, wgs84Bbox);

    foreach(IFeature* iF, theFeatures) {
        PrimitiveFeature* f = static_cast<PrimitiveFeature*>(iF);
        if (f->theId.type & IFeature::Point)
            continue;

        PrimitivePainter* fpainter = myStyles[QString("%1%2").arg(f->Tags[0].first).arg(f->Tags[0].second)];
        if (fpainter->matchesZoom(m_PixelPerM)) {
            QPainterPath pp = m_transform.map(f->thePath);
                fpainter->drawBackground(&pp, &P, m_PixelPerM);
        }
    }
    foreach(IFeature* iF, theFeatures) {
        PrimitiveFeature* f = static_cast<PrimitiveFeature*>(iF);
        if (f->theId.type & IFeature::Point)
            continue;

        PrimitivePainter* fpainter = myStyles[QString("%1%2").arg(f->Tags[0].first).arg(f->Tags[0].second)];
        if (fpainter->matchesZoom(m_PixelPerM)) {
            QPainterPath pp = m_transform.map(f->thePath);
            if (!(f->theId.type & IFeature::Point))
                fpainter->drawForeground(&pp,& P, m_PixelPerM);
        }
    }
    foreach(IFeature* iF, theFeatures) {
        PrimitiveFeature* f = static_cast<PrimitiveFeature*>(iF);
        PrimitivePainter* fpainter = myStyles[QString("%1%2").arg(f->Tags[0].first).arg(f->Tags[0].second)];
        if (fpainter->matchesZoom(m_PixelPerM)) {
            if (f->theId.type & IFeature::Point) {
                QPointF pt = m_transform.map((QPointF)f->thePath.elementAt(0));
                fpainter->drawTouchup(&pt, &P, m_PixelPerM);
            } else {
                QPainterPath pp = m_transform.map(f->thePath);
                fpainter->drawTouchup(&pp, &P, m_PixelPerM);
            }
        }
    }
    P.end();

    foreach(IFeature* iF, theFeatures) {
        PrimitiveFeature* f = static_cast<PrimitiveFeature*>(iF);
        m_cache.insert(f->theId, f);
    }

    return pix;
}

void SpatialiteAdapter::buildFeatures(const QString& table, const QRectF& selbox, const IProjection* proj) const
{
    QString tag = table.mid(3);
//    QPainterPath clipPath;
//    clipPath.addRect(selbox/*.adjusted(-1000, -1000, 1000, 1000)*/);
    double x, y;

    sqlite3_stmt* pStmt = m_stmtHandles[table];
    if (!pStmt)
        return;

    sqlite3_bind_double(pStmt, 1, selbox.bottomLeft().x());
    sqlite3_bind_double(pStmt, 2, selbox.topRight().y());
    sqlite3_bind_double(pStmt, 3, selbox.topRight().x());
    sqlite3_bind_double(pStmt, 4, selbox.bottomLeft().y());

    while (sqlite3_step(pStmt) == SQLITE_ROW) {
        qint64 id = sqlite3_column_int64(pStmt, 0);
        if (m_cache.contains(IFeature::FId(IFeature::LineString, id))) {
            IFeature* f = m_cache.take(IFeature::FId(IFeature::LineString, id));
            theFeatures << f;
            continue;
        }
        QString sub_type((const char*)sqlite3_column_text(pStmt, 1));
//        if (!myStyles.contains(QString("%1%2").arg(tag).arg(sub_type)))
//            continue;

        QString name((const char*)sqlite3_column_text(pStmt, 2));
        int blobSize = sqlite3_column_bytes(pStmt, 3);
        QByteArray ba((const char*)sqlite3_column_blob(pStmt, 3), blobSize);
        const unsigned char* blob = (const unsigned char*)ba.constData();

        gaiaGeomCollPtr coll = gaiaFromSpatiaLiteBlobWkb(blob, blobSize);
//        Q_ASSERT(coll);
        if (!coll)
            continue;

        gaiaPointPtr node = coll->FirstPoint;
        while (node) {
            PrimitiveFeature* f = new PrimitiveFeature();
            f->theId = IFeature::FId(IFeature::Point, id);
            f->Tags.append(qMakePair(QString("name"), name));
            QPointF pt(node->X, node->Y);
            if (proj)
                pt = proj->project(pt);
            f->thePath.moveTo(pt);
            f->Tags.append(qMakePair(tag, sub_type));
            theFeatures << f;

            node = node->Next;
        }

        gaiaLinestringPtr way = coll->FirstLinestring;
        while (way) {
            if (way->Points) {
                PrimitiveFeature* f = new PrimitiveFeature();
                f->theId = IFeature::FId(IFeature::LineString, id);
                f->Tags.append(qMakePair(QString("name"), name));
                f->Tags.append(qMakePair(tag, sub_type));

                gaiaGetPoint(way->Coords, 0, &x, &y);
                QPointF pt(x, y);
                if (proj)
                    pt = proj->project(pt);
                f->thePath.moveTo(pt);
                for (int i=1; i<way->Points; ++i) {
                    gaiaGetPoint(way->Coords, i, &x, &y);
                    pt.setX(x);
                    pt.setY(y);
                    if (proj)
                        pt = proj->project(pt);
                    f->thePath.lineTo(pt);
                }
                theFeatures << f;
            }

            way = way->Next;
        }

        gaiaPolygonPtr poly = coll->FirstPolygon;
        while (poly) {
            gaiaRingPtr ring = poly->Exterior;
            if (poly->NumInteriors)
                qDebug() << "has interiors!";
            if (ring->Points) {
                PrimitiveFeature* f = new PrimitiveFeature();
                f->theId = IFeature::FId(IFeature::LineString, id);
                f->Tags.append(qMakePair(QString("name"), name));
                f->Tags.append(qMakePair(tag, sub_type));

                gaiaGetPoint(ring->Coords, 0, &x, &y);
                QPointF pt(x, y);
                if (proj)
                    pt = proj->project(pt);
                f->thePath.moveTo(pt);
                for (int i=1; i<ring->Points; ++i) {
                    gaiaGetPoint(ring->Coords, i, &x, &y);
                    pt.setX(x);
                    pt.setY(y);
                    if (proj)
                        pt = proj->project(pt);
                    f->thePath.lineTo(pt);
                }
                theFeatures << f;
            }

            poly = poly->Next;
        }
    }

    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
}

IImageManager* SpatialiteAdapter::getImageManager()
{
    return NULL;
}

void SpatialiteAdapter::setImageManager(IImageManager* anImageManager)
{
}

void SpatialiteAdapter::cleanup()
{
}

bool SpatialiteAdapter::toXML(QXmlStreamWriter& stream)
{
    bool OK = true;

    stream.writeStartElement("Database");
    if (m_loaded)
        stream.writeAttribute("filename", m_dbName);
    stream.writeEndElement();

    return OK;
}

void SpatialiteAdapter::fromXML(QXmlStreamReader& stream)
{
    while(!stream.atEnd() && !stream.isEndElement()) {
        if (stream.name() == "Database") {
            QString fn = stream.attributes().value("filename").toString();
            if (!fn.isEmpty())
                setFile(fn);
        }
        stream.readNext();
    }
}

QString SpatialiteAdapter::toPropertiesHtml()
{
    QString h;

    if (m_loaded)
        h += "<i>" + tr("Filename") + ": </i>" + m_dbName;

    return h;
}

#ifndef _MOBILE
#if !(QT_VERSION >= QT_VERSION_CHECK(5,0,0))
Q_EXPORT_PLUGIN2(MSpatialiteBackgroundPlugin, SpatialiteAdapterFactory)
#endif
#endif
