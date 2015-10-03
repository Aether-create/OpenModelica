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

#include "Component.h"
#include "ComponentProperties.h"

/*!
 * \class ComponentInfo
 * \brief A class containing the information about the component like visibility, stream, casuality etc.
 */
/*!
 * \brief ComponentInfo::ComponentInfo
 * \param value
 * \param pParent
 */
ComponentInfo::ComponentInfo(QString value, QObject *pParent)
  : QObject(pParent)
{
  mClassName = "";
  mName = "";
  mComment = "";
  mIsProtected = false;
  mIsFinal = false;
  mIsFlow = false;
  mIsStream = false;
  mIsReplaceable = false;
  mVariabilityMap.insert("constant", "constant");
  mVariabilityMap.insert("discrete", "discrete");
  mVariabilityMap.insert("parameter", "parameter");
  mVariabilityMap.insert("unspecified", "default");
  mVariability = "";
  mIsInner = false;
  mIsOuter = false;
  mCasualityMap.insert("input", "input");
  mCasualityMap.insert("output", "output");
  mCasualityMap.insert("unspecified", "none");
  mCasuality = "";
  mArrayIndex = "";
  mIsArray = false;
  parseComponentInfoString(value);
}

/*!
 * \brief ComponentInfo::parseComponentInfoString
 * Parses the component info string.
 * \param value
 */
void ComponentInfo::parseComponentInfoString(QString value)
{
  if (value.isEmpty()) {
    return;
  }
  QStringList list = StringHandler::unparseStrings(value);
  // read the class name
  if (list.size() > 0) {
    mClassName = list.at(0);
  } else {
    return;
  }
  // read the name
  if (list.size() > 1) {
    mName = list.at(1);
  } else {
    return;
  }
  // read the class comment
  if (list.size() > 2) {
    mComment = list.at(2);
  } else {
    return;
  }
  // read the class access
  if (list.size() > 3) {
    mIsProtected = StringHandler::removeFirstLastQuotes(list.at(3)).contains("protected");
  } else {
    return;
  }
  // read the final attribute
  if (list.size() > 4) {
    mIsFinal = list.at(4).contains("true");
  } else {
    return;
  }
  // read the flow attribute
  if (list.size() > 5) {
    mIsFlow = list.at(5).contains("true");
  } else {
    return;
  }
  // read the stream attribute
  if (list.size() > 6) {
    mIsStream = list.at(6).contains("true");
  } else {
    return;
  }
  // read the replaceable attribute
  if (list.size() > 7) {
    mIsReplaceable = list.at(7).contains("true");
  } else {
    return;
  }
  // read the variability attribute
  if (list.size() > 8) {
    QMap<QString, QString>::iterator variability_it;
    for (variability_it = mVariabilityMap.begin(); variability_it != mVariabilityMap.end(); ++variability_it) {
      if (variability_it.key().compare(StringHandler::removeFirstLastQuotes(list.at(8))) == 0) {
        mVariability = variability_it.value();
        break;
      }
    }
  }
  // read the inner attribute
  if (list.size() > 9) {
    mIsInner = list.at(9).contains("inner");
    mIsOuter = list.at(9).contains("outer");
  } else {
    return;
  }
  // read the casuality attribute
  if (list.size() > 10) {
    QMap<QString, QString>::iterator casuality_it;
    for (casuality_it = mCasualityMap.begin(); casuality_it != mCasualityMap.end(); ++casuality_it) {
      if (casuality_it.key().compare(StringHandler::removeFirstLastQuotes(list.at(10))) == 0) {
        mCasuality = casuality_it.value();
        break;
      }
    }
  }
  // read the array index value
  if (list.size() > 11) {
    setArrayIndex(list.at(11));
  }
}

/*!
 * \brief ComponentInfo::setArrayIndex
 * Sets the array index
 * \param arrayIndex
 */
void ComponentInfo::setArrayIndex(QString arrayIndex)
{
  mArrayIndex = arrayIndex;
  if (mArrayIndex.compare("{}") != 0) {
    mIsArray = true;
  } else {
    mIsArray = false;
  }
}

Component::Component(QString name, LibraryTreeItem *pLibraryTreeItem, QString transformation, QPointF position, ComponentInfo *pComponentInfo,
                     GraphicsView *pGraphicsView)
  : QGraphicsItem(0), mpReferenceComponent(0), mpParentComponent(0)
{
  setZValue(1000);
  mpLibraryTreeItem = pLibraryTreeItem;
  mpComponentInfo = pComponentInfo;
  mpComponentInfo->setName(name);
  mpGraphicsView = pGraphicsView;
  mIsInheritedComponent = false;
  mComponentType = Component::Root;
  mTransformationString = transformation;
  // Construct the temporary polygon that is used when scaling
  mpResizerRectangle = new QGraphicsRectItem;
  mpResizerRectangle->setZValue(5000);  // set to a very high value
  mpGraphicsView->addItem(mpResizerRectangle);
  QPen pen;
  pen.setStyle(Qt::DotLine);
  pen.setColor(Qt::transparent);
  mpResizerRectangle->setPen(pen);
  setOldScenePosition(QPointF(0, 0));
  setOldPosition(QPointF(0, 0));
  setComponentFlags(true);
  createNonExistingComponent();
  createDefaultComponent();
  if (mpGraphicsView->getModelWidget()->getLibraryTreeItem()->getLibraryType() == LibraryTreeItem::TLM) {
//    parseAnnotationString(Helper::defaultComponentAnnotationString);
  } else {
    if (mpLibraryTreeItem->isNonExisting()) {
      mpNonExistingComponentLine->setVisible(true);
    } else {
      createClassInheritedShapes();
      createClassShapes(mpLibraryTreeItem);
      createClassInheritedComponents();
      createClassComponents(mpLibraryTreeItem);
      if (!mpLibraryTreeItem->getModelWidget()->getIconGraphicsView()->hasAnnotation()) {
        mpDefaultComponentRectangle->setVisible(true);
        mpDefaultComponentText->setVisible(true);
      }
    }
  }
  // transformation
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpCoOrdinateSystem = mpLibraryTreeItem->getModelWidget()->getIconGraphicsView()->getCoOrdinateSystem();
  } else {
    mpCoOrdinateSystem = mpLibraryTreeItem->getModelWidget()->getDiagramGraphicsView()->getCoOrdinateSystem();
  }
  mpTransformation = new Transformation(mpGraphicsView->getViewType());
  mpTransformation->parseTransformationString(transformation, boundingRect().width(), boundingRect().height());
  if (transformation.isEmpty()) {
    // snap to grid while creating component
    position = mpGraphicsView->snapPointToGrid(position);
    mpTransformation->setOrigin(position);
    qreal initialScale = mpCoOrdinateSystem->getInitialScale();
    mpTransformation->setExtent1(QPointF(initialScale * boundingRect().left(), initialScale * boundingRect().top()));
    mpTransformation->setExtent2(QPointF(initialScale * boundingRect().right(), initialScale * boundingRect().bottom()));
    mpTransformation->setRotateAngle(0.0);
  }
  setTransform(mpTransformation->getTransformationMatrix());
  createActions();
  mpOriginItem = new OriginItem(this);
  createResizerItems();
  setToolTip(tr("<b>%1</b> %2").arg(mpLibraryTreeItem->getNameStructure()).arg(mpComponentInfo->getName()));
  connect(mpLibraryTreeItem, SIGNAL(loaded(LibraryTreeItem*)), SLOT(handleLoaded()));
  connect(mpLibraryTreeItem, SIGNAL(unLoaded(LibraryTreeItem*)), SLOT(handleUnloaded()));
  connect(this, SIGNAL(transformHasChanged()), SLOT(updatePlacementAnnotation()));
  connect(this, SIGNAL(transformHasChanged()), SLOT(updateOriginItem()));
}

Component::Component(Component *pComponent, GraphicsView *pGraphicsView, Component *pParent)
  : QGraphicsItem(pParent), mpReferenceComponent(pComponent), mpParentComponent(pParent)
{
  setZValue(3000);
  mpLibraryTreeItem = mpReferenceComponent->getLibraryTreeItem();
  mpComponentInfo = mpReferenceComponent->getComponentInfo();
  mIsInheritedComponent = mpReferenceComponent->isInheritedComponent();
  mComponentType = Component::Port;
  mpGraphicsView = pGraphicsView;
  mTransformationString = mpReferenceComponent->getTransformationString();
  createClassShapes(mpLibraryTreeItem);
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpCoOrdinateSystem = mpLibraryTreeItem->getModelWidget()->getIconGraphicsView()->getCoOrdinateSystem();
  } else {
    mpCoOrdinateSystem = mpLibraryTreeItem->getModelWidget()->getDiagramGraphicsView()->getCoOrdinateSystem();
  }
  mpTransformation = new Transformation(mpReferenceComponent->getTransformation());
  setTransform(mpTransformation->getTransformationMatrix());
  mpOriginItem = 0;
  setToolTip(tr("<b>%1</b> %2<br /><br />Component declared in %3").arg(mpLibraryTreeItem->getNameStructure()).arg(mpComponentInfo->getName())
             .arg(mpReferenceComponent->getGraphicsView()->getModelWidget()->getLibraryTreeItem()->getNameStructure()));
  connect(mpReferenceComponent, SIGNAL(added()), SLOT(referenceComponentAdded()));
  connect(mpReferenceComponent, SIGNAL(transformHasChanged()), SLOT(referenceComponentChanged()));
  connect(mpReferenceComponent, SIGNAL(displayTextChanged()), SIGNAL(displayTextChanged()));
  connect(mpReferenceComponent, SIGNAL(deleted()), SLOT(referenceComponentDeleted()));
}

