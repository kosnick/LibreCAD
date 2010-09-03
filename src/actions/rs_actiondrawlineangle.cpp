/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#include "rs_actiondrawlineangle.h"
#include "rs_snapper.h"



RS_ActionDrawLineAngle::RS_ActionDrawLineAngle(RS_EntityContainer& container,
        RS_GraphicView& graphicView,
        double angle,
        bool fixedAngle)
        :RS_PreviewActionInterface("Draw lines with given angle",
                           container, graphicView) {

    this->angle = angle;
    length = 1.0;
    snpPoint = 0;
    this->fixedAngle = fixedAngle;
    pos = RS_Vector(false);
    reset();
}



RS_ActionDrawLineAngle::~RS_ActionDrawLineAngle() {}


QAction* RS_ActionDrawLineAngle::createGUIAction(RS2::ActionType type, QObject* /*parent*/) {
    QAction* action=NULL;
	
	if (type==RS2::ActionDrawLineAngle) {
		// tr("&Angle"),
		// "Line: Angle"
		action = new QAction(tr("&Angle"),  NULL);
		action->setIcon(QIcon(":/extui/linesangle.png"));
	    action->setStatusTip(tr("Draw lines with a given angle"));
	}
	else if (type==RS2::ActionDrawLineHorizontal) {
		//  tr("Line: Horizontal"),
        action = new QAction(tr("&Horizontal"),  NULL);
		action->setIcon(QIcon(":/extui/lineshor.png"));
        action->setStatusTip(tr("Draw horizontal lines"));
	}
	else if (type==RS2::ActionDrawLineVertical) {
		// tr("H&orizontal / Vertical"),
        action = new QAction(tr("hor./vert. line"), NULL);
		action->setIcon(QIcon(":/extui/linesver.png"));
        action->setStatusTip(tr("Draw horizontal/vertical lines"));
	}
    return action;
}

void RS_ActionDrawLineAngle::reset() {
    data = RS_LineData(RS_Vector(false),
                       RS_Vector(false));
}



void RS_ActionDrawLineAngle::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}



void RS_ActionDrawLineAngle::trigger() {
    RS_PreviewActionInterface::trigger();

    preparePreview();
    RS_Line* line = new RS_Line(container,
                                data);
    line->setLayerToActive();
    line->setPenToActive();
    container->addEntity(line);

    // upd. undo list:
    if (document!=NULL) {
        document->startUndoCycle();
        document->addUndoable(line);
        document->endUndoCycle();
    }
	
    graphicView->moveRelativeZero(data.startpoint);
	graphicView->redraw(RS2::RedrawDrawing);
    RS_DEBUG->print("RS_ActionDrawLineAngle::trigger(): line added: %d",
                    line->getId());
}



void RS_ActionDrawLineAngle::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineAngle::mouseMoveEvent begin");

    if (getStatus()==SetPos) {
        pos = snapPoint(e);
        deletePreview();
        preparePreview();
        preview->addEntity(new RS_Line(preview,
                                       data));
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawLineAngle::mouseMoveEvent end");
}



void RS_ActionDrawLineAngle::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        if (getStatus()==SetPos) {
            RS_CoordinateEvent ce(snapPoint(e));
            coordinateEvent(&ce);
        }
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}


void RS_ActionDrawLineAngle::preparePreview() {
    RS_Vector p1, p2;
    // End:
    if (snpPoint == 2) {
        p2.setPolar(length * -1, angle);
    } else {
        p2.setPolar(length, angle);
    }

    // Middle:
    if (snpPoint == 1) {
        p1 = pos - (p2 / 2);
    } else {
        p1 = pos;
    }

    p2 += p1;
    data = RS_LineData(p1, p2);
}


void RS_ActionDrawLineAngle::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    switch (getStatus()) {
    case SetPos:
        pos = e->getCoordinate();
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawLineAngle::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetPos:
        if (!fixedAngle && checkCommand("angle", c)) {
            deletePreview();
            setStatus(SetAngle);
        } else if (checkCommand("length", c)) {
            deletePreview();
            setStatus(SetLength);
        }
        break;

    case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok==true) {
                angle = RS_Math::deg2rad(a);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(SetPos);
        }
        break;

    case SetLength: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok==true) {
                length = l;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(SetPos);
        }
        break;

    default:
        break;
    }
}



RS_StringList RS_ActionDrawLineAngle::getAvailableCommands() {
    RS_StringList cmd;

    switch (getStatus()) {
    case SetPos:
        if (!fixedAngle) {
            cmd += command("angle");
        }
        cmd += command("length");
        break;
    default:
        break;
    }

    return cmd;
}


void RS_ActionDrawLineAngle::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPos:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify position"),
                                            tr("Cancel"));
        break;

    case SetAngle:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter angle:"), tr("Back"));
        break;

    case SetLength:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter length:"), tr("Back"));
        break;

    default:
        break;
    }
}



void RS_ActionDrawLineAngle::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDrawLineAngle::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionDrawLineAngle::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawLineAngle::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarLines);
    }
}

// EOF
