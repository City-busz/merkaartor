#include "CreateRoundaboutInteraction.h"

#include "MainWindow.h"
#include "DocumentCommands.h"
#include "WayCommands.h"
#include "NodeCommands.h"
#include "Painting.h"
#include "Way.h"
#include "Node.h"
#include "LineF.h"
#include "PropertiesDock.h"
#include "MerkaartorPreferences.h"
#include "Global.h"

#include <QDockWidget>
#include <QPainter>

#include <math.h>

CreateRoundaboutInteraction::CreateRoundaboutInteraction(MainWindow* aMain)
    : Interaction(aMain), Center(0,0), HaveCenter(false), theDock(0)
{
#ifndef _MOBILE
    theDock = new QDockWidget(theMain);
    QWidget* DockContent = new QWidget(theDock);
    DockData.setupUi(DockContent);
    theDock->setWidget(DockContent);
    theDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    theMain->addDockWidget(Qt::LeftDockWidgetArea, theDock);
    theDock->show();
    DockData.DriveRight->setChecked(M_PREFS->getrightsidedriving());

    theMain->view()->setCursor(cursor());
#endif
}

CreateRoundaboutInteraction::~CreateRoundaboutInteraction()
{
    M_PREFS->setrightsidedriving(DockData.DriveRight->isChecked());
    delete theDock;
    view()->update();
}

QString CreateRoundaboutInteraction::toHtml()
{
    QString help;
    //help = (MainWindow::tr("LEFT-CLICK to select; LEFT-DRAG to move"));

    QString desc;
    desc = QString("<big><b>%1</b></big><br/>").arg(MainWindow::tr("Create roundabout Interaction"));
    desc += QString("<b>%1</b><br/>").arg(help);

    QString S =
    "<html><head/><body>"
    "<small><i>" + QString(metaObject()->className()) + "</i></small><br/>"
    + desc;
    S += "</body></html>";

    return S;
}

void CreateRoundaboutInteraction::mousePressEvent(QMouseEvent * event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        if (!HaveCenter)
        {
            HaveCenter = true;
            view()->setInteracting(true);
            Center = XY_TO_COORD(event->pos());
        }
        else
        {
            QPointF CenterF(COORD_TO_XY(Center));
            qreal Radius = distance(CenterF,LastCursor)/view()->pixelPerM();
            qreal Precision = 2.49;
            if (Radius<2.5)
                Radius = 2.5;
            qreal Angle = 2*acos(1-Precision/Radius);
            qreal Steps = ceil(2*M_PI/Angle);
            Angle = 2*M_PI/Steps;
            Radius *= view()->pixelPerM();
            qreal Modifier = DockData.DriveRight->isChecked()?-1:1;
            QBrush SomeBrush(QColor(0xff,0x77,0x11,128));
            QPen TP(SomeBrush,view()->pixelPerM()*4+1);
            QPointF Prev(CenterF.x()+cos(Modifier*Angle/2)*Radius,CenterF.y()+sin(Modifier*Angle/2)*Radius);
            Node* First = g_backend.allocNode(theMain->document()->getDirtyOrOriginLayer(), XY_TO_COORD(Prev.toPoint()));
            Way* R = g_backend.allocWay(theMain->document()->getDirtyOrOriginLayer());
            CommandList* L  = new CommandList(MainWindow::tr("Create Roundabout %1").arg(R->id().numId), R);
            L->add(new AddFeatureCommand(theMain->document()->getDirtyOrOriginLayer(),R,true));
            R->add(First);
            L->add(new AddFeatureCommand(theMain->document()->getDirtyOrOriginLayer(),First,true));
            if (M_PREFS->getAutoSourceTag()) {
                QStringList sl = theMain->document()->getCurrentSourceTags();
                if (sl.size())
                    R->setTag("source", sl.join(";"));
            }
            // "oneway" is implied on roundabouts
            //R->setTag("oneway","yes");
            R->setTag("junction","roundabout");
            for (qreal a = Angle*3/2; a<2*M_PI; a+=Angle)
            {
                QPointF Next(CenterF.x()+cos(Modifier*a)*Radius,CenterF.y()+sin(Modifier*a)*Radius);
                Node* New = g_backend.allocNode(theMain->document()->getDirtyOrOriginLayer(), XY_TO_COORD(Next.toPoint()));
                L->add(new AddFeatureCommand(theMain->document()->getDirtyOrOriginLayer(),New,true));
                R->add(New);
            }
            R->add(First);
            for (FeatureIterator it(document()); !it.isEnd(); ++it)
            {
                Way* W1 = CAST_WAY(it.get());
                if (W1 && (W1 != R))
                    Way::createJunction(theMain->document(), L, R, W1, true);
            }
            theMain->properties()->setSelection(R);
            document()->addHistory(L);
            view()->setInteracting(false);
            view()->invalidate(true, true, false);
            theMain->launchInteraction(0);
        }
    }
    else
        Interaction::mousePressEvent(event);
}

void CreateRoundaboutInteraction::mouseMoveEvent(QMouseEvent* event)
{
    LastCursor = event->pos();
    if (HaveCenter)
        view()->update();
    Interaction::mouseMoveEvent(event);
}

void CreateRoundaboutInteraction::mouseReleaseEvent(QMouseEvent* anEvent)
{
    if (M_PREFS->getMouseSingleButton() && anEvent->button() == Qt::RightButton) {
        HaveCenter = false;
        view()->setInteracting(false);
        view()->update();
    }
    Interaction::mouseReleaseEvent(anEvent);
}

void CreateRoundaboutInteraction::paintEvent(QPaintEvent* , QPainter& thePainter)
{
    if (HaveCenter)
    {
        QPointF CenterF(COORD_TO_XY(Center));
        qreal Radius = distance(CenterF,LastCursor)/view()->pixelPerM();
        qreal Precision = 1.99;
        if (Radius<2)
            Radius = 2;
        qreal Angle = 2*acos(1-Precision/Radius);
        qreal Steps = ceil(2*M_PI/Angle);
        Angle = 2*M_PI/Steps;
        Radius *= view()->pixelPerM();
        qreal Modifier = DockData.DriveRight->isChecked()?-1:1;
        QBrush SomeBrush(QColor(0xff,0x77,0x11,128));
        QPen TP(SomeBrush,view()->pixelPerM()*4);
        QPointF Prev(CenterF.x()+cos(Modifier*Angle/2)*Radius,CenterF.y()+sin(Modifier*Angle/2)*Radius);
        for (qreal a = Angle*3/2; a<2*M_PI; a+=Angle)
        {
            QPointF Next(CenterF.x()+cos(Modifier*a)*Radius,CenterF.y()+sin(Modifier*a)*Radius);
            ::draw(thePainter,TP,Feature::OneWay, Prev,Next,4,view()->projection());
            Prev = Next;
        }
        QPointF Next(CenterF.x()+cos(Modifier*Angle/2)*Radius,CenterF.y()+sin(Modifier*Angle/2)*Radius);
        ::draw(thePainter,TP,Feature::OneWay, Prev,Next,4,view()->projection());
    }
}

#ifndef _MOBILE
QCursor CreateRoundaboutInteraction::cursor() const
{
    return QCursor(Qt::CrossCursor);
}
#endif