Component::Component(Component *pComponent, GraphicsView *pGraphicsView)
  : QGraphicsItem(0), mpReferenceComponent(pComponent), mpParentComponent(0)
{
  setZValue(1000);
  mpLibraryTreeItem = mpReferenceComponent->getLibraryTreeItem();
  mpComponentInfo = mpReferenceComponent->getComponentInfo();
  mpGraphicsView = pGraphicsView;
  mIsInheritedComponent = true;
  mComponentType = Component::Root;
  mTransformationString = mpReferenceComponent->getTransformationString();
  //Construct the temporary polygon that is used when scaling
  mpResizerRectangle = new QGraphicsRectItem;
  mpResizerRectangle->setZValue(5000);  // set to a very high value
  mpGraphicsView->addItem(mpResizerRectangle);
  QPen pen;
  pen.setStyle(Qt::DotLine);
  pen.setColor(Qt::transparent);
  mpResizerRectangle->setPen(pen);
  setOldScenePosition(QPointF(0, 0));
  setOldPosition(QPointF(0, 0));
  setComponentFlags(true);
  createNonExistingComponent();
  createDefaultComponent();
  if (mpGraphicsView->getModelWidget()->getLibraryTreeItem()->getLibraryType() == LibraryTreeItem::TLM) {
//    parseAnnotationString(Helper::defaultComponentAnnotationString);
  } else {
    if (mpLibraryTreeItem->isNonExisting()) {
      mpNonExistingComponentLine->setVisible(true);
      // transformation
      mpCoOrdinateSystem = new CoOrdinateSystem;
    } else {
      createClassInheritedShapes();
      createClassShapes(mpLibraryTreeItem);
      createClassInheritedComponents();
      createClassComponents(mpLibraryTreeItem);
      if (!mpLibraryTreeItem->getModelWidget()->getIconGraphicsView()->hasAnnotation()) {
        mpDefaultComponentRectangle->setVisible(true);
        mpDefaultComponentText->setVisible(true);
      }
      // transformation
      if (mpGraphicsView->getViewType() == StringHandler::Icon) {
        mpCoOrdinateSystem = mpLibraryTreeItem->getModelWidget()->getIconGraphicsView()->getCoOrdinateSystem();
      } else {
        mpCoOrdinateSystem = mpLibraryTreeItem->getModelWidget()->getDiagramGraphicsView()->getCoOrdinateSystem();
      }
    }
  }
  mpTransformation = new Transformation(mpReferenceComponent->getTransformation());
  setTransform(mpTransformation->getTransformationMatrix());
  createActions();
  mpOriginItem = new OriginItem(this);
  mpGraphicsView->addItem(mpOriginItem);
  createResizerItems();
  mpGraphicsView->addItem(this);
  setToolTip(tr("<b>%1</b> %2<br /><br />Component declared in %3").arg(mpLibraryTreeItem->getNameStructure()).arg(mpComponentInfo->getName())
             .arg(mpReferenceComponent->getGraphicsView()->getModelWidget()->getLibraryTreeItem()->getNameStructure()));
  connect(mpLibraryTreeItem, SIGNAL(loaded(LibraryTreeItem*)), SLOT(handleLoaded()));
  connect(mpLibraryTreeItem, SIGNAL(unLoaded(LibraryTreeItem*)), SLOT(handleUnloaded()));
  connect(mpReferenceComponent, SIGNAL(added()), SLOT(referenceComponentAdded()));
  connect(mpReferenceComponent, SIGNAL(transformHasChanged()), SLOT(referenceComponentChanged()));
  connect(mpReferenceComponent, SIGNAL(transformHasChanged()), SLOT(updateOriginItem()));
  connect(mpReferenceComponent, SIGNAL(displayTextChanged()), SIGNAL(displayTextChanged()));
  connect(mpReferenceComponent, SIGNAL(deleted()), SLOT(referenceComponentDeleted()));
}

QString Component::getInheritedClassName()
{
  return "";
}

void Component::createNonExistingComponent()
{
  mpNonExistingComponentLine = new LineAnnotation(this);
  mpNonExistingComponentLine->setVisible(false);
}

void Component::createDefaultComponent()
{
  mpDefaultComponentRectangle = new RectangleAnnotation(this);
  mpDefaultComponentRectangle->setVisible(false);
  mpDefaultComponentText = new TextAnnotation(this);
  mpDefaultComponentText->setVisible(false);
}

void Component::createClassInheritedShapes()
{
  foreach (ModelWidget::InheritedClass *pInheritedClass, mpLibraryTreeItem->getModelWidget()->getInheritedClassesList()) {
    createClassShapes(pInheritedClass->mpLibraryTreeItem);
  }
}

void Component::createClassShapes(LibraryTreeItem *pLibraryTreeItem)
{
  GraphicsView *pGraphicsView = pLibraryTreeItem->getModelWidget()->getIconGraphicsView();
  if (pLibraryTreeItem->getRestriction() == StringHandler::Connector && mpGraphicsView->getViewType() == StringHandler::Diagram &&
      mComponentType == Component::Root && pLibraryTreeItem->getModelWidget()->getDiagramGraphicsView()->hasAnnotation()) {
    pGraphicsView = pLibraryTreeItem->getModelWidget()->getDiagramGraphicsView();
  }
  foreach (ShapeAnnotation *pShapeAnnotation, pGraphicsView->getShapesList()) {
    if (dynamic_cast<LineAnnotation*>(pShapeAnnotation)) {
      mShapesList.append(new LineAnnotation(pShapeAnnotation, this));
    } else if (dynamic_cast<PolygonAnnotation*>(pShapeAnnotation)) {
      mShapesList.append(new PolygonAnnotation(pShapeAnnotation, this));
    } else if (dynamic_cast<RectangleAnnotation*>(pShapeAnnotation)) {
      mShapesList.append(new RectangleAnnotation(pShapeAnnotation, this));
    } else if (dynamic_cast<EllipseAnnotation*>(pShapeAnnotation)) {
      mShapesList.append(new EllipseAnnotation(pShapeAnnotation, this));
    } else if (dynamic_cast<TextAnnotation*>(pShapeAnnotation)) {
      mShapesList.append(new TextAnnotation(pShapeAnnotation, this));
    } else if (dynamic_cast<BitmapAnnotation*>(pShapeAnnotation)) {
      mShapesList.append(new BitmapAnnotation(pShapeAnnotation, this));
    }
  }
  connect(pLibraryTreeItem, SIGNAL(shapeAdded(LibraryTreeItem*,ShapeAnnotation*,GraphicsView*)),
          SLOT(handleShapeAdded()), Qt::UniqueConnection);
}

void Component::createClassInheritedComponents()
{
  foreach (ModelWidget::InheritedClass *pInheritedClass, mpLibraryTreeItem->getModelWidget()->getInheritedClassesList()) {
    createClassComponents(pInheritedClass->mpLibraryTreeItem);
  }
}

void Component::createClassComponents(LibraryTreeItem *pLibraryTreeItem)
{
  foreach (Component *pComponent, pLibraryTreeItem->getModelWidget()->getIconGraphicsView()->getComponentsList()) {
    mComponentsList.append(new Component(pComponent, mpGraphicsView, this));
  }
}

/*!
 * \brief Component::hasShapeAnnotation
 * Checks if Component has any ShapeAnnotation
 * \param pComponent
 * \return
 */
bool Component::hasShapeAnnotation(Component *pComponent)
{
  if (!pComponent->getShapesList().isEmpty()) {
    return true;
  }
  bool iconAnnotationFound = false;
  foreach (Component *pInheritedComponent, pComponent->getInheritanceList()) {
    iconAnnotationFound = hasShapeAnnotation(pInheritedComponent);
    if (iconAnnotationFound) {
      return iconAnnotationFound;
    }
  }
  foreach (Component *pChildComponent, pComponent->getComponentsList()) {
    iconAnnotationFound = hasShapeAnnotation(pChildComponent);
    if (iconAnnotationFound) {
      return iconAnnotationFound;
    }
  }
  return iconAnnotationFound;
}

