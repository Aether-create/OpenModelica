/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-2014, Open Source Modelica Consortium (OSMC),
 * c/o Linköpings universitet, Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 LICENSE OR
 * THIS OSMC PUBLIC LICENSE (OSMC-PL) VERSION 1.2.
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES
 * RECIPIENT'S ACCEPTANCE OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3,
 * ACCORDING TO RECIPIENTS CHOICE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from OSMC, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or
 * http://www.openmodelica.org, and in the OpenModelica distribution.
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */
/*
 *
 * @author Adeel Asghar <adeel.asghar@liu.se>
 *
 * RCS: $Id$
 *
 */

#ifndef TEXTANNOTATION_H
#define TEXTANNOTATION_H

#include "ShapeAnnotation.h"
#include "Component.h"

class Component;

class TextAnnotation : public ShapeAnnotation
{
  Q_OBJECT
public:
  TextAnnotation(QString annotation, Component *pComponent);
  TextAnnotation(ShapeAnnotation *pShapeAnnotation, Component *pParent);
  TextAnnotation(Component *pParent);
  TextAnnotation(QString annotation, GraphicsView *pGraphicsView);
  TextAnnotation(ShapeAnnotation *pShapeAnnotation, GraphicsView *pGraphicsView);
  void parseShapeAnnotation(QString annotation);
  QRectF boundingRect() const;
  QPainterPath shape() const;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
  void drawTextAnnotaion(QPainter *painter);
  QString getShapeAnnotation();
  void updateShape(ShapeAnnotation *pShapeAnnotation);
private:
  Component *mpComponent;

  void initUpdateTextString();
  void updateTextStringHelper(QRegExp regExp);
public slots:
  void updateTextString();
  void duplicate();
};

#endif // TEXTANNOTATION_H