void Component::createActions()
{
  // Parameters Action
  mpParametersAction = new QAction(Helper::parameters, mpGraphicsView);
  mpParametersAction->setStatusTip(tr("Shows the component parameters"));
  connect(mpParametersAction, SIGNAL(triggered()), SLOT(showParameters()));
  // Attributes Action
  mpAttributesAction = new QAction(Helper::attributes, mpGraphicsView);
  mpAttributesAction->setStatusTip(tr("Shows the component attributes"));
  connect(mpAttributesAction, SIGNAL(triggered()), SLOT(showAttributes()));
  // View Class Action
  mpViewClassAction = new QAction(QIcon(":/Resources/icons/model.svg"), Helper::viewClass, mpGraphicsView);
  mpViewClassAction->setStatusTip(Helper::viewClassTip);
  connect(mpViewClassAction, SIGNAL(triggered()), SLOT(viewClass()));
  // View Documentation Action
  mpViewDocumentationAction = new QAction(QIcon(":/Resources/icons/info-icon.svg"), Helper::viewDocumentation, mpGraphicsView);
  mpViewDocumentationAction->setStatusTip(Helper::viewDocumentationTip);
  connect(mpViewDocumentationAction, SIGNAL(triggered()), SLOT(viewDocumentation()));
  // TLM  attributes Action
  mpTLMAttributesAction = new QAction(Helper::attributes, mpGraphicsView);
  mpTLMAttributesAction->setStatusTip(tr("Shows the component attributes"));
  connect(mpTLMAttributesAction, SIGNAL(triggered()), SLOT(showTLMAttributes()));
}

void Component::createResizerItems()
{
  bool isSystemLibrary = mpGraphicsView->getModelWidget()->getLibraryTreeItem()->isSystemLibrary();
  qreal x1, y1, x2, y2;
  getResizerItemsPositions(&x1, &y1, &x2, &y2);
  //Bottom left resizer
  mpBottomLeftResizerItem = new ResizerItem(this);
  mpBottomLeftResizerItem->setPos(mapFromScene(x1, y1));
  mpBottomLeftResizerItem->setResizePosition(ResizerItem::BottomLeft);
  connect(mpBottomLeftResizerItem, SIGNAL(resizerItemPressed(ResizerItem*)), SLOT(prepareResizeComponent(ResizerItem*)));
  connect(mpBottomLeftResizerItem, SIGNAL(resizerItemMoved(QPointF)), SLOT(resizeComponent(QPointF)));
  connect(mpBottomLeftResizerItem, SIGNAL(resizerItemReleased()), SLOT(finishResizeComponent()));
  connect(mpBottomLeftResizerItem, SIGNAL(resizerItemPositionChanged()), SIGNAL(transformHasChanged()));
  mpBottomLeftResizerItem->blockSignals(isSystemLibrary || isInheritedComponent());
  //Top left resizer
  mpTopLeftResizerItem = new ResizerItem(this);
  mpTopLeftResizerItem->setPos(mapFromScene(x1, y2));
  mpTopLeftResizerItem->setResizePosition(ResizerItem::TopLeft);
  connect(mpTopLeftResizerItem, SIGNAL(resizerItemPressed(ResizerItem*)), SLOT(prepareResizeComponent(ResizerItem*)));
  connect(mpTopLeftResizerItem, SIGNAL(resizerItemMoved(QPointF)), SLOT(resizeComponent(QPointF)));
  connect(mpTopLeftResizerItem, SIGNAL(resizerItemReleased()), SLOT(finishResizeComponent()));
  connect(mpTopLeftResizerItem, SIGNAL(resizerItemPositionChanged()), SIGNAL(transformHasChanged()));
  mpTopLeftResizerItem->blockSignals(isSystemLibrary || isInheritedComponent());
  //Top Right resizer
  mpTopRightResizerItem = new ResizerItem(this);
  mpTopRightResizerItem->setPos(mapFromScene(x2, y2));
  mpTopRightResizerItem->setResizePosition(ResizerItem::TopRight);
  connect(mpTopRightResizerItem, SIGNAL(resizerItemPressed(ResizerItem*)), SLOT(prepareResizeComponent(ResizerItem*)));
  connect(mpTopRightResizerItem, SIGNAL(resizerItemMoved(QPointF)), SLOT(resizeComponent(QPointF)));
  connect(mpTopRightResizerItem, SIGNAL(resizerItemReleased()), SLOT(finishResizeComponent()));
  connect(mpTopRightResizerItem, SIGNAL(resizerItemPositionChanged()), SIGNAL(transformHasChanged()));
  mpTopRightResizerItem->blockSignals(isSystemLibrary || isInheritedComponent());
  //Bottom Right resizer
  mpBottomRightResizerItem = new ResizerItem(this);
  mpBottomRightResizerItem->setPos(mapFromScene(x2, y1));
  mpBottomRightResizerItem->setResizePosition(ResizerItem::BottomRight);
  connect(mpBottomRightResizerItem, SIGNAL(resizerItemPressed(ResizerItem*)), SLOT(prepareResizeComponent(ResizerItem*)));
  connect(mpBottomRightResizerItem, SIGNAL(resizerItemMoved(QPointF)), SLOT(resizeComponent(QPointF)));
  connect(mpBottomRightResizerItem, SIGNAL(resizerItemReleased()), SLOT(finishResizeComponent()));
  connect(mpBottomRightResizerItem, SIGNAL(resizerItemPositionChanged()), SIGNAL(transformHasChanged()));
  mpBottomRightResizerItem->blockSignals(isSystemLibrary || isInheritedComponent());
}

void Component::getResizerItemsPositions(qreal *x1, qreal *y1, qreal *x2, qreal *y2)
{
  qreal x11, y11, x22, y22;
  sceneBoundingRect().getCoords(&x11, &y11, &x22, &y22);
  if (x11 < x22)
  {
    *x1 = x11;
    *x2 = x22;
  }
  else
  {
    *x1 = x22;
    *x2 = x11;
  }
  if (y11 < y22)
  {
    *y1 = y11;
    *y2 = y22;
  }
  else
  {
    *y1 = y22;
    *y2 = y11;
  }
}

void Component::showResizerItems()
{
  // show the origin item
  if (mpTransformation->hasOrigin()) {
    mpOriginItem->setPos(mpTransformation->getOrigin());
    mpOriginItem->setActive();
  }
  qreal x1, y1, x2, y2;
  getResizerItemsPositions(&x1, &y1, &x2, &y2);
  //Bottom left resizer
  mpBottomLeftResizerItem->setPos(mapFromScene(x1, y1));
  mpBottomLeftResizerItem->setActive();
  //Top left resizer
  mpTopLeftResizerItem->setPos(mapFromScene(x1, y2));
  mpTopLeftResizerItem->setActive();
  //Top Right resizer
  mpTopRightResizerItem->setPos(mapFromScene(x2, y2));
  mpTopRightResizerItem->setActive();
  //Bottom Right resizer
  mpBottomRightResizerItem->setPos(mapFromScene(x2, y1));
  mpBottomRightResizerItem->setActive();
}

void Component::hideResizerItems()
{
  mpOriginItem->setPassive();
  mpBottomLeftResizerItem->setPassive();
  mpTopLeftResizerItem->setPassive();
  mpTopRightResizerItem->setPassive();
  mpBottomRightResizerItem->setPassive();
}

void Component::getScale(qreal *sx, qreal *sy)
{
  qreal angle = mpTransformation->getRotateAngle();
  if (transform().type() == QTransform::TxScale) {
    *sx = transform().m11() / (cos(angle * (M_PI / 180)));
    *sy = transform().m22() / (cos(angle * (M_PI / 180)));
  } else {
    *sx = transform().m12() / sin(angle * (M_PI / 180));
    *sy = -transform().m21() / sin(angle * (M_PI / 180));
  }
}

void Component::setOriginAndExtents()
{
  if (!mpTransformation->hasOrigin()) {
    QPointF extent1, extent2;
    qreal sx, sy;
    getScale(&sx, &sy);
    extent1.setX(sx * boundingRect().left());
    extent1.setY(sy * boundingRect().top());
    extent2.setX(sx * boundingRect().right());
    extent2.setY(sy * boundingRect().bottom());
    mpTransformation->setOrigin(scenePos());
    mpTransformation->setExtent1(extent1);
    mpTransformation->setExtent2(extent2);
  }
}

QRectF Component::boundingRect() const
{
  qreal left = mpCoOrdinateSystem->getExtent().at(0).x();
  qreal bottom = mpCoOrdinateSystem->getExtent().at(0).y();
  qreal right = mpCoOrdinateSystem->getExtent().at(1).x();
  qreal top = mpCoOrdinateSystem->getExtent().at(1).y();
  return QRectF(left, bottom, fabs(left - right), fabs(bottom - top));
}

void Component::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
  if (mpTransformation) {
    setVisible(mpTransformation->getVisible());
  }
}

Component* Component::getRootParentComponent()
{
  Component *pComponent;
  pComponent = this;
  while (pComponent->mpParentComponent)
    pComponent = pComponent->mpParentComponent;
  return pComponent;
}

void Component::setComponentFlags(bool enable)
{
  /*
    Only set the ItemIsMovable & ItemSendsGeometryChanges flags on component if the class is not a system library class
    AND component is not an inherited shape.
    */
  if (!mpGraphicsView->getModelWidget()->getLibraryTreeItem()->isSystemLibrary() && !isInheritedComponent()) {
    setFlag(QGraphicsItem::ItemIsMovable, enable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, enable);
  }
  setFlag(QGraphicsItem::ItemIsSelectable, enable);
}

QString Component::getTransformationAnnotation()
{
  QString annotationString;
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    annotationString.append("iconTransformation=transformation(");
  } else if (mpGraphicsView->getViewType() == StringHandler::Diagram) {
    annotationString.append("transformation=transformation(");
  }
  // add the origin
  if (mpTransformation->hasOrigin()) {
    annotationString.append("origin={").append(QString::number(mpTransformation->getOrigin().x())).append(",");
    annotationString.append(QString::number(mpTransformation->getOrigin().y())).append("}, ");
  }
  // add extent points
  QPointF extent1 = mpTransformation->getExtent1();
  QPointF extent2 = mpTransformation->getExtent2();
  annotationString.append("extent={").append("{").append(QString::number(extent1.x()));
  annotationString.append(",").append(QString::number(extent1.y())).append("},");
  annotationString.append("{").append(QString::number(extent2.x())).append(",");
  annotationString.append(QString::number(extent2.y())).append("}}, ");
  // add icon rotation
  annotationString.append("rotation=").append(QString::number(mpTransformation->getRotateAngle())).append(")");
  return annotationString;
}

QString Component::getPlacementAnnotation()
{
  // create the placement annotation string
  QString placementAnnotationString = "annotate=Placement(";
  if (mpTransformation) {
    placementAnnotationString.append("visible=").append(mpTransformation->getVisible() ? "true" : "false");
  }
  if (mpLibraryTreeItem->getRestriction() == StringHandler::Connector) {
    if (mpGraphicsView->getViewType() == StringHandler::Icon) {
      // first get the component from diagram view and get the transformations
      Component *pComponent;
      pComponent = mpGraphicsView->getModelWidget()->getDiagramGraphicsView()->getComponentObject(getName());
      if (pComponent) {
        placementAnnotationString.append(", ").append(pComponent->getTransformationAnnotation());
      }
      // then get the icon transformations
      placementAnnotationString.append(", ").append(getTransformationAnnotation());
    } else if (mpGraphicsView->getViewType() == StringHandler::Diagram) {
      // first get the component from diagram view and get the transformations
      placementAnnotationString.append(", ").append(getTransformationAnnotation());
      // then get the icon transformations
      Component *pComponent;
      pComponent = mpGraphicsView->getModelWidget()->getIconGraphicsView()->getComponentObject(getName());
      if (pComponent) {
        placementAnnotationString.append(", ").append(pComponent->getTransformationAnnotation());
      }
    }
  } else {
    placementAnnotationString.append(", ").append(getTransformationAnnotation());
  }
  placementAnnotationString.append(")");
  return placementAnnotationString;
}

QString Component::getTransformationOrigin()
{
  // add the icon origin
  QString transformationOrigin;
  transformationOrigin.append("{").append(QString::number(mpTransformation->getOrigin().x())).append(",").append(QString::number(mpTransformation->getOrigin().y())).append("}");
  return transformationOrigin;
}

QString Component::getTransformationExtent()
{
  QString transformationExtent;
  // add extent points
  QPointF extent1 = mpTransformation->getExtent1();
  QPointF extent2 = mpTransformation->getExtent2();
  transformationExtent.append("{").append(QString::number(extent1.x()));
  transformationExtent.append(",").append(QString::number(extent1.y())).append(",");
  transformationExtent.append(QString::number(extent2.x())).append(",");
  transformationExtent.append(QString::number(extent2.y())).append("}");
  return transformationExtent;
}

void Component::applyRotation(qreal angle)
{
  setOriginAndExtents();
  mpTransformation->setRotateAngle(angle);
  setTransform(mpTransformation->getTransformationMatrix());
  showResizerItems();
}

void Component::addConnectionDetails(LineAnnotation *pConnectorLineAnnotation)
{
  // handle component position, rotation and scale changes
  connect(this, SIGNAL(transformChange()), pConnectorLineAnnotation, SLOT(handleComponentMoved()));
  connect(this, SIGNAL(rotationChange()), pConnectorLineAnnotation, SLOT(handleComponentRotation()));
  connect(this, SIGNAL(transformHasChanged()), pConnectorLineAnnotation, SLOT(updateConnectionAnnotation()));
}

void Component::emitAdded()
{
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpGraphicsView->getModelWidget()->getLibraryTreeItem()->handleIconUpdated();
  }
  emit added();
}

void Component::emitTransformHasChanged()
{
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpGraphicsView->getModelWidget()->getLibraryTreeItem()->handleIconUpdated();
  }
  emit transformHasChanged();
}

void Component::emitDeleted()
{
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpGraphicsView->getModelWidget()->getLibraryTreeItem()->handleIconUpdated();
  }
  emit deleted();
}

void Component::componentNameHasChanged()
{
  if (mIsInheritedComponent || mComponentType == Component::Port) {
    setToolTip(tr("<b>%1</b> %2<br /><br />Component declared in %3").arg(mpLibraryTreeItem->getNameStructure())
               .arg(mpComponentInfo->getName())
               .arg(mpReferenceComponent->getGraphicsView()->getModelWidget()->getLibraryTreeItem()->getNameStructure()));
  } else {
    setToolTip(tr("<b>%1</b> %2").arg(mpLibraryTreeItem->getNameStructure()).arg(mpComponentInfo->getName()));
  }
  emit displayTextChanged();
}

void Component::componentParameterHasChanged()
{
  emit displayTextChanged();
}

/*!
  Creates an object of ComponentParameters and uses it to read the parameters of the component.\n
  Returns the parameter string which can be either R=%R or %R.
  \param parameterString - the parameter string to look for.
  \return the parameter string with value.
  */
QString Component::getParameterDisplayString(QString parameterName)
{
  /*How to get the display value,
    1.  Check if the value is available in component modifier.
    2.  Check if the value is available in the component's class as a parameter or variable.
    3.  Find the value in extends classes and check if the value is present in extends modifier.
    3.3 If there is no extends modifier then finally check if value is present in extends classes.
    */
  OMCProxy *pOMCProxy = mpGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow()->getOMCProxy();
  QString displayString = "";
  QString modelName = mpGraphicsView->getModelWidget()->getLibraryTreeItem()->getNameStructure();
  /* case 1 */
  displayString = pOMCProxy->getComponentModifierValue(modelName, mpComponentInfo->getName() + "." + parameterName);
  /* case 2 */
  if (displayString.isEmpty()) {
    QList<ComponentInfo*> componentInfoList = pOMCProxy->getComponents(mpLibraryTreeItem->getNameStructure());
    foreach (ComponentInfo *pComponentInfo, componentInfoList) {
      if (pComponentInfo->getName().compare(parameterName) == 0) {
        displayString = pOMCProxy->getParameterValue(mpLibraryTreeItem->getNameStructure(), parameterName);
        break;
      }
    }
  }
  /* case 3 */
  if (displayString.isEmpty()) {
    foreach (Component *pInheritedComponent, mInheritanceList) {
      QList<ComponentInfo*> componentInfoList = pOMCProxy->getComponents(pInheritedComponent->getLibraryTreeItem()->getNameStructure());
      foreach (ComponentInfo *pComponentInfo, componentInfoList) {
        if (pComponentInfo->getName().compare(parameterName) == 0) {
          displayString = pOMCProxy->getExtendsModifierValue(mpLibraryTreeItem->getNameStructure(),
                                                              pInheritedComponent->getLibraryTreeItem()->getNameStructure(), parameterName);
          /* case 3.3 */
          if (displayString.isEmpty()) {
            displayString = pOMCProxy->getParameterValue(pInheritedComponent->getLibraryTreeItem()->getNameStructure(), parameterName);
          }
          break;
        }
      }
    }
  }
  return displayString;
}

void Component::shapeAdded()
{
  if (mComponentType == Component::Root) {
    mpDefaultComponentRectangle->setVisible(true);
    mpDefaultComponentText->setVisible(true);
  }
}

void Component::shapeDeleted()
{
  if (mComponentType == Component::Root) {
    GraphicsView *pGraphicsView = mpLibraryTreeItem->getModelWidget()->getIconGraphicsView();
    if (mpLibraryTreeItem->getRestriction() == StringHandler::Connector && mpGraphicsView->getViewType() == StringHandler::Diagram &&
        mComponentType == Component::Root && mpLibraryTreeItem->getModelWidget()->getDiagramGraphicsView()->hasAnnotation()) {
      pGraphicsView = mpLibraryTreeItem->getModelWidget()->getDiagramGraphicsView();
    }
    if (!pGraphicsView->hasAnnotation()) {
      mpDefaultComponentRectangle->setVisible(true);
      mpDefaultComponentText->setVisible(true);
    }
  }
}

void Component::addInterfacePoint(TLMInterfacePointInfo *pTLMInterfacePointInfo)
{
  // Add the Interfacepoint to the list.
  mInterfacePointsList.append(pTLMInterfacePointInfo);
}

void Component::removeInterfacePoint(TLMInterfacePointInfo *pTLMInterfacePointInfo)
{
  // remove the Interfacepoint from the list.
  mInterfacePointsList.removeOne(pTLMInterfacePointInfo);
}

void Component::renameInterfacePoint(TLMInterfacePointInfo *pTLMInterfacePointInfo, QString interfacePoint)
{
  // Set the new Interfacepoint.
  pTLMInterfacePointInfo->setInterfaceName(interfacePoint);
}

void Component::duplicateHelper(GraphicsView *pGraphicsView)
{
  Component *pComponent = pGraphicsView->getComponentsList().last();
  OMCProxy *pOMCProxy = mpGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow()->getOMCProxy();
  if (pComponent) {
    /* set the original component transformation to the duplicated one. */
    pComponent->getTransformation()->setExtent1(mpTransformation->getExtent1());
    pComponent->getTransformation()->setExtent2(mpTransformation->getExtent2());
    pComponent->getTransformation()->setRotateAngle(mpTransformation->getRotateAngle());
    pComponent->setTransform(pComponent->getTransformation()->getTransformationMatrix());
    /* get the original component attributes */
    QString className = pGraphicsView->getModelWidget()->getLibraryTreeItem()->getNameStructure();
    QList<ComponentInfo*> componentInfoList = pOMCProxy->getComponents(className);
    foreach (ComponentInfo *pComponentInfo, componentInfoList) {
      if (pComponentInfo->getName() == mpComponentInfo->getName()) {
        QString isFinal = pComponentInfo->getFinal() ? "true" : "false";
        QString isFlow = pComponentInfo->getFlow() ? "true" : "false";
        QString isProtected = pComponentInfo->getProtected() ? "true" : "false";
        QString isReplaceAble = pComponentInfo->getReplaceable() ? "true" : "false";
        QString variability = pComponentInfo->getVariablity();
        QString isInner = pComponentInfo->getInner() ? "true" : "false";
        QString isOuter = pComponentInfo->getOuter() ? "true" : "false";
        QString causality = pComponentInfo->getCausality();
        // update duplicated component attributes
        if (!pOMCProxy->setComponentProperties(className, pComponent->getName(), isFinal, isFlow, isProtected, isReplaceAble,
                                                variability, isInner, isOuter, causality)) {
          QMessageBox::critical(pGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow(),
                                QString(Helper::applicationName).append(" - ").append(Helper::error), pOMCProxy->getResult(),
                                Helper::ok);
          pOMCProxy->printMessagesStringInternal();
        }
        if (pOMCProxy->setComponentDimensions(className, pComponent->getName(), pComponentInfo->getArrayIndex())) {
          pComponent->getComponentInfo()->setArrayIndex(pComponentInfo->getArrayIndex());
        } else {
          QMessageBox::critical(pGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow(),
                                QString(Helper::applicationName).append(" - ").append(Helper::error), pOMCProxy->getResult(),
                                Helper::ok);
          pOMCProxy->printMessagesStringInternal();
        }
        break;
      }
    }
    /* get original component modifiers and apply them to duplicated one. */
    QStringList componentModifiersList = pOMCProxy->getComponentModifierNames(className, mpComponentInfo->getName());
    bool modifierValueChanged = false;
    foreach (QString componentModifier, componentModifiersList) {
      QString originalModifierName = QString(mpComponentInfo->getName()).append(".").append(componentModifier);
      QString duplicatedModifierName = QString(pComponent->getName()).append(".").append(componentModifier);
      if (pOMCProxy->setComponentModifierValue(className, duplicatedModifierName,
                                                pOMCProxy->getComponentModifierValue(className, originalModifierName).prepend("=")))
        modifierValueChanged = true;

    }
    if (modifierValueChanged) {
      pComponent->componentParameterHasChanged();
      pComponent->update();
    }
  }
}

void Component::removeShapes()
{
  foreach (ShapeAnnotation *pShapeAnnotation, mShapesList) {
    delete pShapeAnnotation;
  }
  mShapesList.clear();
  mpNonExistingComponentLine->setVisible(false);
  mpDefaultComponentRectangle->setVisible(false);
  mpDefaultComponentText->setVisible(false);
}

void Component::removeComponents()
{
  foreach (Component *pComponent, mComponentsList) {
    delete pComponent;
  }
  mComponentsList.clear();
}

void Component::updatePlacementAnnotation()
{
  // Add component annotation.
  LibraryTreeItem *pLibraryTreeItem = mpGraphicsView->getModelWidget()->getLibraryTreeItem();
  if (pLibraryTreeItem->getLibraryType()== LibraryTreeItem::TLM) {
    TLMEditor *pTLMEditor = dynamic_cast<TLMEditor*>(mpGraphicsView->getModelWidget()->getEditor());
    pTLMEditor->updateSubModelPlacementAnnotation(mpComponentInfo->getName(), getTransformation()->getVisible()? "true" : "false",
                                                  getTransformationOrigin(), getTransformationExtent(),
                                                  QString::number(getTransformation()->getRotateAngle()));
  } else {
    OMCProxy *pOMCProxy = mpGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow()->getOMCProxy();
    pOMCProxy->updateComponent(mpComponentInfo->getName(), mpLibraryTreeItem->getNameStructure(),
                               mpGraphicsView->getModelWidget()->getLibraryTreeItem()->getNameStructure(), getPlacementAnnotation());
    mpGraphicsView->getModelWidget()->updateModelicaText();
  }
  // set the model modified
  mpGraphicsView->getModelWidget()->setModelModified();
  /* When something is changed in the icon layer then update the LibraryTreeItem in the Library Browser */
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpGraphicsView->getModelWidget()->getLibraryTreeItem()->handleIconUpdated();
  }
}

/*!
 * \brief Component::updateOriginItem
 * Slot that updates the position of the component OriginItem.
 */
void Component::updateOriginItem()
{
  if (mpTransformation->hasOrigin()) {
    mpOriginItem->setPos(mpTransformation->getOrigin());
  }
}

void Component::handleLoaded()
{
  // clear all shapes & components
  removeShapes();
  removeComponents();
  if (mComponentType == Component::Port) {
    createClassShapes(mpLibraryTreeItem);
  } else {
    if (mpGraphicsView->getViewType() == StringHandler::Icon) {
      mpGraphicsView->addItem(this);
    }
    if (!mpLibraryTreeItem->getModelWidget() || mpLibraryTreeItem->getModelWidget()->isReloadNeeded()) {
      MainWindow *pMainWindow = mpGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow();
      pMainWindow->getLibraryWidget()->getLibraryTreeModel()->showModelWidget(mpLibraryTreeItem, "", false);
    }
    createClassInheritedShapes();
    createClassShapes(mpLibraryTreeItem);
    createClassInheritedComponents();
    createClassComponents(mpLibraryTreeItem);
    if (!mpLibraryTreeItem->getModelWidget()->getIconGraphicsView()->hasAnnotation()) {
      mpDefaultComponentRectangle->setVisible(true);
      mpDefaultComponentText->setVisible(true);
    }
  }
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpGraphicsView->getModelWidget()->getLibraryTreeItem()->handleIconUpdated();
  }
}

void Component::handleUnloaded()
{
  // clear all shapes & components
  removeShapes();
  removeComponents();
  // unload component based on type.
  if (mComponentType == Component::Port) {
    setVisible(false);
  } else {
    if (mpGraphicsView->getViewType() == StringHandler::Icon) {
      mpGraphicsView->removeItem(this);
    } else {
      // show a non exisitng component
      mpNonExistingComponentLine->setVisible(true);
    }
  }
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpGraphicsView->getModelWidget()->getLibraryTreeItem()->handleIconUpdated();
  }
}

void Component::handleShapeAdded()
{
  removeShapes();
  createClassInheritedShapes();
  createClassShapes(mpLibraryTreeItem);
}

/*!
 * \brief Component::referenceComponentAdded
 * Adds the referenced components when reference component is added.
 */
void Component::referenceComponentAdded()
{
  if (mComponentType == Component::Port) {
    setVisible(true);
  } else {
    mpGraphicsView->addItem(this);
    mpGraphicsView->addItem(mpOriginItem);
  }
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpGraphicsView->getModelWidget()->getLibraryTreeItem()->handleIconUpdated();
  }
}

/*!
 * \brief Component::referenceComponentChanged
 * Updates the referenced components when reference component is changed.
 */
void Component::referenceComponentChanged()
{
  Component *pComponent = qobject_cast<Component*>(sender());
  if (pComponent) {
    mpTransformation->updateTransformation(pComponent->getTransformation());
    setTransform(mpTransformation->getTransformationMatrix());
  }
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpGraphicsView->getModelWidget()->getLibraryTreeItem()->handleIconUpdated();
  }
}

/*!
 * \brief Component::referenceComponentDeleted
 * Delets the referenced components when reference component is deleted.
 */
void Component::referenceComponentDeleted()
{
  if (mComponentType == Component::Port) {
    setVisible(false);
  } else {
    mpGraphicsView->removeItem(this);
    mpGraphicsView->removeItem(mpOriginItem);
  }
  if (mpGraphicsView->getViewType() == StringHandler::Icon) {
    mpGraphicsView->getModelWidget()->getLibraryTreeItem()->handleIconUpdated();
  }
}

void Component::prepareResizeComponent(ResizerItem *pResizerItem)
{
  mpSelectedResizerItem = pResizerItem;
  mTransform = transform();
  mSceneBoundingRect = sceneBoundingRect();
  QPointF topLeft = sceneBoundingRect().topLeft();
  QPointF topRight = sceneBoundingRect().topRight();
  QPointF bottomLeft = sceneBoundingRect().bottomLeft();
  QPointF bottomRight = sceneBoundingRect().bottomRight();
  mTransformationStartPosition = scenePos();
  mPivotPoint = sceneBoundingRect().center();

  if (mpSelectedResizerItem->getResizePosition() == ResizerItem::BottomLeft) {
    mTransformationStartPosition = topLeft;
    mPivotPoint = bottomRight;
  } else if (mpSelectedResizerItem->getResizePosition() == ResizerItem::TopLeft) {
    mTransformationStartPosition = bottomLeft;
    mPivotPoint = topRight;
  } else if (mpSelectedResizerItem->getResizePosition() == ResizerItem::TopRight) {
    mTransformationStartPosition = bottomRight;
    mPivotPoint = topLeft;
  } else if (mpSelectedResizerItem->getResizePosition() == ResizerItem::BottomRight) {
    mTransformationStartPosition = topRight;
    mPivotPoint = bottomLeft;
  }
  mpResizerRectangle->setRect(boundingRect()); //Sets the current item to the temporary rect
  mpResizerRectangle->setTransform(transform()); //Set the same matrix of this item to the temporary item
  mpResizerRectangle->setPos(pos());
}

void Component::resizeComponent(QPointF newPosition)
{
  float xDistance; //X distance between the current position of the mouse and the starting position mouse
  float yDistance; //Y distance between the current position of the mouse and the starting position mouse
  //Calculates the X distance
  xDistance = newPosition.x() - mTransformationStartPosition.x();
  //If the starting point is on the negative side of the X plane we do an inverse of the value
  if (mTransformationStartPosition.x() < mPivotPoint.x()) {
    xDistance = xDistance * -1;
  }
  //Calculates the Y distance
  yDistance = newPosition.y() - mTransformationStartPosition.y();
  //If the starting point is on the negative side of the Y plane we do an inverse of the value
  if (mTransformationStartPosition.y() < mPivotPoint.y()) {
    yDistance = yDistance * -1;
  }
  //Calculate the factors by dividing the distances againts the original size of this container
  mXFactor = 0;
  mYFactor = 0;
  mXFactor = xDistance / mSceneBoundingRect.width();
  mYFactor = yDistance / mSceneBoundingRect.height();
  mXFactor = 1 + mXFactor;
  mYFactor = 1 + mYFactor;
  // if preserveAspectRatio is true then resize equally
  if (mpCoOrdinateSystem->getPreserveAspectRatio()) {
    qreal factor = qMax(fabs(mXFactor), fabs(mYFactor));
    mXFactor = mXFactor < 0 ? mXFactor = factor * -1 : mXFactor = factor;
    mYFactor = mYFactor < 0 ? mYFactor = factor * -1 : mYFactor = factor;
  }
  // Apply the transformation to the temporary polygon using the new scaling factors
  QPointF pivot = mPivotPoint - pos();
  // Creates a temporaty transformation
  QTransform tmpTransform = QTransform().translate(pivot.x(), pivot.y()).rotate(0)
      .scale(mXFactor, mYFactor)
      .translate(-pivot.x(), -pivot.y());
  mpResizerRectangle->setTransform(mTransform * tmpTransform); //Multiplies the previous transform * the temporary
  setTransform(mTransform * tmpTransform);
  // set the final resize on component.
  QPointF extent1, extent2;
  qreal sx, sy;
  getScale(&sx, &sy);
  extent1.setX(sx * boundingRect().left());
  extent1.setY(sy * boundingRect().top());
  extent2.setX(sx * boundingRect().right());
  extent2.setY(sy * boundingRect().bottom());
  mpTransformation->setOrigin(scenePos());
  mpTransformation->setExtent1(extent1);
  mpTransformation->setExtent2(extent2);
  setTransform(mpTransformation->getTransformationMatrix());
  // let connections know that component has changed.
  emit transformChange();
}

void Component::finishResizeComponent()
{
  if (isSelected()) {
    showResizerItems();
  } else {
    setSelected(true);
  }
}

/*!
 * \brief Component::deleteMe
 * Deletes the Component from the current view.
 */
void Component::deleteMe()
{
  // delete the component from model
  mpGraphicsView->deleteComponent(this);
}

void Component::duplicate()
{
  QPointF gridStep(mpGraphicsView->getCoOrdinateSystem()->getHorizontalGridStep(),
                   mpGraphicsView->getCoOrdinateSystem()->getVerticalGridStep());
  if (mpGraphicsView->addComponent(mpLibraryTreeItem->getNameStructure(), scenePos() + gridStep)) {
    if (mpLibraryTreeItem->getRestriction() == StringHandler::Connector) {
      if (mpGraphicsView->getViewType() == StringHandler::Diagram) {
        duplicateHelper(mpGraphicsView);
        duplicateHelper(mpGraphicsView->getModelWidget()->getIconGraphicsView());
      } else {
        duplicateHelper(mpGraphicsView);
        duplicateHelper(mpGraphicsView->getModelWidget()->getDiagramGraphicsView());
      }
    } else {
      duplicateHelper(mpGraphicsView);
    }
  }
}

void Component::rotateClockwise()
{
  qreal oldRotation = StringHandler::getNormalizedAngle(mpTransformation->getRotateAngle());
  qreal rotateIncrement = -90;
  qreal angle = oldRotation + rotateIncrement;
  applyRotation(angle);
  emit rotationChange();
}

void Component::rotateAntiClockwise()
{
  qreal oldRotation = StringHandler::getNormalizedAngle(mpTransformation->getRotateAngle());
  qreal rotateIncrement = 90;
  qreal angle = oldRotation + rotateIncrement;
  applyRotation(angle);
  emit rotationChange();
}

/*!
 * \brief Component::flipHorizontal
 * Flips the component horizontally.
 */
void Component::flipHorizontal()
{
  setOriginAndExtents();
  QPointF extent1 = mpTransformation->getExtent1();
  QPointF extent2 = mpTransformation->getExtent2();
  // do the flipping based on the component angle.
  qreal angle = StringHandler::getNormalizedAngle(mpTransformation->getRotateAngle());
  if ((angle >= 0 && angle < 90) || (angle >= 180 && angle < 270)) {
    mpTransformation->setExtent1(QPointF(extent2.x(), extent1.y()));
    mpTransformation->setExtent2(QPointF(extent1.x(), extent2.y()));
  } else if ((angle >= 90 && angle < 180) || (angle >= 270 && angle < 360)) {
    mpTransformation->setExtent1(QPointF(extent1.x(), extent2.y()));
    mpTransformation->setExtent2(QPointF(extent2.x(), extent1.y()));
  }
  setTransform(mpTransformation->getTransformationMatrix());
  emit rotationChange();
  emit transformHasChanged();
  showResizerItems();
}

/*!
 * \brief Component::flipVertical
 * Flips the component vertically.
 */
void Component::flipVertical()
{
  setOriginAndExtents();
  QPointF extent1 = mpTransformation->getExtent1();
  QPointF extent2 = mpTransformation->getExtent2();
  // do the flipping based on the component angle.
  qreal angle = StringHandler::getNormalizedAngle(mpTransformation->getRotateAngle());
  if ((angle >= 0 && angle < 90) || (angle >= 180 && angle < 270)) {
    mpTransformation->setExtent1(QPointF(extent1.x(), extent2.y()));
    mpTransformation->setExtent2(QPointF(extent2.x(), extent1.y()));
  } else if ((angle >= 90 && angle < 180) || (angle >= 270 && angle < 360)) {
    mpTransformation->setExtent1(QPointF(extent2.x(), extent1.y()));
    mpTransformation->setExtent2(QPointF(extent1.x(), extent2.y()));
  }
  setTransform(mpTransformation->getTransformationMatrix());
  emit rotationChange();
  emit transformHasChanged();
  showResizerItems();
}

/*!
  Slot that moves component upwards depending on the grid step size value
  \sa moveDown(),
      moveLeft(),
      moveRight(),
      moveShiftUp(),
      moveShiftDown(),
      moveShiftLeft(),
      moveShiftRight(),
      moveCtrlUp(),
      moveCtrlDown(),
      moveCtrlLeft(),
      moveCtrlRight()
  */
void Component::moveUp()
{
  mpTransformation->adjustPosition(0, mpGraphicsView->getCoOrdinateSystem()->getVerticalGridStep());
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component upwards depending on the grid step size value multiplied by 5
  \sa moveUp(),
      moveDown(),
      moveLeft(),
      moveRight(),
      moveShiftDown(),
      moveShiftLeft(),
      moveShiftRight(),
      moveCtrlUp(),
      moveCtrlDown(),
      moveCtrlLeft(),
      moveCtrlRight()
  */
void Component::moveShiftUp()
{
  mpTransformation->adjustPosition(0, mpGraphicsView->getCoOrdinateSystem()->getVerticalGridStep() * 5);
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component one pixel upwards
  \sa moveUp(),
      moveDown(),
      moveLeft(),
      moveRight(),
      moveShiftUp(),
      moveShiftDown(),
      moveShiftLeft(),
      moveShiftRight(),
      moveCtrlDown(),
      moveCtrlLeft(),
      moveCtrlRight()
  */
void Component::moveCtrlUp()
{
  mpTransformation->adjustPosition(0, 1);
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component downwards depending on the grid step size value
  \sa moveUp(),
      moveLeft(),
      moveRight(),
      moveShiftUp(),
      moveShiftDown(),
      moveShiftLeft(),
      moveShiftRight(),
      moveCtrlUp(),
      moveCtrlDown(),
      moveCtrlLeft(),
      moveCtrlRight()
  */
void Component::moveDown()
{
  mpTransformation->adjustPosition(0, -mpGraphicsView->getCoOrdinateSystem()->getVerticalGridStep());
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component downwards depending on the grid step size value multiplied by 5
  \sa moveUp(),
      moveDown(),
      moveLeft(),
      moveRight(),
      moveShiftUp(),
      moveShiftLeft(),
      moveShiftRight(),
      moveCtrlUp(),
      moveCtrlDown(),
      moveCtrlLeft(),
      moveCtrlRight()
  */
void Component::moveShiftDown()
{
  mpTransformation->adjustPosition(0, -(mpGraphicsView->getCoOrdinateSystem()->getVerticalGridStep() * 5));
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component one pixel downwards
  \sa moveUp(),
      moveDown(),
      moveLeft(),
      moveRight(),
      moveShiftUp(),
      moveShiftDown(),
      moveShiftLeft(),
      moveShiftRight(),
      moveCtrlUp(),
      moveCtrlLeft(),
      moveCtrlRight()
  */
void Component::moveCtrlDown()
{
  mpTransformation->adjustPosition(0, -1);
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component leftwards depending on the grid step size value
  \sa moveUp(),
      moveDown(),
      moveRight(),
      moveShiftUp(),
      moveShiftDown(),
      moveShiftLeft(),
      moveShiftRight(),
      moveCtrlUp(),
      moveCtrlDown(),
      moveCtrlLeft(),
      moveCtrlRight()
  */
void Component::moveLeft()
{
  mpTransformation->adjustPosition(-mpGraphicsView->getCoOrdinateSystem()->getHorizontalGridStep(), 0);
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component leftwards depending on the grid step size value multiplied by 5
  \sa moveUp(),
      moveDown(),
      moveLeft(),
      moveRight(),
      moveShiftUp(),
      moveShiftDown(),
      moveShiftRight(),
      moveCtrlUp(),
      moveCtrlDown(),
      moveCtrlLeft(),
      moveCtrlRight()
  */
void Component::moveShiftLeft()
{
  mpTransformation->adjustPosition(-(mpGraphicsView->getCoOrdinateSystem()->getHorizontalGridStep() * 5), 0);
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component one pixel leftwards
  \sa moveUp(),
      moveDown(),
      moveLeft(),
      moveRight(),
      moveShiftUp(),
      moveShiftDown(),
      moveShiftLeft(),
      moveShiftRight(),
      moveCtrlUp(),
      moveCtrlDown(),
      moveCtrlRight()
  */
void Component::moveCtrlLeft()
{
  mpTransformation->adjustPosition(-1, 0);
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component rightwards depending on the grid step size value
  \sa moveUp(),
      moveDown(),
      moveLeft(),
      moveShiftUp(),
      moveShiftDown(),
      moveShiftLeft(),
      moveShiftRight(),
      moveCtrlUp(),
      moveCtrlDown(),
      moveCtrlLeft(),
      moveCtrlRight()
  */
void Component::moveRight()
{
  mpTransformation->adjustPosition(mpGraphicsView->getCoOrdinateSystem()->getHorizontalGridStep(), 0);
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component rightwards depending on the grid step size value multiplied by 5
  \sa moveUp(),
      moveDown(),
      moveLeft(),
      moveRight(),
      moveShiftUp(),
      moveShiftDown(),
      moveShiftLeft(),
      moveCtrlUp(),
      moveCtrlDown(),
      moveCtrlLeft(),
      moveCtrlRight()
  */
void Component::moveShiftRight()
{
  mpTransformation->adjustPosition(mpGraphicsView->getCoOrdinateSystem()->getHorizontalGridStep() * 5, 0);
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

/*!
  Slot that moves component one pixel rightwards
  \sa moveUp(),
      moveDown(),
      moveLeft(),
      moveRight(),
      moveShiftUp(),
      moveShiftDown(),
      moveShiftLeft(),
      moveShiftRight(),
      moveCtrlUp(),
      moveCtrlDown(),
      moveCtrlLeft()
  */
void Component::moveCtrlRight()
{
  mpTransformation->adjustPosition(1, 0);
  setTransform(mpTransformation->getTransformationMatrix());
  emit transformChange();
}

//! Slot that opens up the component parameters dialog.
//! @see showAttributes()
void Component::showParameters()
{
  MainWindow *pMainWindow = mpGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow();
  pMainWindow->getStatusBar()->showMessage(tr("Opening %1 %2 parameters window").arg(mpLibraryTreeItem->getNameStructure())
                                           .arg(mpComponentInfo->getName()));
  pMainWindow->getProgressBar()->setRange(0, 0);
  pMainWindow->showProgressBar();
  ComponentParameters *pComponentParameters = new ComponentParameters(this, pMainWindow);
  pMainWindow->hideProgressBar();
  pMainWindow->getStatusBar()->clearMessage();
  pComponentParameters->exec();
}

//! Slot that opens up the component attributes dialog.
//! @see showParameters()
void Component::showAttributes()
{
  MainWindow *pMainWindow = mpGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow();
  pMainWindow->getStatusBar()->showMessage(tr("Opening %1 %2 attributes window").arg(mpLibraryTreeItem->getNameStructure())
                                           .arg(mpComponentInfo->getName()));
  pMainWindow->getProgressBar()->setRange(0, 0);
  pMainWindow->showProgressBar();
  ComponentAttributes *pComponentAttributes = new ComponentAttributes(this, pMainWindow);
  pMainWindow->hideProgressBar();
  pMainWindow->getStatusBar()->clearMessage();
  pComponentAttributes->exec();
}

//! Slot that opens up the component Modelica class in a new tab/window.
void Component::viewClass()
{
  MainWindow *pMainWindow = mpGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow();
  pMainWindow->getLibraryWidget()->openLibraryTreeItem(mpLibraryTreeItem->getNameStructure());
}

//! Slot that opens up the component Modelica class in a documentation view.
void Component::viewDocumentation()
{
  MainWindow *pMainWindow = mpGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow();
  pMainWindow->getDocumentationWidget()->showDocumentation(mpLibraryTreeItem->getNameStructure());
  pMainWindow->getDocumentationDockWidget()->show();
}

/*!
 * \brief Component::showTLMAttributes
 * Slot that opens up the SubModelAttributes Dialog.
 */
void Component::showTLMAttributes()
{
  MainWindow *pMainWindow = mpGraphicsView->getModelWidget()->getModelWidgetContainer()->getMainWindow();
  SubModelAttributes *pTLMComponentAttributes = new SubModelAttributes(this, pMainWindow);
  pTLMComponentAttributes->exec();
}

/*! Event when mouse is double clicked on a component.
 *  Shows the component properties dialog.
 */
void Component::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
  Q_UNUSED(event);
  LibraryTreeItem *pLibraryTreeItem = mpGraphicsView->getModelWidget()->getLibraryTreeItem();
  if(pLibraryTreeItem->getLibraryType()== LibraryTreeItem::TLM) {
    emit showTLMAttributes();
  } else {
    if (!mpParentComponent) { // if root component is double clicked then show parameters.
      mpGraphicsView->removeConnection();
      emit showParameters();
    }
  }
}

void Component::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  Component *pComponent = getRootParentComponent();
  if (pComponent->isSelected()) {
    pComponent->showResizerItems();
  } else {
    // unselect all items
    foreach (QGraphicsItem *pItem, mpGraphicsView->items()) {
      pItem->setSelected(false);
    }
    pComponent->setSelected(true);
  }
  QMenu menu(mpGraphicsView);
  menu.addAction(pComponent->getParametersAction());
  menu.addAction(pComponent->getAttributesAction());
  menu.addSeparator();
  menu.addAction(pComponent->getViewClassAction());
  menu.addAction(pComponent->getViewDocumentationAction());
  menu.addSeparator();
  LibraryTreeItem *pLibraryTreeItem = mpGraphicsView->getModelWidget()->getLibraryTreeItem();
  if (pLibraryTreeItem) {
    QMenu menu(mpGraphicsView);
    switch (pLibraryTreeItem->getLibraryType()) {
      case LibraryTreeItem::Modelica:
      default:
        menu.addAction(pComponent->getParametersAction());
        menu.addAction(pComponent->getAttributesAction());
        menu.addSeparator();
        menu.addAction(pComponent->getViewClassAction());
        menu.addAction(pComponent->getViewDocumentationAction());
        menu.addSeparator();
        if (pComponent->isInheritedComponent()) {
          mpGraphicsView->getDeleteAction()->setDisabled(true);
          mpGraphicsView->getDuplicateAction()->setDisabled(true);
          mpGraphicsView->getRotateClockwiseAction()->setDisabled(true);
          mpGraphicsView->getRotateAntiClockwiseAction()->setDisabled(true);
          mpGraphicsView->getFlipHorizontalAction()->setDisabled(true);
          mpGraphicsView->getFlipVerticalAction()->setDisabled(true);
        }
        menu.addAction(mpGraphicsView->getDeleteAction());
        menu.addAction(mpGraphicsView->getDuplicateAction());
        menu.addSeparator();
        menu.addAction(mpGraphicsView->getRotateClockwiseAction());
        menu.addAction(mpGraphicsView->getRotateAntiClockwiseAction());
        menu.addAction(mpGraphicsView->getFlipHorizontalAction());
        menu.addAction(mpGraphicsView->getFlipVerticalAction());
        break;
      case LibraryTreeItem::TLM:
        menu.addAction(pComponent->getTLMAttributesAction());
        break;
    }
    menu.exec(event->screenPos());
  }
}

QVariant Component::itemChange(GraphicsItemChange change, const QVariant &value)
{
  QGraphicsItem::itemChange(change, value);
  if (change == QGraphicsItem::ItemSelectedHasChanged) {
    if (isSelected()) {
      showResizerItems();
      setCursor(Qt::SizeAllCursor);
      // Only allow manipulations on component if the class is not a system library class OR component is not an inherited component.
      if (!mpGraphicsView->getModelWidget()->getLibraryTreeItem()->isSystemLibrary() && !isInheritedComponent()) {
        connect(mpGraphicsView, SIGNAL(mouseDelete()), this, SLOT(deleteMe()), Qt::UniqueConnection);
        connect(mpGraphicsView->getDuplicateAction(), SIGNAL(triggered()), this, SLOT(duplicate()), Qt::UniqueConnection);
        connect(mpGraphicsView->getRotateClockwiseAction(), SIGNAL(triggered()), this, SLOT(rotateClockwise()), Qt::UniqueConnection);
        connect(mpGraphicsView->getRotateClockwiseAction(), SIGNAL(triggered()), this, SIGNAL(transformHasChanged()), Qt::UniqueConnection);
        connect(mpGraphicsView->getRotateAntiClockwiseAction(), SIGNAL(triggered()), this, SLOT(rotateAntiClockwise()), Qt::UniqueConnection);
        connect(mpGraphicsView->getRotateAntiClockwiseAction(), SIGNAL(triggered()), this, SIGNAL(transformHasChanged()), Qt::UniqueConnection);
        connect(mpGraphicsView->getFlipHorizontalAction(), SIGNAL(triggered()), this, SLOT(flipHorizontal()), Qt::UniqueConnection);
        connect(mpGraphicsView->getFlipVerticalAction(), SIGNAL(triggered()), this, SLOT(flipVertical()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressDelete()), this, SLOT(deleteMe()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressDuplicate()), this, SLOT(duplicate()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressRotateClockwise()), this, SLOT(rotateClockwise()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressRotateAntiClockwise()), this, SLOT(rotateAntiClockwise()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressUp()), this, SLOT(moveUp()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressShiftUp()), this, SLOT(moveShiftUp()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressCtrlUp()), this, SLOT(moveCtrlUp()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressDown()), this, SLOT(moveDown()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressShiftDown()), this, SLOT(moveShiftDown()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressCtrlDown()), this, SLOT(moveCtrlDown()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressLeft()), this, SLOT(moveLeft()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressShiftLeft()), this, SLOT(moveShiftLeft()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressCtrlLeft()), this, SLOT(moveCtrlLeft()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressRight()), this, SLOT(moveRight()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressShiftRight()), this, SLOT(moveShiftRight()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyPressCtrlRight()), this, SLOT(moveCtrlRight()), Qt::UniqueConnection);
        connect(mpGraphicsView, SIGNAL(keyRelease()), this, SIGNAL(transformHasChanged()), Qt::UniqueConnection);
      }
    } else {
      if (!mpBottomLeftResizerItem->isPressed() && !mpTopLeftResizerItem->isPressed() &&
          !mpTopRightResizerItem->isPressed() && !mpBottomRightResizerItem->isPressed()) {
        hideResizerItems();
      }
      /* Always hide ResizerItem's for system library class and inherited components. */
      if (mpGraphicsView->getModelWidget()->getLibraryTreeItem()->isSystemLibrary() || isInheritedComponent()) {
        hideResizerItems();
      }
      unsetCursor();
      /* Only allow manipulations on component if the class is not a system library class OR component is not an inherited component. */
      if (!mpGraphicsView->getModelWidget()->getLibraryTreeItem()->isSystemLibrary() && !isInheritedComponent()) {
        disconnect(mpGraphicsView, SIGNAL(mouseDelete()), this, SLOT(deleteMe()));
        disconnect(mpGraphicsView->getDuplicateAction(), SIGNAL(triggered()), this, SLOT(duplicate()));
        disconnect(mpGraphicsView->getRotateClockwiseAction(), SIGNAL(triggered()), this, SLOT(rotateClockwise()));
        disconnect(mpGraphicsView->getRotateClockwiseAction(), SIGNAL(triggered()), this, SIGNAL(transformHasChanged()));
        disconnect(mpGraphicsView->getRotateAntiClockwiseAction(), SIGNAL(triggered()), this, SLOT(rotateAntiClockwise()));
        disconnect(mpGraphicsView->getRotateAntiClockwiseAction(), SIGNAL(triggered()), this, SIGNAL(transformHasChanged()));
        disconnect(mpGraphicsView->getFlipHorizontalAction(), SIGNAL(triggered()), this, SLOT(flipHorizontal()));
        disconnect(mpGraphicsView->getFlipVerticalAction(), SIGNAL(triggered()), this, SLOT(flipVertical()));
        disconnect(mpGraphicsView, SIGNAL(keyPressDelete()), this, SLOT(deleteMe()));
        disconnect(mpGraphicsView, SIGNAL(keyPressDuplicate()), this, SLOT(duplicate()));
        disconnect(mpGraphicsView, SIGNAL(keyPressRotateClockwise()), this, SLOT(rotateClockwise()));
        disconnect(mpGraphicsView, SIGNAL(keyPressRotateAntiClockwise()), this, SLOT(rotateAntiClockwise()));
        disconnect(mpGraphicsView, SIGNAL(keyPressUp()), this, SLOT(moveUp()));
        disconnect(mpGraphicsView, SIGNAL(keyPressShiftUp()), this, SLOT(moveShiftUp()));
        disconnect(mpGraphicsView, SIGNAL(keyPressCtrlUp()), this, SLOT(moveCtrlUp()));
        disconnect(mpGraphicsView, SIGNAL(keyPressDown()), this, SLOT(moveDown()));
        disconnect(mpGraphicsView, SIGNAL(keyPressShiftDown()), this, SLOT(moveShiftDown()));
        disconnect(mpGraphicsView, SIGNAL(keyPressCtrlDown()), this, SLOT(moveCtrlDown()));
        disconnect(mpGraphicsView, SIGNAL(keyPressLeft()), this, SLOT(moveLeft()));
        disconnect(mpGraphicsView, SIGNAL(keyPressShiftLeft()), this, SLOT(moveShiftLeft()));
        disconnect(mpGraphicsView, SIGNAL(keyPressCtrlLeft()), this, SLOT(moveCtrlLeft()));
        disconnect(mpGraphicsView, SIGNAL(keyPressRight()), this, SLOT(moveRight()));
        disconnect(mpGraphicsView, SIGNAL(keyPressShiftRight()), this, SLOT(moveShiftRight()));
        disconnect(mpGraphicsView, SIGNAL(keyPressCtrlRight()), this, SLOT(moveCtrlRight()));
        disconnect(mpGraphicsView, SIGNAL(keyRelease()), this, SIGNAL(transformHasChanged()));
      }
    }
  } else if (change == QGraphicsItem::ItemPositionHasChanged) {
    emit transformChange();
  }
  else if (change == QGraphicsItem::ItemPositionChange) {
    // move by grid distance while dragging component
    QPointF positionDifference = mpGraphicsView->movePointByGrid(value.toPointF() - pos());
    return pos() + positionDifference;
  }
  return value;
}
