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

#include "OptionsDialog.h"
#include <limits>

//! @class OptionsDialog
//! @brief Creates an interface with options like Modelica Text, Pen Styles, Libraries etc.

//! Constructor
//! @param pParent is the pointer to MainWindow
OptionsDialog::OptionsDialog(MainWindow *pParent)
  : QDialog(pParent, Qt::WindowTitleHint), mpSettings(OpenModelica::getApplicationSettings())
{
  setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::options));
  setModal(true);
  mpMainWindow = pParent;
  mpGeneralSettingsPage = new GeneralSettingsPage(this);
  mpLibrariesPage = new LibrariesPage(this);
  mpModelicaTextSettings = new ModelicaTextSettings();
  mpModelicaTextEditorPage = new ModelicaTextEditorPage(this);
  mpGraphicalViewsPage = new GraphicalViewsPage(this);
  mpSimulationPage = new SimulationPage(this);
  mpNotificationsPage = new NotificationsPage(this);
  mpLineStylePage = new LineStylePage(this);
  mpFillStylePage = new FillStylePage(this);
  mpCurveStylePage = new CurveStylePage(this);
  mpFigaroPage = new FigaroPage(this);
  mpDebuggerPage = new DebuggerPage(this);
  // get the settings
  readSettings();
  // set up the Options Dialog
  setUpDialog();
}

OptionsDialog::~OptionsDialog()
{
  delete mpModelicaTextSettings;
}

//! Reads the settings from omedit.ini file.
void OptionsDialog::readSettings()
{
  mpSettings->sync();
  readGeneralSettings();
  readLibrariesSettings();
  readModelicaTextSettings();
  mpModelicaTextEditorPage->initializeFields();
  emit modelicaTextSettingsChanged();
  readGraphicalViewsSettings();
  readSimulationSettings();
  readNotificationsSettings();
  readLineStyleSettings();
  readFillStyleSettings();
  readCurveStyleSettings();
  readFigaroSettings();
  readDebuggerSettings();
}

//! Reads the General section settings from omedit.ini
void OptionsDialog::readGeneralSettings()
{
  // read the language option
  if (mpSettings->contains("language"))
  {
    /* Handle locale stored both as variant and as QString */
    QLocale locale = QLocale(mpSettings->value("language").toString());
    int currentIndex = mpGeneralSettingsPage->getLanguageComboBox()->findData(locale.name() == "C" ? mpSettings->value("language") : QVariant(locale), Qt::UserRole, Qt::MatchExactly);
    if (currentIndex > -1) {
      mpGeneralSettingsPage->getLanguageComboBox()->setCurrentIndex(currentIndex);
    }
  }
  // read the working directory
  if (mpSettings->contains("workingDirectory"))
    mpMainWindow->getOMCProxy()->changeDirectory(mpSettings->value("workingDirectory").toString());
  mpGeneralSettingsPage->setWorkingDirectory(mpMainWindow->getOMCProxy()->changeDirectory());
  // read the user customizations
  if (mpSettings->contains("userCustomizations"))
    mpGeneralSettingsPage->setPreserveUserCustomizations(mpSettings->value("userCustomizations").toBool());
  // read show protected classes
  if (mpSettings->contains("showProtectedClasses"))
    mpGeneralSettingsPage->setShowProtectedClasses(mpSettings->value("showProtectedClasses").toBool());
  // read the modeling view mode
  if (mpSettings->contains("modeling/viewmode"))
    mpGeneralSettingsPage->setModelingViewMode(mpSettings->value("modeling/viewmode").toString());
  // read the plotting view mode
  if (mpSettings->contains("plotting/viewmode"))
    mpGeneralSettingsPage->setPlottingViewMode(mpSettings->value("plotting/viewmode").toString());
  // read the default view
  if (mpSettings->contains("defaultView"))
    mpGeneralSettingsPage->setDefaultView(mpSettings->value("defaultView").toString());
  // read auto save
  if (mpSettings->contains("autoSave/enable"))
    mpGeneralSettingsPage->getEnableAutoSaveGroupBox()->setChecked(mpSettings->value("autoSave/enable").toBool());
  if (mpSettings->contains("autoSave/interval"))
    mpGeneralSettingsPage->getAutoSaveIntervalSpinBox()->setValue(mpSettings->value("autoSave/interval").toInt());
  if (mpSettings->contains("autoSave/enableSingleClasses"))
    mpGeneralSettingsPage->getEnableAutoSaveForSingleClassesCheckBox()->setChecked(mpSettings->value("autoSave/enableSingleClasses").toBool());
  if (mpSettings->contains("autoSave/enableOneFilePackages"))
    mpGeneralSettingsPage->getEnableAutoSaveForOneFilePackagesCheckBox()->setChecked(mpSettings->value("autoSave/enableOneFilePackages").toBool());
  // read welcome page
  if (mpSettings->contains("welcomePage/view"))
    mpGeneralSettingsPage->setWelcomePageView(mpSettings->value("welcomePage/view").toInt());
  if (mpSettings->contains("welcomePage/showLatestNews"))
    mpGeneralSettingsPage->getShowLatestNewsCheckBox()->setChecked(mpSettings->value("welcomePage/showLatestNews").toBool());
}

//! Reads the Libraries section settings from omedit.ini
void OptionsDialog::readLibrariesSettings()
{
  // read the system libraries
  int i = 0;
  while(i < mpLibrariesPage->getSystemLibrariesTree()->topLevelItemCount())
  {
    qDeleteAll(mpLibrariesPage->getSystemLibrariesTree()->topLevelItem(i)->takeChildren());
    delete mpLibrariesPage->getSystemLibrariesTree()->topLevelItem(i);
    i = 0;   //Restart iteration
  }
  // read the settings and add libraries
  mpSettings->beginGroup("libraries");
  QStringList systemLibraries = mpSettings->childKeys();
  foreach (QString systemLibrary, systemLibraries)
  {
    QStringList values;
    values << systemLibrary << mpSettings->value(systemLibrary).toString();
    mpLibrariesPage->getSystemLibrariesTree()->addTopLevelItem(new QTreeWidgetItem(values));
  }
  mpSettings->endGroup();
  // read the forceModelicaLoad
  if (mpSettings->contains("forceModelicaLoad"))
    mpLibrariesPage->getForceModelicaLoadCheckBox()->setChecked(mpSettings->value("forceModelicaLoad").toBool());
  // read user libraries
  i = 0;
  while(i < mpLibrariesPage->getUserLibrariesTree()->topLevelItemCount())
  {
    qDeleteAll(mpLibrariesPage->getUserLibrariesTree()->topLevelItem(i)->takeChildren());
    delete mpLibrariesPage->getUserLibrariesTree()->topLevelItem(i);
    i = 0;   //Restart iteration
  }
  // read the settings and add libraries
  mpSettings->beginGroup("userlibraries");
  QStringList userLibraries = mpSettings->childKeys();
  foreach (QString userLibrary, userLibraries)
  {
    QStringList values;
    values << QUrl::fromPercentEncoding(QByteArray(userLibrary.toStdString().c_str())) << mpSettings->value(userLibrary).toString();
    mpLibrariesPage->getUserLibrariesTree()->addTopLevelItem(new QTreeWidgetItem(values));
  }
  mpSettings->endGroup();
}

//! Reads the ModelicaText settings from omedit.ini
void OptionsDialog::readModelicaTextSettings()
{
  if (mpSettings->contains("textEditor/enableSyntaxHighlighting"))
    mpModelicaTextEditorPage->getSyntaxHighlightingCheckbox()->setChecked(mpSettings->value("textEditor/enableSyntaxHighlighting").toBool());
  if (mpSettings->contains("textEditor/enableLineWrapping"))
    mpModelicaTextEditorPage->getLineWrappingCheckbox()->setChecked(mpSettings->value("textEditor/enableLineWrapping").toBool());
  if (mpSettings->contains("textEditor/fontFamily"))
    mpModelicaTextSettings->setFontFamily(mpSettings->value("textEditor/fontFamily").toString());
  if (mpSettings->contains("textEditor/fontSize"))
    mpModelicaTextSettings->setFontSize(mpSettings->value("textEditor/fontSize").toDouble());
  if (mpSettings->contains("textEditor/textRuleColor"))
    mpModelicaTextSettings->setTextRuleColor(QColor(mpSettings->value("textEditor/textRuleColor").toUInt()));
  if (mpSettings->contains("textEditor/keywordRuleColor"))
    mpModelicaTextSettings->setKeywordRuleColor(QColor(mpSettings->value("textEditor/keywordRuleColor").toUInt()));
  if (mpSettings->contains("textEditor/typeRuleColor"))
    mpModelicaTextSettings->setTypeRuleColor(QColor(mpSettings->value("textEditor/typeRuleColor").toUInt()));
  if (mpSettings->contains("textEditor/functionRuleColor"))
    mpModelicaTextSettings->setFunctionRuleColor(QColor(mpSettings->value("textEditor/functionRuleColor").toUInt()));
  if (mpSettings->contains("textEditor/quotesRuleColor"))
    mpModelicaTextSettings->setQuotesRuleColor(QColor(mpSettings->value("textEditor/quotesRuleColor").toUInt()));
  if (mpSettings->contains("textEditor/commentRuleColor"))
    mpModelicaTextSettings->setCommentRuleColor(QColor(mpSettings->value("textEditor/commentRuleColor").toUInt()));
  if (mpSettings->contains("textEditor/numberRuleColor"))
    mpModelicaTextSettings->setNumberRuleColor(QColor(mpSettings->value("textEditor/numberRuleColor").toUInt()));
}

//! Reads the GraphicsViews section settings from omedit.ini
void OptionsDialog::readGraphicalViewsSettings()
{
  if (mpSettings->contains("iconView/extentLeft"))
    mpGraphicalViewsPage->setIconViewExtentLeft(mpSettings->value("iconView/extentLeft").toDouble());
  if (mpSettings->contains("iconView/extentBottom"))
    mpGraphicalViewsPage->setIconViewExtentBottom(mpSettings->value("iconView/extentBottom").toDouble());
  if (mpSettings->contains("iconView/extentRight"))
    mpGraphicalViewsPage->setIconViewExtentRight(mpSettings->value("iconView/extentRight").toDouble());
  if (mpSettings->contains("iconView/extentTop"))
    mpGraphicalViewsPage->setIconViewExtentTop(mpSettings->value("iconView/extentTop").toDouble());
  if (mpSettings->contains("iconView/gridHorizontal"))
    mpGraphicalViewsPage->setIconViewGridHorizontal(mpSettings->value("iconView/gridHorizontal").toDouble());
  if (mpSettings->contains("iconView/gridVertical"))
    mpGraphicalViewsPage->setIconViewGridVertical(mpSettings->value("iconView/gridVertical").toDouble());
  if (mpSettings->contains("iconView/scaleFactor"))
    mpGraphicalViewsPage->setIconViewScaleFactor(mpSettings->value("iconView/scaleFactor").toDouble());
  if (mpSettings->contains("iconView/preserveAspectRatio"))
    mpGraphicalViewsPage->setIconViewPreserveAspectRation(mpSettings->value("iconView/preserveAspectRatio").toBool());
  if (mpSettings->contains("DiagramView/extentLeft"))
    mpGraphicalViewsPage->setDiagramViewExtentLeft(mpSettings->value("DiagramView/extentLeft").toDouble());
  if (mpSettings->contains("DiagramView/extentBottom"))
    mpGraphicalViewsPage->setDiagramViewExtentBottom(mpSettings->value("DiagramView/extentBottom").toDouble());
  if (mpSettings->contains("DiagramView/extentRight"))
    mpGraphicalViewsPage->setDiagramViewExtentRight(mpSettings->value("DiagramView/extentRight").toDouble());
  if (mpSettings->contains("DiagramView/extentTop"))
    mpGraphicalViewsPage->setDiagramViewExtentTop(mpSettings->value("DiagramView/extentTop").toDouble());
  if (mpSettings->contains("DiagramView/gridHorizontal"))
    mpGraphicalViewsPage->setDiagramViewGridHorizontal(mpSettings->value("DiagramView/gridHorizontal").toDouble());
  if (mpSettings->contains("DiagramView/gridVertical"))
    mpGraphicalViewsPage->setDiagramViewGridVertical(mpSettings->value("DiagramView/gridVertical").toDouble());
  if (mpSettings->contains("DiagramView/scaleFactor"))
    mpGraphicalViewsPage->setDiagramViewScaleFactor(mpSettings->value("DiagramView/scaleFactor").toDouble());
  if (mpSettings->contains("DiagramView/preserveAspectRatio"))
    mpGraphicalViewsPage->setDiagramViewPreserveAspectRation(mpSettings->value("DiagramView/preserveAspectRatio").toBool());
}

//! Reads the Simulation section settings from omedit.ini
void OptionsDialog::readSimulationSettings()
{
  if (mpSettings->contains("simulation/matchingAlgorithm"))
  {
    int currentIndex = mpSimulationPage->getMatchingAlgorithmComboBox()->findText(mpSettings->value("simulation/matchingAlgorithm").toString(), Qt::MatchExactly);
    if (currentIndex > -1)
      mpSimulationPage->getMatchingAlgorithmComboBox()->setCurrentIndex(currentIndex);
  }
  if (mpSettings->contains("simulation/indexReductionMethod"))
  {
    int currentIndex = mpSimulationPage->getIndexReductionMethodComboBox()->findText(mpSettings->value("simulation/indexReductionMethod").toString(), Qt::MatchExactly);
    if (currentIndex > -1)
      mpSimulationPage->getIndexReductionMethodComboBox()->setCurrentIndex(currentIndex);
  }
  if (mpSettings->contains("simulation/OMCFlags"))
    mpSimulationPage->getOMCFlagsTextBox()->setText(mpSettings->value("simulation/OMCFlags").toString());
  if (mpSettings->contains("simulation/saveClassBeforeSimulation"))
    mpSimulationPage->getSaveClassBeforeSimulationCheckBox()->setChecked(mpSettings->value("simulation/saveClassBeforeSimulation").toBool());
}

//! Reads the Notifications section settings from omedit.ini
void OptionsDialog::readNotificationsSettings()
{
  if (mpSettings->contains("notifications/promptQuitApplication"))
  {
    mpNotificationsPage->getQuitApplicationCheckBox()->setChecked(mpSettings->value("notifications/promptQuitApplication").toBool());
  }
  if (mpSettings->contains("notifications/itemDroppedOnItself"))
  {
    mpNotificationsPage->getItemDroppedOnItselfCheckBox()->setChecked(mpSettings->value("notifications/itemDroppedOnItself").toBool());
  }
  if (mpSettings->contains("notifications/replaceableIfPartial"))
  {
    mpNotificationsPage->getReplaceableIfPartialCheckBox()->setChecked(mpSettings->value("notifications/replaceableIfPartial").toBool());
  }
  if (mpSettings->contains("notifications/innerModelNameChanged"))
  {
    mpNotificationsPage->getInnerModelNameChangedCheckBox()->setChecked(mpSettings->value("notifications/innerModelNameChanged").toBool());
  }
  if (mpSettings->contains("notifications/saveModelForBitmapInsertion"))
  {
    mpNotificationsPage->getSaveModelForBitmapInsertionCheckBox()->setChecked(mpSettings->value("notifications/saveModelForBitmapInsertion").toBool());
  }
}

//! Reads the LineStyle section settings from omedit.ini
void OptionsDialog::readLineStyleSettings()
{
  if (mpSettings->contains("linestyle/color"))
  {
    QColor color = QColor(mpSettings->value("linestyle/color").toUInt());
    if (color.isValid())
    {
      mpLineStylePage->setLineColor(color);
      mpLineStylePage->setLinePickColorButtonIcon();
    }
  }
  if (mpSettings->contains("linestyle/pattern"))
    mpLineStylePage->setLinePattern(mpSettings->value("linestyle/pattern").toString());
  if (mpSettings->contains("linestyle/thickness"))
    mpLineStylePage->setLineThickness(mpSettings->value("linestyle/thickness").toFloat());
  if (mpSettings->contains("linestyle/startArrow"))
    mpLineStylePage->setLineStartArrow(mpSettings->value("linestyle/startArrow").toString());
  if (mpSettings->contains("linestyle/endArrow"))
    mpLineStylePage->setLineEndArrow(mpSettings->value("linestyle/endArrow").toString());
  if (mpSettings->contains("linestyle/arrowSize"))
    mpLineStylePage->setLineArrowSize(mpSettings->value("linestyle/arrowSize").toFloat());
  if (mpSettings->contains("linestyle/smooth"))
    mpLineStylePage->setLineSmooth(mpSettings->value("linestyle/smooth").toBool());
}

//! Reads the FillStyle section settings from omedit.ini
void OptionsDialog::readFillStyleSettings()
{
  if (mpSettings->contains("fillstyle/color"))
  {
    QColor color = QColor(mpSettings->value("fillstyle/color").toUInt());
    if (color.isValid())
    {
      mpFillStylePage->setFillColor(color);
      mpFillStylePage->setFillPickColorButtonIcon();
    }
  }
  if (mpSettings->contains("fillstyle/pattern"))
    mpFillStylePage->setFillPattern(mpSettings->value("fillstyle/pattern").toString());
}

//! Reads the CurveStyle section settings from omedit.ini
void OptionsDialog::readCurveStyleSettings()
{
  if (mpSettings->contains("curvestyle/pattern"))
    mpCurveStylePage->setCurvePattern(mpSettings->value("curvestyle/pattern").toInt());
  if (mpSettings->contains("curvestyle/thickness"))
    mpCurveStylePage->setCurveThickness(mpSettings->value("curvestyle/thickness").toFloat());
}

//! Reads the Fiagro section settings from omedit.ini
void OptionsDialog::readFigaroSettings()
{
  if (mpSettings->contains("figaro/databasefile"))
    mpFigaroPage->getFigaroDatabaseFileTextBox()->setText(mpSettings->value("figaro/databasefile").toString());
  if (mpSettings->contains("figaro/mode"))
  {
    int currentIndex = mpFigaroPage->getFigaroModeComboBox()->findData(mpSettings->value("figaro/mode").toString(), Qt::UserRole, Qt::MatchExactly);
    if (currentIndex > -1) mpFigaroPage->getFigaroModeComboBox()->setCurrentIndex(currentIndex);
  }
  if (mpSettings->contains("figaro/options"))
    mpFigaroPage->getFigaroOptionsTextBox()->setText(mpSettings->value("figaro/options").toString());
  if (mpSettings->contains("figaro/process"))
    mpFigaroPage->getFigaroProcessTextBox()->setText(mpSettings->value("figaro/process").toString());
}

/*!
  Reads the Debugger section settings to omedit.ini
  */
void OptionsDialog::readDebuggerSettings()
{
  if (mpSettings->contains("algorithmicDebugger/GDBPath"))
    mpDebuggerPage->setGDBPath(mpSettings->value("algorithmicDebugger/GDBPath").toString());
  if (mpSettings->contains("algorithmicDebugger/GDBCommandTimeout"))
    mpDebuggerPage->getGDBCommandTimeoutSpinBox()->setValue(mpSettings->value("algorithmicDebugger/GDBCommandTimeout").toInt());
  if (mpSettings->contains("algorithmicDebugger/displayCFrames"))
    mpDebuggerPage->getDisplayCFramesCheckBox()->setChecked(mpSettings->value("algorithmicDebugger/displayCFrames").toBool());
  if (mpSettings->contains("algorithmicDebugger/displayUnknownFrames"))
    mpDebuggerPage->getDisplayUnknownFramesCheckBox()->setChecked(mpSettings->value("algorithmicDebugger/displayUnknownFrames").toBool());
  if (mpSettings->contains("algorithmicDebugger/clearOutputOnNewRun"))
    mpDebuggerPage->getClearOutputOnNewRunCheckBox()->setChecked(mpSettings->value("algorithmicDebugger/clearOutputOnNewRun").toBool());
  if (mpSettings->contains("algorithmicDebugger/clearLogOnNewRun"))
    mpDebuggerPage->getClearLogOnNewRunCheckBox()->setChecked(mpSettings->value("algorithmicDebugger/clearLogOnNewRun").toBool());
  if (mpSettings->contains("transformationalDebugger/alwaysShowTransformationalDebugger"))
    mpDebuggerPage->getAlwaysShowTransformationsCheckBox()->setChecked(mpSettings->value("transformationalDebugger/alwaysShowTransformationalDebugger").toBool());
  if (mpSettings->contains("transformationalDebugger/generateOperations"))
    mpDebuggerPage->getGenerateOperationsCheckBox()->setChecked(mpSettings->value("transformationalDebugger/generateOperations").toBool());
}

//! Saves the General section settings to omedit.ini
void OptionsDialog::saveGeneralSettings()
{
  // save Language option
  QString language;
  if (mpGeneralSettingsPage->getLanguageComboBox()->currentIndex() == 0)
    language = QLocale::system().name();
  else
    language = mpGeneralSettingsPage->getLanguageComboBox()->itemData(mpGeneralSettingsPage->getLanguageComboBox()->currentIndex()).toLocale().name();
  mpSettings->setValue("language", language);
  // save working directory
  mpMainWindow->getOMCProxy()->changeDirectory(mpGeneralSettingsPage->getWorkingDirectory());
  mpSettings->setValue("workingDirectory", mpMainWindow->getOMCProxy()->changeDirectory());
  // save user customizations
  mpSettings->setValue("userCustomizations", mpGeneralSettingsPage->getPreserveUserCustomizations());
  // save show protected classes
  mpSettings->setValue("showProtectedClasses", mpGeneralSettingsPage->getShowProtectedClasses());
  // show/hide the protected classes
  getMainWindow()->getLibraryTreeWidget()->showProtectedClasses(mpGeneralSettingsPage->getShowProtectedClasses());
  // save modeling view mode
  mpSettings->setValue("modeling/viewmode", mpGeneralSettingsPage->getModelingViewMode());
  if (mpGeneralSettingsPage->getModelingViewMode().compare(Helper::subWindow) == 0)
  {
    mpMainWindow->getModelWidgetContainer()->setViewMode(QMdiArea::SubWindowView);
    ModelWidget *pModelWidget = mpMainWindow->getModelWidgetContainer()->getCurrentModelWidget();
    if (pModelWidget)
    {
      pModelWidget->show();
      pModelWidget->setWindowState(Qt::WindowMaximized);
    }
  }
  else
  {
    mpMainWindow->getModelWidgetContainer()->setViewMode(QMdiArea::TabbedView);
  }
  // save plotting view mode
  mpSettings->setValue("plotting/viewmode", mpGeneralSettingsPage->getPlottingViewMode());
  if (mpGeneralSettingsPage->getPlottingViewMode().compare(Helper::subWindow) == 0)
  {
    mpMainWindow->getPlotWindowContainer()->setViewMode(QMdiArea::SubWindowView);
    OMPlot::PlotWindow *pPlotWindow = mpMainWindow->getPlotWindowContainer()->getCurrentWindow();
    if (pPlotWindow)
    {
      pPlotWindow->show();
      pPlotWindow->setWindowState(Qt::WindowMaximized);
    }
  }
  else
  {
    mpMainWindow->getPlotWindowContainer()->setViewMode(QMdiArea::TabbedView);
  }
  // save default view
  mpSettings->setValue("defaultView", mpGeneralSettingsPage->getDefaultView());
  // save auto save
  mpSettings->setValue("autoSave/enable", mpGeneralSettingsPage->getEnableAutoSaveGroupBox()->isChecked());
  mpSettings->setValue("autoSave/interval", mpGeneralSettingsPage->getAutoSaveIntervalSpinBox()->value());
  mpSettings->setValue("autoSave/enableSingleClasses", mpGeneralSettingsPage->getEnableAutoSaveForSingleClassesCheckBox()->isChecked());
  mpSettings->setValue("autoSave/enableOneFilePackages", mpGeneralSettingsPage->getEnableAutoSaveForOneFilePackagesCheckBox()->isChecked());
  mpMainWindow->toggleAutoSave();
  // save welcome page
  switch (mpGeneralSettingsPage->getWelcomePageView())
  {
    case 2:
      mpMainWindow->getWelcomePageWidget()->getSplitter()->setOrientation(Qt::Vertical);
      break;
    case 1:
    default:
      mpMainWindow->getWelcomePageWidget()->getSplitter()->setOrientation(Qt::Horizontal);
      break;
  }
  mpSettings->setValue("welcomePage/view", mpGeneralSettingsPage->getWelcomePageView());
  if (mpGeneralSettingsPage->getShowLatestNewsCheckBox()->isChecked())
  {
    mpMainWindow->getWelcomePageWidget()->getLatestNewsFrame()->show();
    mpMainWindow->getWelcomePageWidget()->addLatestNewsListItems();
  }
  else
  {
    mpMainWindow->getWelcomePageWidget()->getLatestNewsFrame()->hide();
  }
  mpSettings->setValue("welcomePage/showLatestNews", mpGeneralSettingsPage->getShowLatestNewsCheckBox()->isChecked());
}

//! Saves the Libraries section settings to omedit.ini
void OptionsDialog::saveLibrariesSettings()
{
  // read the settings and add system libraries
  mpSettings->beginGroup("libraries");
  foreach (QString lib, mpSettings->childKeys())
  {
    mpSettings->remove(lib);
  }
  QTreeWidgetItemIterator systemLibrariesIterator(mpLibrariesPage->getSystemLibrariesTree());
  while (*systemLibrariesIterator)
  {
    QTreeWidgetItem *pItem = dynamic_cast<QTreeWidgetItem*>(*systemLibrariesIterator);
    mpSettings->setValue(pItem->text(0), pItem->text(1));
    ++systemLibrariesIterator;
  }
  mpSettings->endGroup();
  mpSettings->setValue("forceModelicaLoad", mpLibrariesPage->getForceModelicaLoadCheckBox()->isChecked());
  // read the settings and add user libraries
  mpSettings->beginGroup("userlibraries");
  foreach (QString lib, mpSettings->childKeys())
  {
    mpSettings->remove(lib);
  }
  QTreeWidgetItemIterator userLibrariesIterator(mpLibrariesPage->getUserLibrariesTree());
  while (*userLibrariesIterator)
  {
    QTreeWidgetItem *pItem = dynamic_cast<QTreeWidgetItem*>(*userLibrariesIterator);
    mpSettings->setValue(QUrl::toPercentEncoding(pItem->text(0)), pItem->text(1));
    ++userLibrariesIterator;
  }
  mpSettings->endGroup();
}

//! Saves the ModelicaText settings to omedit.ini
void OptionsDialog::saveModelicaTextSettings()
{
  mpSettings->setValue("textEditor/enableSyntaxHighlighting", mpModelicaTextEditorPage->getSyntaxHighlightingCheckbox()->isChecked());
  mpSettings->setValue("textEditor/enableLineWrapping", mpModelicaTextEditorPage->getLineWrappingCheckbox()->isChecked());
  mpSettings->setValue("textEditor/fontFamily", mpModelicaTextSettings->getFontFamily());
  mpSettings->setValue("textEditor/fontSize", mpModelicaTextSettings->getFontSize());
  mpSettings->setValue("textEditor/textRuleColor", mpModelicaTextSettings->getTextRuleColor().rgba());
  mpSettings->setValue("textEditor/keywordRuleColor", mpModelicaTextSettings->getKeywordRuleColor().rgba());
  mpSettings->setValue("textEditor/typeRuleColor", mpModelicaTextSettings->getTypeRuleColor().rgba());
  mpSettings->setValue("textEditor/functionRuleColor", mpModelicaTextSettings->getFunctionRuleColor().rgba());
  mpSettings->setValue("textEditor/quotesRuleColor", mpModelicaTextSettings->getQuotesRuleColor().rgba());
  mpSettings->setValue("textEditor/commentRuleColor", mpModelicaTextSettings->getCommentRuleColor().rgba());
  mpSettings->setValue("textEditor/numberRuleColor", mpModelicaTextSettings->getNumberRuleColor().rgba());
}

//! Saves the GraphicsViews section settings to omedit.ini
void OptionsDialog::saveGraphicalViewsSettings()
{
  mpSettings->setValue("iconView/extentLeft", mpGraphicalViewsPage->getIconViewExtentLeft());
  mpSettings->setValue("iconView/extentBottom", mpGraphicalViewsPage->getIconViewExtentBottom());
  mpSettings->setValue("iconView/extentRight", mpGraphicalViewsPage->getIconViewExtentRight());
  mpSettings->setValue("iconView/extentTop", mpGraphicalViewsPage->getIconViewExtentTop());
  mpSettings->setValue("iconView/gridHorizontal", mpGraphicalViewsPage->getIconViewGridHorizontal());
  mpSettings->setValue("iconView/gridVertical", mpGraphicalViewsPage->getIconViewGridVertical());
  mpSettings->setValue("iconView/scaleFactor", mpGraphicalViewsPage->getIconViewScaleFactor());
  mpSettings->setValue("iconView/preserveAspectRatio", mpGraphicalViewsPage->getIconViewPreserveAspectRation());
  mpSettings->setValue("DiagramView/extentLeft", mpGraphicalViewsPage->getDiagramViewExtentLeft());
  mpSettings->setValue("DiagramView/extentBottom", mpGraphicalViewsPage->getDiagramViewExtentBottom());
  mpSettings->setValue("DiagramView/extentRight", mpGraphicalViewsPage->getDiagramViewExtentRight());
  mpSettings->setValue("DiagramView/extentTop", mpGraphicalViewsPage->getDiagramViewExtentTop());
  mpSettings->setValue("DiagramView/gridHorizontal", mpGraphicalViewsPage->getDiagramViewGridHorizontal());
  mpSettings->setValue("DiagramView/gridVertical", mpGraphicalViewsPage->getDiagramViewGridVertical());
  mpSettings->setValue("DiagramView/scaleFactor", mpGraphicalViewsPage->getDiagramViewScaleFactor());
  mpSettings->setValue("DiagramView/preserveAspectRatio", mpGraphicalViewsPage->getDiagramViewPreserveAspectRation());
}

//! Saves the Simulation section settings to omedit.ini
void OptionsDialog::saveSimulationSettings()
{
  mpSettings->setValue("simulation/matchingAlgorithm", mpSimulationPage->getMatchingAlgorithmComboBox()->currentText());
  mpMainWindow->getOMCProxy()->setMatchingAlgorithm(mpSimulationPage->getMatchingAlgorithmComboBox()->currentText());
  mpSettings->setValue("simulation/indexReductionMethod", mpSimulationPage->getIndexReductionMethodComboBox()->currentText());
  mpMainWindow->getOMCProxy()->setIndexReductionMethod(mpSimulationPage->getIndexReductionMethodComboBox()->currentText());
  mpMainWindow->getOMCProxy()->clearCommandLineOptions();
  if (mpMainWindow->getOMCProxy()->setCommandLineOptions(mpSimulationPage->getOMCFlagsTextBox()->text()))
    mpSettings->setValue("simulation/OMCFlags", mpSimulationPage->getOMCFlagsTextBox()->text());
  else
    mpSimulationPage->getOMCFlagsTextBox()->setText(mpSettings->value("simulation/OMCFlags").toString());
  mpSettings->setValue("simulation/saveClassBeforeSimulation", mpSimulationPage->getSaveClassBeforeSimulationCheckBox()->isChecked());
}

//! Saves the Notifications section settings to omedit.ini
void OptionsDialog::saveNotificationsSettings()
{
  mpSettings->setValue("notifications/promptQuitApplication", mpNotificationsPage->getQuitApplicationCheckBox()->isChecked());
  mpSettings->setValue("notifications/itemDroppedOnItself", mpNotificationsPage->getItemDroppedOnItselfCheckBox()->isChecked());
  mpSettings->setValue("notifications/replaceableIfPartial", mpNotificationsPage->getReplaceableIfPartialCheckBox()->isChecked());
  mpSettings->setValue("notifications/innerModelNameChanged", mpNotificationsPage->getInnerModelNameChangedCheckBox()->isChecked());
  mpSettings->setValue("notifications/saveModelForBitmapInsertion", mpNotificationsPage->getSaveModelForBitmapInsertionCheckBox()->isChecked());
}

//! Saves the LineStyle section settings to omedit.ini
void OptionsDialog::saveLineStyleSettings()
{
  mpSettings->setValue("linestyle/color", mpLineStylePage->getLineColor().rgba());
  mpSettings->setValue("linestyle/pattern", mpLineStylePage->getLinePattern());
  mpSettings->setValue("linestyle/thickness", mpLineStylePage->getLineThickness());
  mpSettings->setValue("linestyle/startArrow", mpLineStylePage->getLineStartArrow());
  mpSettings->setValue("linestyle/endArrow", mpLineStylePage->getLineEndArrow());
  mpSettings->setValue("linestyle/arrowSize", mpLineStylePage->getLineArrowSize());
  mpSettings->setValue("linestyle/smooth", mpLineStylePage->getLineSmooth());
}

//! Saves the FillStyle section settings to omedit.ini
void OptionsDialog::saveFillStyleSettings()
{
  mpSettings->setValue("fillstyle/color", mpFillStylePage->getFillColor().rgba());
  mpSettings->setValue("fillstyle/pattern", mpFillStylePage->getFillPattern());
}

//! Saves the CurveStyle section settings to omedit.ini
void OptionsDialog::saveCurveStyleSettings()
{
  mpSettings->setValue("curvestyle/pattern", mpCurveStylePage->getCurvePattern());
  mpSettings->setValue("curvestyle/thickness", mpCurveStylePage->getCurveThickness());
}

//! Saves the Figaro section settings to omedit.ini
void OptionsDialog::saveFigaroSettings()
{
  mpSettings->setValue("figaro/databasefile", mpFigaroPage->getFigaroDatabaseFileTextBox()->text());
  mpSettings->setValue("figaro/mode", mpFigaroPage->getFigaroModeComboBox()->itemData(mpFigaroPage->getFigaroModeComboBox()->currentIndex()).toString());
  mpSettings->setValue("figaro/options", mpFigaroPage->getFigaroOptionsTextBox()->text());
  mpSettings->setValue("figaro/process", mpFigaroPage->getFigaroProcessTextBox()->text());
}

/*!
  Saves the Debugger section settings to omedit.ini
  */
void OptionsDialog::saveDebuggerSettings()
{
  mpSettings->beginGroup("algorithmicDebugger");
  mpSettings->setValue("GDBPath", mpDebuggerPage->getGDBPath());
  mpSettings->value("GDBCommandTimeout", mpDebuggerPage->getGDBCommandTimeoutSpinBox()->value());
  mpSettings->setValue("displayCFrames", mpDebuggerPage->getDisplayCFramesCheckBox()->isChecked());
  mpSettings->setValue("displayUnknownFrames", mpDebuggerPage->getDisplayUnknownFramesCheckBox()->isChecked());
  mpMainWindow->getDebuggerMainWindow()->getStackFramesWidget()->getStackFramesTreeWidget()->updateStackFrames();
  mpSettings->setValue("clearOutputOnNewRun", mpDebuggerPage->getClearOutputOnNewRunCheckBox()->isChecked());
  mpSettings->setValue("clearLogOnNewRun", mpDebuggerPage->getClearLogOnNewRunCheckBox()->isChecked());
  mpSettings->endGroup();
  mpSettings->beginGroup("transformationalDebugger");
  mpSettings->setValue("alwaysShowTransformationalDebugger", mpDebuggerPage->getAlwaysShowTransformationsCheckBox()->isChecked());
  mpSettings->setValue("generateOperations", mpDebuggerPage->getGenerateOperationsCheckBox()->isChecked());
  if (mpDebuggerPage->getGenerateOperationsCheckBox()->isChecked())
    mpMainWindow->getOMCProxy()->setCommandLineOptions("+d=infoXmlOperations");
  mpSettings->endGroup();
}

//! Sets up the Options Widget dialog
void OptionsDialog::setUpDialog()
{
  mpOptionsList = new QListWidget;
  mpOptionsList->setItemDelegate(new OptionsItemDelegate(mpOptionsList));
  mpOptionsList->setViewMode(QListView::ListMode);
  mpOptionsList->setMovement(QListView::Static);
  mpOptionsList->setIconSize(QSize(22, 22));
  mpOptionsList->setCurrentRow(0, QItemSelectionModel::Select);
  connect(mpOptionsList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));
  // add items to options list
  addListItems();
  // get maximum width for options list
  mpOptionsList->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
  int width = mpOptionsList->sizeHintForColumn(0) + mpOptionsList->frameWidth() * 2 + 5;
  if (mpOptionsList->verticalScrollBar()->isVisible())
      width += mpOptionsList->verticalScrollBar()->width();
  mpOptionsList->setMaximumWidth(width);
  // create pages
  createPages();
  // Create the buttons
  mpOkButton = new QPushButton(Helper::ok);
  mpOkButton->setAutoDefault(true);
  connect(mpOkButton, SIGNAL(clicked()), SLOT(saveSettings()));
  mpCancelButton = new QPushButton(Helper::cancel);
  mpCancelButton->setAutoDefault(false);
  connect(mpCancelButton, SIGNAL(clicked()), SLOT(reject()));
  mpButtonBox = new QDialogButtonBox(Qt::Horizontal);
  mpButtonBox->addButton(mpOkButton, QDialogButtonBox::ActionRole);
  mpButtonBox->addButton(mpCancelButton, QDialogButtonBox::ActionRole);
  QHBoxLayout *horizontalLayout = new QHBoxLayout;
  horizontalLayout->addWidget(mpOptionsList);
  horizontalLayout->addWidget(mpPagesWidget, 1);
  // Create a layout
  QGridLayout *mainLayout = new QGridLayout;
  mainLayout->addLayout(horizontalLayout, 0, 0);
  mainLayout->addWidget(mpButtonBox, 1, 0);
  setLayout(mainLayout);
}

//! Adds items to the list view of Options Widget
void OptionsDialog::addListItems()
{
  // General Settings Item
  QListWidgetItem *pGeneralSettingsItem = new QListWidgetItem(mpOptionsList);
  pGeneralSettingsItem->setIcon(QIcon(":/Resources/icons/general.png"));
  pGeneralSettingsItem->setText(tr("General"));
  mpOptionsList->item(0)->setSelected(true);
  // Libraries Item
  QListWidgetItem *pLibrariesItem = new QListWidgetItem(mpOptionsList);
  pLibrariesItem->setIcon(QIcon(":/Resources/icons/libraries.png"));
  pLibrariesItem->setText(Helper::libraries);
  // Modelica Text Item
  QListWidgetItem *pModelicaTextEditorItem = new QListWidgetItem(mpOptionsList);
  pModelicaTextEditorItem->setIcon(QIcon(":/Resources/icons/modeltextoptions.png"));
  pModelicaTextEditorItem->setText(tr("Modelica Text Editor"));
  // Graphical Views Item
  QListWidgetItem *pGraphicalViewsItem = new QListWidgetItem(mpOptionsList);
  pGraphicalViewsItem->setIcon(QIcon(":/Resources/icons/modeling.png"));
  pGraphicalViewsItem->setText(tr("Graphical Views"));
  // Simulation Item
  QListWidgetItem *pSimulationItem = new QListWidgetItem(mpOptionsList);
  pSimulationItem->setIcon(QIcon(":/Resources/icons/simulate.svg"));
  pSimulationItem->setText(Helper::simulation);
  // Notifications Item
  QListWidgetItem *pNotificationsItem = new QListWidgetItem(mpOptionsList);
  pNotificationsItem->setIcon(QIcon(":/Resources/icons/notificationicon.png"));
  pNotificationsItem->setText(tr("Notifications"));
  // Pen Style Item
  QListWidgetItem *pLineStyleItem = new QListWidgetItem(mpOptionsList);
  pLineStyleItem->setIcon(QIcon(":/Resources/icons/linestyle.png"));
  pLineStyleItem->setText(Helper::lineStyle);
  // Brush Style Item
  QListWidgetItem *pFillStyleItem = new QListWidgetItem(mpOptionsList);
  pFillStyleItem->setIcon(QIcon(":/Resources/icons/fillstyle.png"));
  pFillStyleItem->setText(Helper::fillStyle);
  // Curve Style Item
  QListWidgetItem *pCurveStyleItem = new QListWidgetItem(mpOptionsList);
  pCurveStyleItem->setIcon(QIcon(":/Resources/icons/omplot.png"));
  pCurveStyleItem->setText(Helper::curveStyle);
  // Figaro Item
  QListWidgetItem *pFigaroItem = new QListWidgetItem(mpOptionsList);
  pFigaroItem->setIcon(QIcon(":/Resources/icons/console.png"));
  pFigaroItem->setText(Helper::figaro);
  // Debugger Item
  QListWidgetItem *pDebuggerItem = new QListWidgetItem(mpOptionsList);
  pDebuggerItem->setIcon(QIcon(":/Resources/icons/debugger.svg"));
  pDebuggerItem->setText(tr("Debugger"));
}

//! Creates pages for the Options Widget. The pages are created as stacked widget and are mapped with mpOptionsList.
void OptionsDialog::createPages()
{
  mpPagesWidget = new QStackedWidget;
  mpPagesWidget->addWidget(mpGeneralSettingsPage);
  mpPagesWidget->addWidget(mpLibrariesPage);
  mpPagesWidget->addWidget(mpModelicaTextEditorPage);
  mpPagesWidget->addWidget(mpGraphicalViewsPage);
  mpPagesWidget->addWidget(mpSimulationPage);
  mpPagesWidget->addWidget(mpNotificationsPage);
  mpPagesWidget->addWidget(mpLineStylePage);
  mpPagesWidget->addWidget(mpFillStylePage);
  mpPagesWidget->addWidget(mpCurveStylePage);
  mpPagesWidget->addWidget(mpFigaroPage);
  mpPagesWidget->addWidget(mpDebuggerPage);
}

MainWindow* OptionsDialog::getMainWindow()
{
  return mpMainWindow;
}

GeneralSettingsPage* OptionsDialog::getGeneralSettingsPage()
{
  return mpGeneralSettingsPage;
}

ModelicaTextSettings* OptionsDialog::getModelicaTextSettings()
{
  return mpModelicaTextSettings;
}

ModelicaTextEditorPage* OptionsDialog::getModelicaTextEditorPage()
{
  return mpModelicaTextEditorPage;
}

GraphicalViewsPage* OptionsDialog::getGraphicalViewsPage()
{
  return mpGraphicalViewsPage;
}

SimulationPage* OptionsDialog::getSimulationPage()
{
  return mpSimulationPage;
}

NotificationsPage* OptionsDialog::getNotificationsPage()
{
  return mpNotificationsPage;
}

LineStylePage* OptionsDialog::getLineStylePage()
{
  return mpLineStylePage;
}

FillStylePage* OptionsDialog::getFillStylePage()
{
  return mpFillStylePage;
}

CurveStylePage* OptionsDialog::getCurveStylePage()
{
  return mpCurveStylePage;
}

FigaroPage* OptionsDialog::getFigaroPage()
{
  return mpFigaroPage;
}

DebuggerPage* OptionsDialog::getDebuggerPage()
{
  return mpDebuggerPage;
}

//! Change the page in Options Widget when the mpOptionsList currentItemChanged Signal is raised.
void OptionsDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
  if (!current)
    current = previous;

  mpPagesWidget->setCurrentIndex(mpOptionsList->row(current));
}

//! Reimplementation of QWidget's reject function. If user reject the settings then set them back to original.
void OptionsDialog::reject()
{
  // read the old settings from the file
  readSettings();
  QDialog::reject();
}

//! Saves the settings to omedit.ini file.
void OptionsDialog::saveSettings()
{
  saveGeneralSettings();
  saveLibrariesSettings();
  saveModelicaTextSettings();
  // emit the signal so that all syntax highlighters are updated
  emit modelicaTextSettingsChanged();
  // emit the signal so that all text editors can set line wrapping mode
  emit updateLineWrapping();
  saveGraphicalViewsSettings();
  saveSimulationSettings();
  saveNotificationsSettings();
  saveLineStyleSettings();
  saveFillStyleSettings();
  saveCurveStyleSettings();
  saveFigaroSettings();
  saveDebuggerSettings();
  mpSettings->sync();
  accept();
}

//! @class GeneralSettingsPage
//! @brief Creates an interface for genaral settings.

//! Constructor
//! @param pParent is the pointer to OptionsDialog
GeneralSettingsPage::GeneralSettingsPage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpOptionsDialog = pParent;
  mpGeneralSettingsGroupBox = new QGroupBox(Helper::general);
  // Language Option
  mpLanguageLabel = new Label(tr("Language:"));
  mpLanguageComboBox = new QComboBox;
  mpLanguageComboBox->addItem(tr("Auto Detected"), "");
  /* Slow sorting, but works using regular Qt functions */
  QMap<QString, QLocale> map;
  map.insert(tr("Chinese").append(" (zh_CN)"), QLocale(QLocale::Chinese));
  map.insert(tr("English").append(" (en)"), QLocale(QLocale::English));
  map.insert(tr("French").append(" (fr)"), QLocale(QLocale::French));
  map.insert(tr("German").append(" (de)"), QLocale(QLocale::German));
  map.insert(tr("Italian").append(" (it)"), QLocale(QLocale::Italian));
  map.insert(tr("Japanese").append(" (ja)"), QLocale(QLocale::Japanese));
  map.insert(tr("Romanian").append(" (ro)"), QLocale(QLocale::Romanian));
  map.insert(tr("Russian").append(" (ru)"), QLocale(QLocale::Russian));
  map.insert(tr("Spanish").append(" (es)"), QLocale(QLocale::Spanish));
  map.insert(tr("Swedish").append(" (sv)"), QLocale(QLocale::Swedish));
  QStringList keys(map.keys());
  keys.sort();
  foreach (const QString &key, keys) {
    QLocale locale = map[key];
    mpLanguageComboBox->addItem(key, locale);
  }
  // Working Directory
  mpWorkingDirectoryLabel = new Label(Helper::workingDirectory);
  mpWorkingDirectoryTextBox = new QLineEdit(mpOptionsDialog->getMainWindow()->getOMCProxy()->changeDirectory());
  mpWorkingDirectoryBrowseButton = new QPushButton(Helper::browse);
  mpWorkingDirectoryBrowseButton->setAutoDefault(false);
  connect(mpWorkingDirectoryBrowseButton, SIGNAL(clicked()), SLOT(selectWorkingDirectory()));
  // Store Customizations Option
  mpPreserveUserCustomizations = new QCheckBox(tr("Preserve User's GUI Customizations"));
  // set the layout of general settings group
  QGridLayout *generalSettingsLayout = new QGridLayout;
  generalSettingsLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  generalSettingsLayout->addWidget(mpLanguageLabel, 0, 0);
  generalSettingsLayout->addWidget(mpLanguageComboBox, 0, 1, 1, 2);
  generalSettingsLayout->addWidget(mpWorkingDirectoryLabel, 1, 0);
  generalSettingsLayout->addWidget(mpWorkingDirectoryTextBox, 1, 1);
  generalSettingsLayout->addWidget(mpWorkingDirectoryBrowseButton, 1, 2);
  generalSettingsLayout->addWidget(mpPreserveUserCustomizations, 2, 0, 1, 3);
  mpGeneralSettingsGroupBox->setLayout(generalSettingsLayout);
  // Libraries Browser group box
  mpLibrariesBrowserGroupBox = new QGroupBox(tr("Libraries Browser"));
  mpShowProtectedClasses = new QCheckBox(tr("Show Protected Classes"));
  // Libraries Browser group box layout
  QGridLayout *pLibrariesBrowserLayout = new QGridLayout;
  pLibrariesBrowserLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pLibrariesBrowserLayout->addWidget(mpShowProtectedClasses);
  mpLibrariesBrowserGroupBox->setLayout(pLibrariesBrowserLayout);
  // Modeling View Mode
  mpModelingViewModeGroupBox = new QGroupBox(tr("Modeling View Mode"));
  mpModelingTabbedViewRadioButton = new QRadioButton(tr("Tabbed View"));
  mpModelingTabbedViewRadioButton->setChecked(true);
  mpModelingSubWindowViewRadioButton = new QRadioButton(tr("SubWindow View"));
  QButtonGroup *pModelingViewModeButtonGroup = new QButtonGroup;
  pModelingViewModeButtonGroup->addButton(mpModelingTabbedViewRadioButton);
  pModelingViewModeButtonGroup->addButton(mpModelingSubWindowViewRadioButton);
  // modeling view radio buttons layout
  QHBoxLayout *pModelingRadioButtonsLayout = new QHBoxLayout;
  pModelingRadioButtonsLayout->addWidget(mpModelingTabbedViewRadioButton);
  pModelingRadioButtonsLayout->addWidget(mpModelingSubWindowViewRadioButton);
  // set the layout of modeling view mode group
  QGridLayout *modelingViewModeLayout = new QGridLayout;
  modelingViewModeLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  modelingViewModeLayout->addLayout(pModelingRadioButtonsLayout, 0, 0);
  mpModelingViewModeGroupBox->setLayout(modelingViewModeLayout);
  // Plotting View Mode
  mpPlottingViewModeGroupBox = new QGroupBox(tr("Plotting View Mode"));
  mpPlottingTabbedViewRadioButton = new QRadioButton(tr("Tabbed View"));
  mpPlottingTabbedViewRadioButton->setChecked(true);
  mpPlottingSubWindowViewRadioButton = new QRadioButton(tr("SubWindow View"));
  QButtonGroup *pPlottingViewModeButtonGroup = new QButtonGroup;
  pPlottingViewModeButtonGroup->addButton(mpPlottingTabbedViewRadioButton);
  pPlottingViewModeButtonGroup->addButton(mpPlottingSubWindowViewRadioButton);
  // plotting view radio buttons layout
  QHBoxLayout *pPlottingRadioButtonsLayout = new QHBoxLayout;
  pPlottingRadioButtonsLayout->addWidget(mpPlottingTabbedViewRadioButton);
  pPlottingRadioButtonsLayout->addWidget(mpPlottingSubWindowViewRadioButton);
  // set the layout of plotting view mode group
  QGridLayout *pPlottingViewModeLayout = new QGridLayout;
  pPlottingViewModeLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pPlottingViewModeLayout->addLayout(pPlottingRadioButtonsLayout, 0, 0);
  mpPlottingViewModeGroupBox->setLayout(pPlottingViewModeLayout);
  // Default View
  mpDefaultViewGroupBox = new QGroupBox(tr("Default View"));
  mpDefaultViewGroupBox->setToolTip(tr("This settings will be used when no preferredView annotation is defined."));
  mpIconViewRadioButton = new QRadioButton(Helper::iconView);
  mpDiagramViewRadioButton = new QRadioButton(Helper::diagramView);
  mpDiagramViewRadioButton->setChecked(true);
  mpTextViewRadioButton = new QRadioButton(Helper::textView);
  mpDocumentationViewRadioButton = new QRadioButton(Helper::documentationView);
  QButtonGroup *pDefaultViewButtonGroup = new QButtonGroup;
  pDefaultViewButtonGroup->addButton(mpIconViewRadioButton);
  pDefaultViewButtonGroup->addButton(mpDiagramViewRadioButton);
  pDefaultViewButtonGroup->addButton(mpTextViewRadioButton);
  pDefaultViewButtonGroup->addButton(mpDocumentationViewRadioButton);
  // default view radio buttons layout
  QGridLayout *pDefaultViewRadioButtonsGridLayout = new QGridLayout;
  pDefaultViewRadioButtonsGridLayout->addWidget(mpIconViewRadioButton, 0, 0);
  pDefaultViewRadioButtonsGridLayout->addWidget(mpDiagramViewRadioButton, 0, 1);
  pDefaultViewRadioButtonsGridLayout->addWidget(mpTextViewRadioButton, 1, 0);
  pDefaultViewRadioButtonsGridLayout->addWidget(mpDocumentationViewRadioButton, 1, 1);
  // set the layout of default view group
  QGridLayout *pDefautlViewLayout = new QGridLayout;
  pDefautlViewLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pDefautlViewLayout->addLayout(pDefaultViewRadioButtonsGridLayout, 0, 0);
  mpDefaultViewGroupBox->setLayout(pDefautlViewLayout);
  // Auto Save
  mpEnableAutoSaveGroupBox = new QGroupBox(tr("Enable Auto Save"));
  mpEnableAutoSaveGroupBox->setToolTip("Auto save feature is experimental. If you encounter unexpected crashes then disable it.");
  mpEnableAutoSaveGroupBox->setCheckable(true);
  mpEnableAutoSaveGroupBox->setChecked(true);
  mpAutoSaveIntervalLabel = new Label(tr("Auto Save Interval:"));
  mpAutoSaveIntervalSpinBox = new QSpinBox;
  mpAutoSaveIntervalSpinBox->setSuffix(tr(" seconds"));
  mpAutoSaveIntervalSpinBox->setRange(60, std::numeric_limits<int>::max());
  mpAutoSaveIntervalSpinBox->setSingleStep(30);
  mpAutoSaveIntervalSpinBox->setValue(300);
  mpAutoSaveSecondsLabel = new Label;
  connect(mpAutoSaveIntervalSpinBox, SIGNAL(valueChanged(int)), SLOT(autoSaveIntervalValueChanged(int)));
  mpEnableAutoSaveForSingleClassesCheckBox = new QCheckBox(tr("Enable Auto Save for single classes"));
  mpEnableAutoSaveForSingleClassesCheckBox->setChecked(true);
  mpEnableAutoSaveForOneFilePackagesCheckBox = new QCheckBox(tr("Enable Auto Save for one file packages (Experimental)"));
  // calculate the auto save interval seconds.
  autoSaveIntervalValueChanged(mpAutoSaveIntervalSpinBox->value());
  // Auto Save layout
  QGridLayout *pAutoSaveGridLayout = new QGridLayout;
  pAutoSaveGridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pAutoSaveGridLayout->addWidget(mpAutoSaveIntervalLabel, 0, 0);
  pAutoSaveGridLayout->addWidget(mpAutoSaveIntervalSpinBox, 0, 1);
  pAutoSaveGridLayout->addWidget(mpAutoSaveSecondsLabel, 0, 2);
  pAutoSaveGridLayout->addWidget(mpEnableAutoSaveForSingleClassesCheckBox, 1, 0, 1, 3);
  pAutoSaveGridLayout->addWidget(mpEnableAutoSaveForOneFilePackagesCheckBox, 2, 0, 1, 3);
  mpEnableAutoSaveGroupBox->setLayout(pAutoSaveGridLayout);
  // Welcome Page
  mpWelcomePageGroupBox = new QGroupBox(tr("Welcome Page"));
  mpHorizontalViewRadioButton = new QRadioButton(tr("Horizontal View"));
  mpHorizontalViewRadioButton->setChecked(true);
  mpVerticalViewRadioButton = new QRadioButton(tr("Vertical View"));
  QButtonGroup *pWelcomePageViewButtons = new QButtonGroup;
  pWelcomePageViewButtons->addButton(mpHorizontalViewRadioButton);
  pWelcomePageViewButtons->addButton(mpVerticalViewRadioButton);
  // plotting view radio buttons layout
  QHBoxLayout *pWelcomePageViewButtonsLayout = new QHBoxLayout;
  pWelcomePageViewButtonsLayout->addWidget(mpHorizontalViewRadioButton);
  pWelcomePageViewButtonsLayout->addWidget(mpVerticalViewRadioButton);
  // Show/hide latest news checkbox
  mpShowLatestNewsCheckBox = new QCheckBox(tr("Show Latest News"));
  mpShowLatestNewsCheckBox->setChecked(true);
  // Welcome Page layout
  QGridLayout *pWelcomePageGridLayout = new QGridLayout;
  pWelcomePageGridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pWelcomePageGridLayout->addLayout(pWelcomePageViewButtonsLayout, 0, 0);
  pWelcomePageGridLayout->addWidget(mpShowLatestNewsCheckBox, 1, 0);
  mpWelcomePageGroupBox->setLayout(pWelcomePageGridLayout);
  // set the layout
  QVBoxLayout *pMainLayout = new QVBoxLayout;
  pMainLayout->setContentsMargins(0, 0, 0, 0);
  pMainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pMainLayout->addWidget(mpGeneralSettingsGroupBox);
  pMainLayout->addWidget(mpLibrariesBrowserGroupBox);
  pMainLayout->addWidget(mpModelingViewModeGroupBox);
  pMainLayout->addWidget(mpPlottingViewModeGroupBox);
  pMainLayout->addWidget(mpDefaultViewGroupBox);
  pMainLayout->addWidget(mpEnableAutoSaveGroupBox);
  pMainLayout->addWidget(mpWelcomePageGroupBox);
  setLayout(pMainLayout);
}

QComboBox* GeneralSettingsPage::getLanguageComboBox()
{
  return mpLanguageComboBox;
}

//! Sets the working directory text box value.
//! @param value the working directory value.
//! @see getWorkingDirectory();
void GeneralSettingsPage::setWorkingDirectory(QString value)
{
  mpWorkingDirectoryTextBox->setText(value);
}

//! Returns the working directory text box value.
//! @return working directory as string.
//! @see setWorkingDirectory();
QString GeneralSettingsPage::getWorkingDirectory()
{
  return mpWorkingDirectoryTextBox->text();
}

void GeneralSettingsPage::setPreserveUserCustomizations(bool value)
{
  mpPreserveUserCustomizations->setChecked(value);
}

bool GeneralSettingsPage::getPreserveUserCustomizations()
{
  return mpPreserveUserCustomizations->isChecked();
}

void GeneralSettingsPage::setShowProtectedClasses(bool value)
{
  mpShowProtectedClasses->setChecked(value);
}

bool GeneralSettingsPage::getShowProtectedClasses()
{
  return mpShowProtectedClasses->isChecked();
}

void GeneralSettingsPage::setModelingViewMode(QString value)
{
  if (value.compare(Helper::subWindow) == 0)
    mpModelingSubWindowViewRadioButton->setChecked(true);
  else
    mpModelingTabbedViewRadioButton->setChecked(true);
}

QString GeneralSettingsPage::getModelingViewMode()
{
  if (mpModelingSubWindowViewRadioButton->isChecked())
    return Helper::subWindow;
  else
    return Helper::tabbed;
}

void GeneralSettingsPage::setPlottingViewMode(QString value)
{
  if (value.compare(Helper::subWindow) == 0)
    mpPlottingSubWindowViewRadioButton->setChecked(true);
  else
    mpPlottingTabbedViewRadioButton->setChecked(true);
}

QString GeneralSettingsPage::getPlottingViewMode()
{
  if (mpPlottingSubWindowViewRadioButton->isChecked())
    return Helper::subWindow;
  else
    return Helper::tabbed;
}

void GeneralSettingsPage::setDefaultView(QString value)
{
  if (value.compare(Helper::iconView) == 0)
    mpIconViewRadioButton->setChecked(true);
  else if (value.compare(Helper::textView) == 0)
    mpTextViewRadioButton->setChecked(true);
  else if (value.compare(Helper::documentationView) == 0)
    mpDocumentationViewRadioButton->setChecked(true);
  else
    mpDiagramViewRadioButton->setChecked(true);
}

QString GeneralSettingsPage::getDefaultView()
{
  if (mpIconViewRadioButton->isChecked())
    return Helper::iconView;
  else if (mpTextViewRadioButton->isChecked())
    return Helper::textView;
  else if (mpDocumentationViewRadioButton->isChecked())
    return Helper::documentationView;
  else
    return Helper::diagramView;
}

QGroupBox* GeneralSettingsPage::getEnableAutoSaveGroupBox()
{
  return mpEnableAutoSaveGroupBox;
}

QSpinBox* GeneralSettingsPage::getAutoSaveIntervalSpinBox()
{
  return mpAutoSaveIntervalSpinBox;
}

QCheckBox* GeneralSettingsPage::getEnableAutoSaveForSingleClassesCheckBox()
{
  return mpEnableAutoSaveForSingleClassesCheckBox;
}

QCheckBox* GeneralSettingsPage::getEnableAutoSaveForOneFilePackagesCheckBox()
{
  return mpEnableAutoSaveForOneFilePackagesCheckBox;
}

int GeneralSettingsPage::getWelcomePageView()
{
  if (mpHorizontalViewRadioButton->isChecked())
    return 1;
  else if (mpVerticalViewRadioButton->isChecked())
    return 2;
  else
    return 0;
}

void GeneralSettingsPage::setWelcomePageView(int view)
{
  switch (view)
  {
    case 2:
      mpVerticalViewRadioButton->setChecked(true);
      break;
    case 1:
    default:
      mpHorizontalViewRadioButton->setChecked(true);
      break;
  }
}

QCheckBox* GeneralSettingsPage::getShowLatestNewsCheckBox()
{
  return mpShowLatestNewsCheckBox;
}

//! Slot activated when mpWorkingDirectoryBrowseButton clicked signal is raised.
//! Allows user to choose a new working directory.
void GeneralSettingsPage::selectWorkingDirectory()
{
  mpWorkingDirectoryTextBox->setText(StringHandler::getExistingDirectory(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseDirectory), NULL));
}

void GeneralSettingsPage::autoSaveIntervalValueChanged(int value)
{
  mpAutoSaveSecondsLabel->setText(tr("(%1 minute(s))").arg((double)value/60));
}

//! @class LibrariesPage
//! @brief Creates an interface for Libraries settings.

//! Constructor
//! @param pParent is the pointer to OptionsDialog
LibrariesPage::LibrariesPage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpOptionsDialog = pParent;
  // system libraries groupbox
  mpSystemLibrariesGroupBox = new QGroupBox(tr("System Libraries"));
  // system libraries note
  mpSystemLibrariesNoteLabel = new Label(tr("* The system libraries are read from the MODELICAPATH and are always read-only."));
  mpSystemLibrariesNoteLabel->setElideMode(Qt::ElideMiddle);
  // MODELICAPATH
  mpModelicaPathLabel = new Label(QString("MODELICAPATH = ").append(Helper::OpenModelicaLibrary));
  mpModelicaPathLabel->setElideMode(Qt::ElideMiddle);
  // system libraries tree
  mpSystemLibrariesTree = new QTreeWidget;
  mpSystemLibrariesTree->setItemDelegate(new ItemDelegate(mpSystemLibrariesTree));
  mpSystemLibrariesTree->setIndentation(0);
  mpSystemLibrariesTree->setColumnCount(2);
  mpSystemLibrariesTree->setTextElideMode(Qt::ElideMiddle);
  QStringList systemLabels;
  systemLabels << tr("Name") << tr("Version");
  mpSystemLibrariesTree->setHeaderLabels(systemLabels);
  connect(mpSystemLibrariesTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(openEditSystemLibrary()));
  // system libraries buttons
  mpAddSystemLibraryButton = new QPushButton(Helper::add);
  mpAddSystemLibraryButton->setAutoDefault(false);
  connect(mpAddSystemLibraryButton, SIGNAL(clicked()), SLOT(openAddSystemLibrary()));
  mpEditSystemLibraryButton = new QPushButton(Helper::edit);
  mpEditSystemLibraryButton->setAutoDefault(false);
  connect(mpEditSystemLibraryButton, SIGNAL(clicked()), SLOT(openEditSystemLibrary()));
  mpRemoveSystemLibraryButton = new QPushButton(Helper::remove);
  mpRemoveSystemLibraryButton->setAutoDefault(false);
  connect(mpRemoveSystemLibraryButton, SIGNAL(clicked()), SLOT(removeSystemLibrary()));
  // system libraries button box
  mpSystemLibrariesButtonBox = new QDialogButtonBox(Qt::Vertical);
  mpSystemLibrariesButtonBox->addButton(mpAddSystemLibraryButton, QDialogButtonBox::ActionRole);
  mpSystemLibrariesButtonBox->addButton(mpEditSystemLibraryButton, QDialogButtonBox::ActionRole);
  mpSystemLibrariesButtonBox->addButton(mpRemoveSystemLibraryButton, QDialogButtonBox::ActionRole);
  // system libraries groupbox layout
  QGridLayout *pSystemLibrariesLayout = new QGridLayout;
  pSystemLibrariesLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pSystemLibrariesLayout->addWidget(mpSystemLibrariesNoteLabel, 0, 0, 1, 2);
  pSystemLibrariesLayout->addWidget(mpModelicaPathLabel, 1, 0);
  pSystemLibrariesLayout->addWidget(mpSystemLibrariesTree, 2, 0);
  pSystemLibrariesLayout->addWidget(mpSystemLibrariesButtonBox, 2, 1);
  mpSystemLibrariesGroupBox->setLayout(pSystemLibrariesLayout);
  // force Modelica load checkbox
  mpForceModelicaLoadCheckBox = new QCheckBox(tr("Force loading of Modelica Standard Library"));
  mpForceModelicaLoadCheckBox->setToolTip(tr("This will make sure that Modelica and ModelicaReference will always load even if user has removed them from the list of system libraries."));
  mpForceModelicaLoadCheckBox->setChecked(true);
  // user libraries groupbox
  mpUserLibrariesGroupBox = new QGroupBox(tr("User Libraries"));
  // user libraries tree
  mpUserLibrariesTree = new QTreeWidget;
  mpUserLibrariesTree->setItemDelegate(new ItemDelegate(mpUserLibrariesTree));
  mpUserLibrariesTree->setIndentation(0);
  mpUserLibrariesTree->setColumnCount(2);
  mpUserLibrariesTree->setTextElideMode(Qt::ElideMiddle);
  QStringList userLabels;
  userLabels << tr("Path") << tr("Encoding");
  mpUserLibrariesTree->setHeaderLabels(userLabels);
  connect(mpUserLibrariesTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(openEditUserLibrary()));
  // user libraries buttons
  mpAddUserLibraryButton = new QPushButton(Helper::add);
  mpAddUserLibraryButton->setAutoDefault(false);
  connect(mpAddUserLibraryButton, SIGNAL(clicked()), SLOT(openAddUserLibrary()));
  mpEditUserLibraryButton = new QPushButton(Helper::edit);
  mpEditUserLibraryButton->setAutoDefault(false);
  connect(mpEditUserLibraryButton, SIGNAL(clicked()), SLOT(openEditUserLibrary()));
  mpRemoveUserLibraryButton = new QPushButton(Helper::remove);
  mpRemoveUserLibraryButton->setAutoDefault(false);
  connect(mpRemoveUserLibraryButton, SIGNAL(clicked()), SLOT(removeUserLibrary()));
  // user libraries button box
  mpUserLibrariesButtonBox = new QDialogButtonBox(Qt::Vertical);
  mpUserLibrariesButtonBox->addButton(mpAddUserLibraryButton, QDialogButtonBox::ActionRole);
  mpUserLibrariesButtonBox->addButton(mpEditUserLibraryButton, QDialogButtonBox::ActionRole);
  mpUserLibrariesButtonBox->addButton(mpRemoveUserLibraryButton, QDialogButtonBox::ActionRole);
  // user libraries groupbox layout
  QGridLayout *pUserLibrariesLayout = new QGridLayout;
  pUserLibrariesLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pUserLibrariesLayout->addWidget(mpUserLibrariesTree, 0, 0);
  pUserLibrariesLayout->addWidget(mpUserLibrariesButtonBox, 0, 1);
  mpUserLibrariesGroupBox->setLayout(pUserLibrariesLayout);
  // libraries note label
  mpLibrariesNoteLabel = new Label(tr("* The libraries changes will take effect after restart."));
  mpLibrariesNoteLabel->setElideMode(Qt::ElideMiddle);
  // main layout
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mpSystemLibrariesGroupBox);
  layout->addWidget(mpForceModelicaLoadCheckBox);
  layout->addWidget(mpUserLibrariesGroupBox);
  layout->addWidget(mpLibrariesNoteLabel);
  setLayout(layout);
}

//! Returns the System Libraries Tree instance.
QTreeWidget* LibrariesPage::getSystemLibrariesTree()
{
  return mpSystemLibrariesTree;
}

QCheckBox* LibrariesPage::getForceModelicaLoadCheckBox()
{
  return mpForceModelicaLoadCheckBox;
}

//! Returns the User Libraries Tree instance.
QTreeWidget* LibrariesPage::getUserLibrariesTree()
{
  return mpUserLibrariesTree;
}

//! Slot activated when mpAddSystemLibraryButton clicked signal is raised.
//! Creates an instance of AddLibraryWidget and show it.
void LibrariesPage::openAddSystemLibrary()
{
  AddSystemLibraryDialog *pAddSystemLibraryWidget = new AddSystemLibraryDialog(this);
  pAddSystemLibraryWidget->show();
}

//! Slot activated when mpRemoveSystemLibraryButton clicked signal is raised.
//! Removes the selected tree item
void LibrariesPage::removeSystemLibrary()
{
  if (mpSystemLibrariesTree->selectedItems().size() > 0)
  {
    mpSystemLibrariesTree->removeItemWidget(mpSystemLibrariesTree->selectedItems().at(0), 0);
    delete mpSystemLibrariesTree->selectedItems().at(0);
  }
}

//! Slot activated when mpEditSystemLibraryButton clicked signal is raised.
//! Opens the AddLibraryWidget in edit mode and pass it the selected tree item.
void LibrariesPage::openEditSystemLibrary()
{
  if (mpSystemLibrariesTree->selectedItems().size() > 0)
  {
    AddSystemLibraryDialog *pAddSystemLibraryWidget = new AddSystemLibraryDialog(this);
    pAddSystemLibraryWidget->setWindowTitle(QString(Helper::applicationName).append(" - ").append(tr("Edit System Library")));
    pAddSystemLibraryWidget->mEditFlag = true;
    int currentIndex = pAddSystemLibraryWidget->mpNameComboBox->findText(mpSystemLibrariesTree->selectedItems().at(0)->text(0), Qt::MatchExactly);
    pAddSystemLibraryWidget->mpNameComboBox->setCurrentIndex(currentIndex);
    pAddSystemLibraryWidget->mpVersionTextBox->setText(mpSystemLibrariesTree->selectedItems().at(0)->text(1));
    pAddSystemLibraryWidget->show();
  }
}

//! Slot activated when mpAddUserLibraryButton clicked signal is raised.
//! Creates an instance of AddLibraryWidget and show it.
void LibrariesPage::openAddUserLibrary()
{
  AddUserLibraryDialog *pAddUserLibraryWidget = new AddUserLibraryDialog(this);
  pAddUserLibraryWidget->show();
}

//! Slot activated when mpRemoveUserLibraryButton clicked signal is raised.
//! Removes the selected tree item
void LibrariesPage::removeUserLibrary()
{
  if (mpUserLibrariesTree->selectedItems().size() > 0)
  {
    mpUserLibrariesTree->removeItemWidget(mpUserLibrariesTree->selectedItems().at(0), 0);
    delete mpUserLibrariesTree->selectedItems().at(0);
  }
}

//! Slot activated when mpEditUserLibraryButton clicked signal is raised.
//! Opens the AddLibraryWidget in edit mode and pass it the selected tree item.
void LibrariesPage::openEditUserLibrary()
{
  if (mpUserLibrariesTree->selectedItems().size() > 0)
  {
    AddUserLibraryDialog *pAddUserLibraryWidget = new AddUserLibraryDialog(this);
    pAddUserLibraryWidget->setWindowTitle(QString(Helper::applicationName).append(" - ").append(tr("Edit User Library")));
    pAddUserLibraryWidget->mEditFlag = true;
    pAddUserLibraryWidget->mpPathTextBox->setText(mpUserLibrariesTree->selectedItems().at(0)->text(0));
    int currentIndex = pAddUserLibraryWidget->mpEncodingComboBox->findData(mpUserLibrariesTree->selectedItems().at(0)->text(1));
    if (currentIndex > -1)
      pAddUserLibraryWidget->mpEncodingComboBox->setCurrentIndex(currentIndex);
    pAddUserLibraryWidget->show();
  }
}

//! @class AddSystemLibraryDialog
//! @brief Creates an interface for Adding new System Libraries.

//! Constructor
//! @param pParent is the pointer to LibrariesPage
AddSystemLibraryDialog::AddSystemLibraryDialog(LibrariesPage *pParent)
  : QDialog(pParent, Qt::WindowTitleHint), mEditFlag(false)
{
  setWindowTitle(QString(Helper::applicationName).append(" - ").append(tr("Add System Library")));
  setAttribute(Qt::WA_DeleteOnClose);
  setModal(true);
  mpLibrariesPage = pParent;
  mpNameLabel = new Label(Helper::name);
  mpNameComboBox = new QComboBox;
  foreach (const QString &key, mpLibrariesPage->mpOptionsDialog->getMainWindow()->getOMCProxy()->getAvailableLibraries()) {
    mpNameComboBox->addItem(key,key);
  }

  mpValueLabel = new Label(tr("Version:"));
  mpVersionTextBox = new QLineEdit("default");
  mpOkButton = new QPushButton(Helper::ok);
  connect(mpOkButton, SIGNAL(clicked()), SLOT(addSystemLibrary()));
  QGridLayout *mainLayout = new QGridLayout;
  mainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  mainLayout->addWidget(mpNameLabel, 0, 0);
  mainLayout->addWidget(mpNameComboBox, 0, 1);
  mainLayout->addWidget(mpValueLabel, 1, 0);
  mainLayout->addWidget(mpVersionTextBox, 1, 1);
  mainLayout->addWidget(mpOkButton, 2, 0, 1, 2, Qt::AlignRight);
  setLayout(mainLayout);
}

//! Returns tree if the name exists in the tree's first column.
bool AddSystemLibraryDialog::nameExists(QTreeWidgetItem *pItem)
{
  QTreeWidgetItemIterator it(mpLibrariesPage->getSystemLibrariesTree());
  while (*it)
  {
    QTreeWidgetItem *pChildItem = dynamic_cast<QTreeWidgetItem*>(*it);
    // edit case
    if (pItem)
    {
      if (pChildItem != pItem)
      {
        if (pChildItem->text(0).compare(mpNameComboBox->currentText()) == 0)
        {
          return true;
        }
      }
    }
    // add case
    else
    {
      if (pChildItem->text(0).compare(mpNameComboBox->currentText()) == 0)
      {
        return true;
      }
    }
    ++it;
  }
  return false;
}

//! Slot activated when mpOkButton clicked signal is raised.
//! Add/Edit the system library in the tree.
void AddSystemLibraryDialog::addSystemLibrary()
{
  // if name text box is empty show error and return
  if (mpNameComboBox->currentText().isEmpty())
  {
    QMessageBox::critical(this, QString(Helper::applicationName).append(" - ").append(Helper::error),
                          GUIMessages::getMessage(GUIMessages::ENTER_NAME).arg("a"), Helper::ok);
    return;
  }
  // if value text box is empty show error and return
  if (mpVersionTextBox->text().isEmpty())
  {
    QMessageBox::critical(this, QString(Helper::applicationName).append(" - ").append(Helper::error),
                          GUIMessages::getMessage(GUIMessages::ENTER_NAME).arg("the value for a"), Helper::ok);
    return;
  }
  // if user is adding a new library
  if (!mEditFlag)
  {
    if (nameExists())
    {
      QMessageBox::critical(this, QString(Helper::applicationName).append(" - ").append(Helper::error),
                            GUIMessages::getMessage(GUIMessages::ITEM_ALREADY_EXISTS), Helper::ok);
      return;
    }
    QStringList values;
    values << mpNameComboBox->currentText() << mpVersionTextBox->text();
    mpLibrariesPage->getSystemLibrariesTree()->addTopLevelItem(new QTreeWidgetItem(values));
  }
  // if user is editing old library
  else if (mEditFlag)
  {
    QTreeWidgetItem *pItem = mpLibrariesPage->getSystemLibrariesTree()->selectedItems().at(0);
    if (nameExists(pItem))
    {
      QMessageBox::critical(this, QString(Helper::applicationName).append(" - ").append(Helper::error),
                            GUIMessages::getMessage(GUIMessages::ITEM_ALREADY_EXISTS), Helper::ok);
      return;
    }
    // pItem->setText(0, mpNameTextBox->text());
    pItem->setText(1, mpVersionTextBox->text());
  }
  accept();
}

//! @class AddUserLibraryDialog
//! @brief Creates an interface for Adding new User Libraries.

//! Constructor
//! @param pParent is the pointer to LibrariesPage
AddUserLibraryDialog::AddUserLibraryDialog(LibrariesPage *pParent)
  : QDialog(pParent, Qt::WindowTitleHint), mEditFlag(false)
{
  setWindowTitle(QString(Helper::applicationName).append(" - ").append(tr("Add User Library")));
  setAttribute(Qt::WA_DeleteOnClose);
  setModal(true);
  mpLibrariesPage = pParent;
  mpPathLabel = new Label(Helper::path);
  mpPathTextBox = new QLineEdit;
  mpPathBrowseButton = new QPushButton(Helper::browse);
  mpPathBrowseButton->setAutoDefault(false);
  connect(mpPathBrowseButton, SIGNAL(clicked()), SLOT(browseUserLibraryPath()));
  mpEncodingLabel = new Label(Helper::encoding);
  mpEncodingComboBox = new QComboBox;
  StringHandler::fillEncodingComboBox(mpEncodingComboBox);
  mpOkButton = new QPushButton(Helper::ok);
  connect(mpOkButton, SIGNAL(clicked()), SLOT(addUserLibrary()));
  QGridLayout *mainLayout = new QGridLayout;
  mainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  mainLayout->addWidget(mpPathLabel, 0, 0);
  mainLayout->addWidget(mpPathTextBox, 0, 1);
  mainLayout->addWidget(mpPathBrowseButton, 0, 2);
  mainLayout->addWidget(mpEncodingLabel, 1, 0);
  mainLayout->addWidget(mpEncodingComboBox, 1, 1, 1, 2);
  mainLayout->addWidget(mpOkButton, 2, 0, 1, 3, Qt::AlignRight);
  setLayout(mainLayout);
}

//! Returns tree if the name exists in the tree's first column.
bool AddUserLibraryDialog::pathExists(QTreeWidgetItem *pItem)
{
  QTreeWidgetItemIterator it(mpLibrariesPage->getUserLibrariesTree());
  while (*it)
  {
    QTreeWidgetItem *pChildItem = dynamic_cast<QTreeWidgetItem*>(*it);
    // edit case
    if (pItem)
    {
      if (pChildItem != pItem)
      {
        if (pChildItem->text(0).compare(mpPathTextBox->text()) == 0)
        {
          return true;
        }
      }
    }
    // add case
    else
    {
      if (pChildItem->text(0).compare(mpPathTextBox->text()) == 0)
      {
        return true;
      }
    }
    ++it;
  }
  return false;
}

//! Slot activated when mpPathBrowseButton clicked signal is raised.
//! Add/Edit the user library in the tree.
void AddUserLibraryDialog::browseUserLibraryPath()
{
  mpPathTextBox->setText(StringHandler::getOpenFileName(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseFile),
                                                        NULL, Helper::omFileTypes, NULL));
}

//! Slot activated when mpOkButton clicked signal is raised.
//! Add/Edit the user library in the tree.
void AddUserLibraryDialog::addUserLibrary()
{
  // if path text box is empty show error and return
  if (mpPathTextBox->text().isEmpty())
  {
    QMessageBox::critical(this, QString(Helper::applicationName).append(" - ").append(Helper::error),
                          tr("Please enter the file path."), Helper::ok);
    return;
  }
  // if user is adding a new library
  if (!mEditFlag)
  {
    if (pathExists())
    {
      QMessageBox::critical(this, QString(Helper::applicationName).append(" - ").append(Helper::error),
                            GUIMessages::getMessage(GUIMessages::ITEM_ALREADY_EXISTS), Helper::ok);
      return;
    }
    QStringList values;
    values << mpPathTextBox->text() << mpEncodingComboBox->itemData(mpEncodingComboBox->currentIndex()).toString();
    mpLibrariesPage->getUserLibrariesTree()->addTopLevelItem(new QTreeWidgetItem(values));
  }
  // if user is editing old library
  else if (mEditFlag)
  {
    QTreeWidgetItem *pItem = mpLibrariesPage->getUserLibrariesTree()->selectedItems().at(0);
    if (pathExists(pItem))
    {
      QMessageBox::critical(this, QString(Helper::applicationName).append(" - ").append(Helper::error),
                            GUIMessages::getMessage(GUIMessages::ITEM_ALREADY_EXISTS), Helper::ok);
      return;
    }
    pItem->setText(0, mpPathTextBox->text());
    pItem->setText(1, mpEncodingComboBox->itemData(mpEncodingComboBox->currentIndex()).toString());
  }
  accept();
}

//! @class ModelicaTextSettings
//! @brief Defines the settings like font, style, keywords colors etc. for the Modelica Text.

//! Constructor
ModelicaTextSettings::ModelicaTextSettings()
{
  // set default values, will be handy if we are unable to create the xml file
  setFontFamily(Helper::monospacedFontInfo.family());
  setFontSize(Helper::monospacedFontInfo.pointSizeF());
  setTextRuleColor(QColor(0, 0, 0));                // black
  setKeywordRuleColor(QColor(139, 0, 0));           // dark red
  setTypeRuleColor(QColor(255, 10, 10));            // red
  setFunctionRuleColor(QColor(0, 0, 255));          // blue
  setQuotesRuleColor(QColor(0, 139, 0));            // dark green
  setCommentRuleColor(QColor(0, 150, 0));           // dark green
  setNumberRuleColor(QColor(139, 0, 139));          // purple
  setTLMTagRuleColor(QColor(0, 0, 255));            // blue
  setTLMElementRuleColor(QColor(0, 0, 255));        // blue
  setTLMQuotesRuleColor(QColor(139, 0, 0));         // dark red
}

//! Sets the font for the Modelica Text.
//! @param fontFamily is the font to set.
void ModelicaTextSettings::setFontFamily(QString fontFamily)
{
  mFontFamily = fontFamily;
}

//! Returns the Modelica Text font.
//! @return mFontFamily the font family.
QString ModelicaTextSettings::getFontFamily()
{
  return mFontFamily;
}

//! Sets the font size for the Modelica Text.
//! @param fontSize is the font size to set.
void ModelicaTextSettings::setFontSize(double fontSize)
{
  mFontSize = fontSize;
}

//! Returns the Modelica Text font size.
//! @return mFontSize the font size.
double ModelicaTextSettings::getFontSize()
{
  return mFontSize;
}

//! Sets the color for the Modelica Text.
//! @param color is the color to set.
void ModelicaTextSettings::setTextRuleColor(QColor color)
{
  mTextRuleColor = color;
}

//! Returns the Modelica Text color.
//! @return mTextRuleColor the color.
QColor ModelicaTextSettings::getTextRuleColor()
{
  return mTextRuleColor;
}

//! Sets the color for the Modelica Text numbers.
//! @param color is the color to set.
void ModelicaTextSettings::setNumberRuleColor(QColor color)
{
  mNumberRuleColor = color;
}

//! Returns the Modelica Text numbers color.
//! @return mNumberRuleColor the color.
QColor ModelicaTextSettings::getNumberRuleColor()
{
  return mNumberRuleColor;
}

//! Sets the color for the Modelica Text keywords.
//! @param color is the color to set.
void ModelicaTextSettings::setKeywordRuleColor(QColor color)
{
  mKeyWordRuleColor = color;
}

//! Returns the Modelica Text keyword color.
//! @return mKeyWordRuleColor the color.
QColor ModelicaTextSettings::getKeywordRuleColor()
{
  return mKeyWordRuleColor;
}

//! Sets the color for the Modelica Text types.
//! @param color is the color to set.
void ModelicaTextSettings::setTypeRuleColor(QColor color)
{
  mTypeRuleColor = color;
}

//! Returns the Modelica Text types color.
//! @return mTypeRuleColor the color.
QColor ModelicaTextSettings::getTypeRuleColor()
{
  return mTypeRuleColor;
}

//! Sets the color for the Modelica Text functions.
//! @param color is the color to set.
void ModelicaTextSettings::setFunctionRuleColor(QColor color)
{
  mFunctionRuleColor = color;
}

//! Returns the Modelica Text functions color.
//! @return mFunctionRuleColor the color.
QColor ModelicaTextSettings::getFunctionRuleColor()
{
  return mFunctionRuleColor;
}

//! Sets the color for the Modelica Text quotes.
//! @param color is the color to set.
void ModelicaTextSettings::setQuotesRuleColor(QColor color)
{
  mQuotesRuleColor = color;
}

//! Returns the Modelica Text quotes color.
//! @return mQuotesRuleColor the color.
QColor ModelicaTextSettings::getQuotesRuleColor()
{
  return mQuotesRuleColor;
}

//! Sets the color for the Modelica Text comments.
//! @param color is the color to set.
void ModelicaTextSettings::setCommentRuleColor(QColor color)
{
  mCommentRuleColor = color;
}

//! Returns the Modelica Text comments color.
//! @return mCommentRuleColor the color.
QColor ModelicaTextSettings::getCommentRuleColor()
{
  return mCommentRuleColor;
}

//! Sets the color for the TLM Tag.
//! @param color is the color to set.
void ModelicaTextSettings::setTLMTagRuleColor(QColor color)
{
  mTLMTagRuleColor = color;
}

//! Returns the TLM Tag color.
//! @return mTLMTagRuleColor the color.
QColor ModelicaTextSettings::getTLMTagRuleColor()
{
  return mTLMTagRuleColor;
}

//! Sets the color for the TLM Element.
//! @param color is the color to set.
void ModelicaTextSettings::setTLMElementRuleColor(QColor color)
{
  mTLMElementRuleColor = color;
}

//! Returns the TLM  Element color.
//! @return mTLMElementRuleColor the color.
QColor ModelicaTextSettings::getTLMElementRuleColor()
{
  return mTLMElementRuleColor;
}

//! Sets the color for the TLM Quotes.
//! @param color is the color to set.
void ModelicaTextSettings::setTLMQuotesRuleColor(QColor color)
{
  mTLMQuotesRuleColor = color;
}

//! Returns the TLM  Quotes color.
//! @return mTLMQuotesRuleColor the color.
QColor ModelicaTextSettings::getTLMQuotesRuleColor()
{
  return mTLMQuotesRuleColor;
}
//! @class ModelicaTextEditorPage
//! @brief Creates an interface for Modelica Text settings.

//! Constructor
//! @param pParent is the pointer to OptionsDialog
ModelicaTextEditorPage::ModelicaTextEditorPage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpOptionsDialog = pParent;
  // general groupbox
  mpGeneralGroupBox = new QGroupBox(Helper::general);
  // syntax highlighting checkbox
  mpSyntaxHighlightingCheckbox = new QCheckBox(tr("Enable Syntax Highlighting"));
  mpSyntaxHighlightingCheckbox->setChecked(true);
  // line wrap checkbox
  mpLineWrappingCheckbox = new QCheckBox(tr("Enable Line Wrapping"));
  mpLineWrappingCheckbox->setChecked(true);
  // fonts & colors groupbox
  mpFontColorsGroupBox = new QGroupBox(tr("Font and Colors"));
  // font family combobox
  mpFontFamilyLabel = new Label(tr("Font Family:"));
  mpFontFamilyComboBox = new QFontComboBox;
  int currentIndex;
  currentIndex = mpFontFamilyComboBox->findText(mpOptionsDialog->getModelicaTextSettings()->getFontFamily(), Qt::MatchExactly);
  mpFontFamilyComboBox->setCurrentIndex(currentIndex);
  connect(mpFontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), SLOT(fontFamilyChanged(QFont)));
  // font size combobox
  mpFontSizeLabel = new Label(tr("Font Size:"));
  mpFontSizeSpinBox = new DoubleSpinBox;
  mpFontSizeSpinBox->setRange(6, std::numeric_limits<double>::max());
  mpFontSizeSpinBox->setValue(mpOptionsDialog->getModelicaTextSettings()->getFontSize());
  mpFontSizeSpinBox->setSingleStep(1);
  connect(mpFontSizeSpinBox, SIGNAL(valueChanged(double)), SLOT(fontSizeChanged(double)));
  // Item color label and pick color button
  mpItemColorLabel = new Label(tr("Item Color:"));
  mpItemColorPickButton = new QPushButton(Helper::pickColor);
  mpItemColorPickButton->setAutoDefault(false);
  connect(mpItemColorPickButton, SIGNAL(clicked()), SLOT(pickColor()));
  // Items list
  mpItemsLabel = new Label(tr("Items:"));
  mpItemsList = new QListWidget;
  mpItemsList->setItemDelegate(new ItemDelegate(mpItemsList));
  mpItemsList->setMaximumHeight(90);
  // Add items to list
  addListItems();
  // make first item in the list selected
  mpItemsList->setCurrentRow(0, QItemSelectionModel::Select);
  // preview textbox
  mpPreviewLabel = new Label(tr("Preview:"));
  mpPreviewPlainTextBox = new QPlainTextEdit;
  mpPreviewPlainTextBox->setTabStopWidth(Helper::tabWidth);
  mpPreviewPlainTextBox->setPlainText(getPreviewText());
  // highlight preview textbox
  ModelicaTextHighlighter *highlighter;
  highlighter = new ModelicaTextHighlighter(mpOptionsDialog->getModelicaTextSettings(), 0, mpPreviewPlainTextBox->document());
  connect(this, SIGNAL(updatePreview()), highlighter, SLOT(settingsChanged()));
  connect(mpOptionsDialog, SIGNAL(modelicaTextSettingsChanged()), highlighter, SLOT(settingsChanged()));
  // set general groupbox layout
  QGridLayout *pGeneralGroupBoxLayout = new QGridLayout;
  pGeneralGroupBoxLayout->addWidget(mpSyntaxHighlightingCheckbox, 0, 0);
  pGeneralGroupBoxLayout->addWidget(mpLineWrappingCheckbox, 1, 0);
  mpGeneralGroupBox->setLayout(pGeneralGroupBoxLayout);
  // set fonts & colors groupbox layout
  QGridLayout *pFontsColorsGroupBoxLayout = new QGridLayout;
  pFontsColorsGroupBoxLayout->addWidget(mpFontFamilyLabel, 0, 0);
  pFontsColorsGroupBoxLayout->addWidget(mpFontSizeLabel, 0, 1);
  pFontsColorsGroupBoxLayout->addWidget(mpFontFamilyComboBox, 1, 0);
  pFontsColorsGroupBoxLayout->addWidget(mpFontSizeSpinBox, 1, 1);
  pFontsColorsGroupBoxLayout->addWidget(mpItemsLabel, 2, 0);
  pFontsColorsGroupBoxLayout->addWidget(mpItemColorLabel, 2, 1);
  pFontsColorsGroupBoxLayout->addWidget(mpItemsList, 3, 0);
  pFontsColorsGroupBoxLayout->addWidget(mpItemColorPickButton, 3, 1, Qt::AlignTop);
  pFontsColorsGroupBoxLayout->addWidget(mpPreviewLabel, 4, 0, 1, 2);
  pFontsColorsGroupBoxLayout->addWidget(mpPreviewPlainTextBox, 5, 0, 1, 2);
  mpFontColorsGroupBox->setLayout(pFontsColorsGroupBoxLayout);
  // set the layout
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mpGeneralGroupBox);
  layout->addWidget(mpFontColorsGroupBox);
  setLayout(layout);
}

//! Adds the Modelica Text settings rules to the mpItemsList.
void ModelicaTextEditorPage::addListItems()
{
  // don't change the Data of items as it is being used in ModelicaTextEditorPage::pickColor slot to identify the items
  mpTextItem = new QListWidgetItem(mpItemsList);
  mpTextItem->setText("Text");
  mpTextItem->setData(Qt::UserRole, "Text");
  mpTextItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getTextRuleColor());

  mpNumberItem = new QListWidgetItem(mpItemsList);
  mpNumberItem->setText("Number");
  mpNumberItem->setData(Qt::UserRole, "Number");
  mpNumberItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getNumberRuleColor());

  mpKeywordItem = new QListWidgetItem(mpItemsList);
  mpKeywordItem->setText("Keyword");
  mpKeywordItem->setData(Qt::UserRole, "Keyword");
  mpKeywordItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getKeywordRuleColor());

  mpTypeItem = new QListWidgetItem(mpItemsList);
  mpTypeItem->setText("Type");
  mpTypeItem->setData(Qt::UserRole, "Type");
  mpTypeItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getTypeRuleColor());

  mpFunctionItem = new QListWidgetItem(mpItemsList);
  mpFunctionItem->setText("Function");
  mpFunctionItem->setData(Qt::UserRole, "Function");
  mpFunctionItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getFunctionRuleColor());

  mpQuotesItem = new QListWidgetItem(mpItemsList);
  mpQuotesItem->setText("Quotes");
  mpQuotesItem->setData(Qt::UserRole, "Quotes");
  mpQuotesItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getQuotesRuleColor());

  mpCommentItem = new QListWidgetItem(mpItemsList);
  mpCommentItem->setText("Comment");
  mpCommentItem->setData(Qt::UserRole, "Comment");
  mpCommentItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getCommentRuleColor());

  mpTLMTagItem = new QListWidgetItem(mpItemsList);
  mpTLMTagItem->setText("TLM Tag");
  mpTLMTagItem->setData(Qt::UserRole, "TLM Tag");
  mpTLMTagItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getTLMTagRuleColor());

  mpTLMElementItem = new QListWidgetItem(mpItemsList);
  mpTLMElementItem->setText("TLM Element");
  mpTLMElementItem->setData(Qt::UserRole, "TLM Element");
  mpTLMElementItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getTLMElementRuleColor());

  mpTLMQuotesItem = new QListWidgetItem(mpItemsList);
  mpTLMQuotesItem->setText("TLM Quotes");
  mpTLMQuotesItem->setData(Qt::UserRole, "TLM Quotes");
  mpTLMQuotesItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getTLMQuotesRuleColor());
}

//! Returns the preview text.
QString ModelicaTextEditorPage::getPreviewText()
{
  QString previewText;
  previewText.append("class HelloWorld /* block\n"
                     "comment */\n"
                     "\tReal x(start = 1); // Line comment\n"
                     "\tparameter Real a = 1.573;\n"
                     "\tString str = \"a\\\"bc\n"
                     "123\";\n"
                     "equation\n"
                     "\tder(x) = - a * x;\n"
                     "end HelloWorld;\n");

  return previewText;
}

//! Initialize all fields with default values.
void ModelicaTextEditorPage::initializeFields()
{
  int currentIndex;
  // select font family item
  currentIndex = mpFontFamilyComboBox->findText(mpOptionsDialog->getModelicaTextSettings()->getFontFamily(), Qt::MatchExactly);
  mpFontFamilyComboBox->setCurrentIndex(currentIndex);
  // select font size item
  mpFontSizeSpinBox->setValue(mpOptionsDialog->getModelicaTextSettings()->getFontSize());
  // make first item in the list selected
  mpItemsList->setCurrentRow(0, QItemSelectionModel::Select);
  // refresh the preview textbox
  mpPreviewPlainTextBox->setPlainText(getPreviewText());
  // update list items
  mpTextItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getTextRuleColor());
  mpNumberItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getNumberRuleColor());
  mpKeywordItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getKeywordRuleColor());
  mpTypeItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getTypeRuleColor());
  mpFunctionItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getFunctionRuleColor());
  mpQuotesItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getQuotesRuleColor());
  mpCommentItem->setForeground(mpOptionsDialog->getModelicaTextSettings()->getCommentRuleColor());
}

QCheckBox* ModelicaTextEditorPage::getSyntaxHighlightingCheckbox()
{
  return mpSyntaxHighlightingCheckbox;
}

QCheckBox* ModelicaTextEditorPage::getLineWrappingCheckbox()
{
  return mpLineWrappingCheckbox;
}

//! Changes the font family when mpFontFamilyComboBox currentFontChanged signal is raised.
void ModelicaTextEditorPage::fontFamilyChanged(QFont font)
{
  mpOptionsDialog->getModelicaTextSettings()->setFontFamily(font.family());
  emit updatePreview();
}

//! Changes the font size when mpFontSizeComboBox currentIndexChanged signal is raised.
void ModelicaTextEditorPage::fontSizeChanged(double newValue)
{
  mpOptionsDialog->getModelicaTextSettings()->setFontSize(newValue);
  emit updatePreview();
}

//! Picks a color for one of the Modelica Text Settings rules.
//! This method is called when mpColorPickButton clicked signal raised.
void ModelicaTextEditorPage::pickColor()
{
  QListWidgetItem *item = mpItemsList->currentItem();
  QColor initialColor;
  // get the color of the item
  if (item->data(Qt::UserRole).toString().toLower().compare("text") == 0)
  {
    initialColor = mpOptionsDialog->getModelicaTextSettings()->getTextRuleColor();
  }
  else if(item->data(Qt::UserRole).toString().toLower().compare("number") == 0)
  {
    initialColor = mpOptionsDialog->getModelicaTextSettings()->getNumberRuleColor();
  }
  else if (item->data(Qt::UserRole).toString().toLower().compare("keyword") == 0)
  {
    initialColor = mpOptionsDialog->getModelicaTextSettings()->getKeywordRuleColor();
  }
  else if(item->data(Qt::UserRole).toString().toLower().compare("type") == 0)
  {
    initialColor = mpOptionsDialog->getModelicaTextSettings()->getTypeRuleColor();
  }
  else if(item->data(Qt::UserRole).toString().toLower().compare("function") == 0)
  {
    initialColor = mpOptionsDialog->getModelicaTextSettings()->getFunctionRuleColor();
  }
  else if(item->data(Qt::UserRole).toString().toLower().compare("quotes") == 0)
  {
    initialColor = mpOptionsDialog->getModelicaTextSettings()->getQuotesRuleColor();
  }
  else if(item->data(Qt::UserRole).toString().toLower().compare("comment") == 0)
  {
    initialColor = mpOptionsDialog->getModelicaTextSettings()->getCommentRuleColor();
  }
  QColor color = QColorDialog::getColor(initialColor);
  if (!color.isValid())
    return;
  // set the color of the item
  if (item->data(Qt::UserRole).toString().toLower().compare("text") == 0)
  {
    mpOptionsDialog->getModelicaTextSettings()->setTextRuleColor(color);
  }
  else if(item->data(Qt::UserRole).toString().toLower().compare("number") == 0)
  {
    mpOptionsDialog->getModelicaTextSettings()->setNumberRuleColor(color);
  }
  else if (item->data(Qt::UserRole).toString().toLower().compare("keyword") == 0)
  {
    mpOptionsDialog->getModelicaTextSettings()->setKeywordRuleColor(color);
  }
  else if(item->data(Qt::UserRole).toString().toLower().compare("type") == 0)
  {
    mpOptionsDialog->getModelicaTextSettings()->setTypeRuleColor(color);
  }
  else if(item->data(Qt::UserRole).toString().toLower().compare("function") == 0)
  {
    mpOptionsDialog->getModelicaTextSettings()->setFunctionRuleColor(color);
  }
  else if(item->data(Qt::UserRole).toString().toLower().compare("quotes") == 0)
  {
    mpOptionsDialog->getModelicaTextSettings()->setQuotesRuleColor(color);
  }
  else if(item->data(Qt::UserRole).toString().toLower().compare("comment") == 0)
  {
    mpOptionsDialog->getModelicaTextSettings()->setCommentRuleColor(color);
  }
  // change the color of item
  item->setForeground(color);
  emit updatePreview();
}

GraphicalViewsPage::GraphicalViewsPage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpOptionsDialog = pParent;
  // graphical view tab widget
  mpGraphicalViewsTabWidget = new QTabWidget;
  // Icon View Widget
  mpIconViewWidget = new QWidget;
  // create Icon View extent points group box
  mpIconViewExtentGroupBox = new QGroupBox(Helper::extent);
  mpIconViewLeftLabel = new Label(QString(Helper::left).append(":"));
  mpIconViewLeftSpinBox = new DoubleSpinBox;
  mpIconViewLeftSpinBox->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  mpIconViewLeftSpinBox->setValue(-100);
  mpIconViewLeftSpinBox->setSingleStep(10);
  mpIconViewBottomLabel = new Label(Helper::bottom);
  mpIconViewBottomSpinBox = new DoubleSpinBox;
  mpIconViewBottomSpinBox->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  mpIconViewBottomSpinBox->setValue(-100);
  mpIconViewBottomSpinBox->setSingleStep(10);
  mpIconViewRightLabel = new Label(QString(Helper::right).append(":"));
  mpIconViewRightSpinBox = new DoubleSpinBox;
  mpIconViewRightSpinBox->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  mpIconViewRightSpinBox->setValue(100);
  mpIconViewRightSpinBox->setSingleStep(10);
  mpIconViewTopLabel = new Label(Helper::top);
  mpIconViewTopSpinBox = new DoubleSpinBox;
  mpIconViewTopSpinBox->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  mpIconViewTopSpinBox->setValue(100);
  mpIconViewTopSpinBox->setSingleStep(10);
  // set the Icon View extent group box layout
  QGridLayout *pIconViewExtentLayout = new QGridLayout;
  pIconViewExtentLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pIconViewExtentLayout->setColumnStretch(1, 1);
  pIconViewExtentLayout->setColumnStretch(3, 1);
  pIconViewExtentLayout->addWidget(mpIconViewLeftLabel, 0, 0);
  pIconViewExtentLayout->addWidget(mpIconViewLeftSpinBox, 0, 1);
  pIconViewExtentLayout->addWidget(mpIconViewBottomLabel, 0, 2);
  pIconViewExtentLayout->addWidget(mpIconViewBottomSpinBox, 0, 3);
  pIconViewExtentLayout->addWidget(mpIconViewRightLabel, 1, 0);
  pIconViewExtentLayout->addWidget(mpIconViewRightSpinBox, 1, 1);
  pIconViewExtentLayout->addWidget(mpIconViewTopLabel, 1, 2);
  pIconViewExtentLayout->addWidget(mpIconViewTopSpinBox, 1, 3);
  mpIconViewExtentGroupBox->setLayout(pIconViewExtentLayout);
  // create the Icon View grid group box
  mpIconViewGridGroupBox = new QGroupBox(Helper::grid);
  mpIconViewGridHorizontalLabel = new Label(QString(Helper::horizontal).append(":"));
  mpIconViewGridHorizontalSpinBox = new DoubleSpinBox;
  mpIconViewGridHorizontalSpinBox->setRange(0, std::numeric_limits<double>::max());
  mpIconViewGridHorizontalSpinBox->setValue(2);
  mpIconViewGridHorizontalSpinBox->setSingleStep(1);
  mpIconViewGridVerticalLabel = new Label(QString(Helper::vertical).append(":"));
  mpIconViewGridVerticalSpinBox = new DoubleSpinBox;
  mpIconViewGridVerticalSpinBox->setRange(0, std::numeric_limits<double>::max());
  mpIconViewGridVerticalSpinBox->setValue(2);
  mpIconViewGridVerticalSpinBox->setSingleStep(1);
  // set the Icon View grid group box layout
  QGridLayout *pIconViewGridLayout = new QGridLayout;
  pIconViewGridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pIconViewGridLayout->setColumnStretch(1, 1);
  pIconViewGridLayout->addWidget(mpIconViewGridHorizontalLabel, 0, 0);
  pIconViewGridLayout->addWidget(mpIconViewGridHorizontalSpinBox, 0, 1);
  pIconViewGridLayout->addWidget(mpIconViewGridVerticalLabel, 1, 0);
  pIconViewGridLayout->addWidget(mpIconViewGridVerticalSpinBox, 1, 1);
  mpIconViewGridGroupBox->setLayout(pIconViewGridLayout);
  // create the Icon View Component group box
  mpIconViewComponentGroupBox = new QGroupBox(Helper::component);
  mpIconViewScaleFactorLabel = new Label(Helper::scaleFactor);
  mpIconViewScaleFactorSpinBox = new DoubleSpinBox;
  mpIconViewScaleFactorSpinBox->setRange(0, std::numeric_limits<double>::max());
  mpIconViewScaleFactorSpinBox->setValue(0.1);
  mpIconViewScaleFactorSpinBox->setSingleStep(0.1);
  mpIconViewPreserveAspectRatioCheckBox = new QCheckBox(Helper::preserveAspectRatio);
  mpIconViewPreserveAspectRatioCheckBox->setChecked(true);
  // set the Icon View component group box layout
  QGridLayout *pIconViewComponentLayout = new QGridLayout;
  pIconViewComponentLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pIconViewComponentLayout->setColumnStretch(1, 1);
  pIconViewComponentLayout->addWidget(mpIconViewScaleFactorLabel, 0, 0);
  pIconViewComponentLayout->addWidget(mpIconViewScaleFactorSpinBox, 0, 1);
  pIconViewComponentLayout->addWidget(mpIconViewPreserveAspectRatioCheckBox, 1, 0, 1, 2);
  mpIconViewComponentGroupBox->setLayout(pIconViewComponentLayout);
  // Icon View Widget Layout
  QVBoxLayout *pIconViewMainLayout = new QVBoxLayout;
  pIconViewMainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pIconViewMainLayout->addWidget(mpIconViewExtentGroupBox);
  pIconViewMainLayout->addWidget(mpIconViewGridGroupBox);
  pIconViewMainLayout->addWidget(mpIconViewComponentGroupBox);
  mpIconViewWidget->setLayout(pIconViewMainLayout);
  // add Icon View Widget as a tab
  mpGraphicalViewsTabWidget->addTab(mpIconViewWidget, tr("Icon View"));
  // Digram View Widget
  mpDiagramViewWidget = new QWidget;
  // create Diagram View extent points group box
  mpDiagramViewExtentGroupBox = new QGroupBox(Helper::extent);
  mpDiagramViewLeftLabel = new Label(QString(Helper::left).append(":"));
  mpDiagramViewLeftSpinBox = new DoubleSpinBox;
  mpDiagramViewLeftSpinBox->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  mpDiagramViewLeftSpinBox->setValue(-100);
  mpDiagramViewLeftSpinBox->setSingleStep(10);
  mpDiagramViewBottomLabel = new Label(Helper::bottom);
  mpDiagramViewBottomSpinBox = new DoubleSpinBox;
  mpDiagramViewBottomSpinBox->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  mpDiagramViewBottomSpinBox->setValue(-100);
  mpDiagramViewBottomSpinBox->setSingleStep(10);
  mpDiagramViewRightLabel = new Label(QString(Helper::right).append(":"));
  mpDiagramViewRightSpinBox = new DoubleSpinBox;
  mpDiagramViewRightSpinBox->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  mpDiagramViewRightSpinBox->setValue(100);
  mpDiagramViewRightSpinBox->setSingleStep(10);
  mpDiagramViewTopLabel = new Label(Helper::top);
  mpDiagramViewTopSpinBox = new DoubleSpinBox;
  mpDiagramViewTopSpinBox->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  mpDiagramViewTopSpinBox->setValue(100);
  mpDiagramViewTopSpinBox->setSingleStep(10);
  // set the Diagram View extent group box layout
  QGridLayout *pDiagramViewExtentLayout = new QGridLayout;
  pDiagramViewExtentLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pDiagramViewExtentLayout->setColumnStretch(1, 1);
  pDiagramViewExtentLayout->setColumnStretch(3, 1);
  pDiagramViewExtentLayout->addWidget(mpDiagramViewLeftLabel, 0, 0);
  pDiagramViewExtentLayout->addWidget(mpDiagramViewLeftSpinBox, 0, 1);
  pDiagramViewExtentLayout->addWidget(mpDiagramViewBottomLabel, 0, 2);
  pDiagramViewExtentLayout->addWidget(mpDiagramViewBottomSpinBox, 0, 3);
  pDiagramViewExtentLayout->addWidget(mpDiagramViewRightLabel, 1, 0);
  pDiagramViewExtentLayout->addWidget(mpDiagramViewRightSpinBox, 1, 1);
  pDiagramViewExtentLayout->addWidget(mpDiagramViewTopLabel, 1, 2);
  pDiagramViewExtentLayout->addWidget(mpDiagramViewTopSpinBox, 1, 3);
  mpDiagramViewExtentGroupBox->setLayout(pDiagramViewExtentLayout);
  // create the Diagram View grid group box
  mpDiagramViewGridGroupBox = new QGroupBox(Helper::grid);
  mpDiagramViewGridHorizontalLabel = new Label(QString(Helper::horizontal).append(":"));
  mpDiagramViewGridHorizontalSpinBox = new DoubleSpinBox;
  mpDiagramViewGridHorizontalSpinBox->setRange(0, std::numeric_limits<double>::max());
  mpDiagramViewGridHorizontalSpinBox->setValue(2);
  mpDiagramViewGridHorizontalSpinBox->setSingleStep(1);
  mpDiagramViewGridVerticalLabel = new Label(QString(Helper::vertical).append(":"));
  mpDiagramViewGridVerticalSpinBox = new DoubleSpinBox;
  mpDiagramViewGridVerticalSpinBox->setRange(0, std::numeric_limits<double>::max());
  mpDiagramViewGridVerticalSpinBox->setValue(2);
  mpDiagramViewGridVerticalSpinBox->setSingleStep(1);
  // set the Diagram View grid group box layout
  QGridLayout *pDiagramViewGridLayout = new QGridLayout;
  pDiagramViewGridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pDiagramViewGridLayout->setColumnStretch(1, 1);
  pDiagramViewGridLayout->addWidget(mpDiagramViewGridHorizontalLabel, 0, 0);
  pDiagramViewGridLayout->addWidget(mpDiagramViewGridHorizontalSpinBox, 0, 1);
  pDiagramViewGridLayout->addWidget(mpDiagramViewGridVerticalLabel, 1, 0);
  pDiagramViewGridLayout->addWidget(mpDiagramViewGridVerticalSpinBox, 1, 1);
  mpDiagramViewGridGroupBox->setLayout(pDiagramViewGridLayout);
  // create the Diagram View Component group box
  mpDiagramViewComponentGroupBox = new QGroupBox(Helper::component);
  mpDiagramViewScaleFactorLabel = new Label(Helper::scaleFactor);
  mpDiagramViewScaleFactorSpinBox = new DoubleSpinBox;
  mpDiagramViewScaleFactorSpinBox->setRange(0, std::numeric_limits<double>::max());
  mpDiagramViewScaleFactorSpinBox->setValue(0.1);
  mpDiagramViewScaleFactorSpinBox->setSingleStep(0.1);
  mpDiagramViewPreserveAspectRatioCheckBox = new QCheckBox(Helper::preserveAspectRatio);
  mpDiagramViewPreserveAspectRatioCheckBox->setChecked(true);
  // set the Diagram View component group box layout
  QGridLayout *pDiagramViewComponentLayout = new QGridLayout;
  pDiagramViewComponentLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pDiagramViewComponentLayout->setColumnStretch(1, 1);
  pDiagramViewComponentLayout->addWidget(mpDiagramViewScaleFactorLabel, 0, 0);
  pDiagramViewComponentLayout->addWidget(mpDiagramViewScaleFactorSpinBox, 0, 1);
  pDiagramViewComponentLayout->addWidget(mpDiagramViewPreserveAspectRatioCheckBox, 1, 0, 1, 2);
  mpDiagramViewComponentGroupBox->setLayout(pDiagramViewComponentLayout);
  // Diagram View Widget Layout
  QVBoxLayout *pDiagramViewMainLayout = new QVBoxLayout;
  pDiagramViewMainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pDiagramViewMainLayout->addWidget(mpDiagramViewExtentGroupBox);
  pDiagramViewMainLayout->addWidget(mpDiagramViewGridGroupBox);
  pDiagramViewMainLayout->addWidget(mpDiagramViewComponentGroupBox);
  mpDiagramViewWidget->setLayout(pDiagramViewMainLayout);
  // add Diagram View Widget as a tab
  mpGraphicalViewsTabWidget->addTab(mpDiagramViewWidget, tr("Diagram View"));
  // set Main Layout
  QHBoxLayout *pHBoxLayout = new QHBoxLayout;
  pHBoxLayout->setContentsMargins(0, 0, 0, 0);
  pHBoxLayout->addWidget(mpGraphicalViewsTabWidget);
  setLayout(pHBoxLayout);
}

void GraphicalViewsPage::setIconViewExtentLeft(double extentLeft)
{
  mpIconViewLeftSpinBox->setValue(extentLeft);
}

double GraphicalViewsPage::getIconViewExtentLeft()
{
  return mpIconViewLeftSpinBox->value();
}

void GraphicalViewsPage::setIconViewExtentBottom(double extentBottom)
{
  mpIconViewBottomSpinBox->setValue(extentBottom);
}

double GraphicalViewsPage::getIconViewExtentBottom()
{
  return mpIconViewBottomSpinBox->value();
}

void GraphicalViewsPage::setIconViewExtentRight(double extentRight)
{
  mpIconViewRightSpinBox->setValue(extentRight);
}

double GraphicalViewsPage::getIconViewExtentRight()
{
  return mpIconViewRightSpinBox->value();
}

void GraphicalViewsPage::setIconViewExtentTop(double extentTop)
{
  mpIconViewTopSpinBox->setValue(extentTop);
}

double GraphicalViewsPage::getIconViewExtentTop()
{
  return mpIconViewTopSpinBox->value();
}

void GraphicalViewsPage::setIconViewGridHorizontal(double gridHorizontal)
{
  mpIconViewGridHorizontalSpinBox->setValue(gridHorizontal);
}

double GraphicalViewsPage::getIconViewGridHorizontal()
{
  return mpIconViewGridHorizontalSpinBox->value();
}

void GraphicalViewsPage::setIconViewGridVertical(double gridVertical)
{
  mpIconViewGridVerticalSpinBox->setValue(gridVertical);
}

double GraphicalViewsPage::getIconViewGridVertical()
{
  return mpIconViewGridVerticalSpinBox->value();
}

void GraphicalViewsPage::setIconViewScaleFactor(double scaleFactor)
{
  mpIconViewScaleFactorSpinBox->setValue(scaleFactor);
}

double GraphicalViewsPage::getIconViewScaleFactor()
{
  return mpIconViewScaleFactorSpinBox->value();
}

void GraphicalViewsPage::setIconViewPreserveAspectRation(bool preserveAspectRation)
{
  mpIconViewPreserveAspectRatioCheckBox->setChecked(preserveAspectRation);
}

bool GraphicalViewsPage::getIconViewPreserveAspectRation()
{
  return mpIconViewPreserveAspectRatioCheckBox->isChecked();
}

void GraphicalViewsPage::setDiagramViewExtentLeft(double extentLeft)
{
  mpDiagramViewLeftSpinBox->setValue(extentLeft);
}

double GraphicalViewsPage::getDiagramViewExtentLeft()
{
  return mpDiagramViewLeftSpinBox->value();
}

void GraphicalViewsPage::setDiagramViewExtentBottom(double extentBottom)
{
  mpDiagramViewBottomSpinBox->setValue(extentBottom);
}

double GraphicalViewsPage::getDiagramViewExtentBottom()
{
  return mpDiagramViewBottomSpinBox->value();
}

void GraphicalViewsPage::setDiagramViewExtentRight(double extentRight)
{
  mpDiagramViewRightSpinBox->setValue(extentRight);
}

double GraphicalViewsPage::getDiagramViewExtentRight()
{
  return mpDiagramViewRightSpinBox->value();
}

void GraphicalViewsPage::setDiagramViewExtentTop(double extentTop)
{
  mpDiagramViewTopSpinBox->setValue(extentTop);
}

double GraphicalViewsPage::getDiagramViewExtentTop()
{
  return mpDiagramViewTopSpinBox->value();
}

void GraphicalViewsPage::setDiagramViewGridHorizontal(double gridHorizontal)
{
  mpDiagramViewGridHorizontalSpinBox->setValue(gridHorizontal);
}

double GraphicalViewsPage::getDiagramViewGridHorizontal()
{
  return mpDiagramViewGridHorizontalSpinBox->value();
}

void GraphicalViewsPage::setDiagramViewGridVertical(double gridVertical)
{
  mpDiagramViewGridVerticalSpinBox->setValue(gridVertical);
}

double GraphicalViewsPage::getDiagramViewGridVertical()
{
  return mpDiagramViewGridVerticalSpinBox->value();
}

void GraphicalViewsPage::setDiagramViewScaleFactor(double scaleFactor)
{
  mpDiagramViewScaleFactorSpinBox->setValue(scaleFactor);
}

double GraphicalViewsPage::getDiagramViewScaleFactor()
{
  return mpDiagramViewScaleFactorSpinBox->value();
}

void GraphicalViewsPage::setDiagramViewPreserveAspectRation(bool preserveAspectRation)
{
  mpDiagramViewPreserveAspectRatioCheckBox->setChecked(preserveAspectRation);
}

bool GraphicalViewsPage::getDiagramViewPreserveAspectRation()
{
  return mpDiagramViewPreserveAspectRatioCheckBox->isChecked();
}

//! @class SimulationPage
//! @brief Creates an interface for simulation settings.

//! Constructor
//! @param pParent is the pointer to OptionsDialog
SimulationPage::SimulationPage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpOptionsDialog = pParent;
  // Matching Algorithm
  mpSimulationGroupBox = new QGroupBox(Helper::simulation);
  mpMatchingAlgorithmLabel = new Label(tr("Matching Algorithm:"));
  QStringList matchingAlgorithmChoices, matchingAlgorithmComments;
  mpOptionsDialog->getMainWindow()->getOMCProxy()->getAvailableMatchingAlgorithms(&matchingAlgorithmChoices, &matchingAlgorithmComments);
  mpMatchingAlgorithmComboBox = new QComboBox;
  int i = 0;
  foreach (QString matchingAlgorithmChoice, matchingAlgorithmChoices)
  {
    mpMatchingAlgorithmComboBox->addItem(matchingAlgorithmChoice);
    mpMatchingAlgorithmComboBox->setItemData(i, matchingAlgorithmComments[i], Qt::ToolTipRole);
    i++;
  }
  mpMatchingAlgorithmComboBox->setCurrentIndex(mpMatchingAlgorithmComboBox->findText(mpOptionsDialog->getMainWindow()->getOMCProxy()->getMatchingAlgorithm()));
  // Index Reduction Method
  mpIndexReductionMethodLabel = new Label(tr("Index Reduction Method:"));
  QStringList indexReductionMethodChoices, indexReductionMethodComments;
  mpOptionsDialog->getMainWindow()->getOMCProxy()->getAvailableIndexReductionMethods(&indexReductionMethodChoices, &indexReductionMethodComments);
  mpIndexReductionMethodComboBox = new QComboBox;
  i = 0;
  foreach (QString indexReductionChoice, indexReductionMethodChoices)
  {
    mpIndexReductionMethodComboBox->addItem(indexReductionChoice);
    mpIndexReductionMethodComboBox->setItemData(i, indexReductionMethodComments[i], Qt::ToolTipRole);
    i++;
  }
  mpIndexReductionMethodComboBox->setCurrentIndex(mpIndexReductionMethodComboBox->findText(mpOptionsDialog->getMainWindow()->getOMCProxy()->getIndexReductionMethod()));
  // OMC Flags
  mpOMCFlagsLabel = new Label(tr("OMC Flags"));
  mpOMCFlagsLabel->setToolTip(tr("Space separated list of flags e.g. +d=initialization +cheapmatchingAlgorithm=3"));
  mpOMCFlagsTextBox = new QLineEdit("+d=initialization");
  /* save class before simulation checkbox */
  mpSaveClassBeforeSimulationCheckBox = new QCheckBox(tr("Save class before simulation"));
  mpSaveClassBeforeSimulationCheckBox->setToolTip(tr("Disabling this will effect the debugger functionality."));
  mpSaveClassBeforeSimulationCheckBox->setChecked(true);
  // set the layout of simulation group
  QGridLayout *pSimulationLayout = new QGridLayout;
  pSimulationLayout->setAlignment(Qt::AlignTop);
  pSimulationLayout->addWidget(mpMatchingAlgorithmLabel, 0, 0);
  pSimulationLayout->addWidget(mpMatchingAlgorithmComboBox, 0, 1);
  pSimulationLayout->addWidget(mpIndexReductionMethodLabel, 1, 0);
  pSimulationLayout->addWidget(mpIndexReductionMethodComboBox, 1, 1);
  pSimulationLayout->addWidget(mpOMCFlagsLabel, 2, 0);
  pSimulationLayout->addWidget(mpOMCFlagsTextBox, 2, 1);
  pSimulationLayout->addWidget(mpSaveClassBeforeSimulationCheckBox, 3, 0, 1, 2);
  mpSimulationGroupBox->setLayout(pSimulationLayout);
  // set the layout
  QVBoxLayout *pLayout = new QVBoxLayout;
  pLayout->setAlignment(Qt::AlignTop);
  pLayout->setContentsMargins(0, 0, 0, 0);
  pLayout->addWidget(mpSimulationGroupBox);
  setLayout(pLayout);
}

QComboBox* SimulationPage::getMatchingAlgorithmComboBox()
{
  return mpMatchingAlgorithmComboBox;
}

QComboBox* SimulationPage::getIndexReductionMethodComboBox()
{
  return mpIndexReductionMethodComboBox;
}

QLineEdit* SimulationPage::getOMCFlagsTextBox()
{
  return mpOMCFlagsTextBox;
}

//! @class NotificationsPage
//! @brief Creates an interface for Notifications settings.

//! Constructor
//! @param pParent is the pointer to OptionsDialog
NotificationsPage::NotificationsPage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpOptionsDialog = pParent;
  // create the notifications groupbox
  mpNotificationsGroupBox = new QGroupBox(tr("Notifications"));
  // create the exit application checkbox
  mpQuitApplicationCheckBox = new QCheckBox(tr("Always quit without prompt"));
  // create the item drop on itself checkbox
  mpItemDroppedOnItselfCheckBox = new QCheckBox(tr("Show item dropped on itself message"));
  mpItemDroppedOnItselfCheckBox->setChecked(true);
  // create the replaceable if partial checkbox
  mpReplaceableIfPartialCheckBox = new QCheckBox(tr("Show model is partial and component is added as replaceable message"));
  mpReplaceableIfPartialCheckBox->setChecked(true);
  // create the inner model name changed checkbox
  mpInnerModelNameChangedCheckBox = new QCheckBox(tr("Show component is declared as inner message"));
  mpInnerModelNameChangedCheckBox->setChecked(true);
  // create the save model for bitmap insertion checkbox
  mpSaveModelForBitmapInsertionCheckBox = new QCheckBox(tr("Show save model for bitmap insertion message"));
  mpSaveModelForBitmapInsertionCheckBox->setChecked(true);
  // set the layout of notifications group
  QGridLayout *pNotificationsLayout = new QGridLayout;
  pNotificationsLayout->setAlignment(Qt::AlignTop);
  pNotificationsLayout->addWidget(mpQuitApplicationCheckBox, 0, 0);
  pNotificationsLayout->addWidget(mpItemDroppedOnItselfCheckBox, 1, 0);
  pNotificationsLayout->addWidget(mpReplaceableIfPartialCheckBox, 2, 0);
  pNotificationsLayout->addWidget(mpInnerModelNameChangedCheckBox, 3, 0);
  pNotificationsLayout->addWidget(mpSaveModelForBitmapInsertionCheckBox, 4, 0);
  mpNotificationsGroupBox->setLayout(pNotificationsLayout);
  // set the layout
  QVBoxLayout *pLayout = new QVBoxLayout;
  pLayout->setAlignment(Qt::AlignTop);
  pLayout->setContentsMargins(0, 0, 0, 0);
  pLayout->addWidget(mpNotificationsGroupBox);
  setLayout(pLayout);
}

QCheckBox* NotificationsPage::getQuitApplicationCheckBox()
{
  return mpQuitApplicationCheckBox;
}

QCheckBox* NotificationsPage::getItemDroppedOnItselfCheckBox()
{
  return mpItemDroppedOnItselfCheckBox;
}

QCheckBox* NotificationsPage::getReplaceableIfPartialCheckBox()
{
  return mpReplaceableIfPartialCheckBox;
}

QCheckBox* NotificationsPage::getInnerModelNameChangedCheckBox()
{
  return mpInnerModelNameChangedCheckBox;
}

QCheckBox* NotificationsPage::getSaveModelForBitmapInsertionCheckBox()
{
  return mpSaveModelForBitmapInsertionCheckBox;
}

//! @class LineStylePage
//! @brief Creates an interface for line style settings.

//! Constructor
//! @param pParent is the pointer to OptionsDialog
LineStylePage::LineStylePage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpOptionsDialog = pParent;
  mpLineStyleGroupBox = new QGroupBox(Helper::lineStyle);
  // Line Color
  mpLineColorLabel = new Label(Helper::color);
  mpLinePickColorButton = new QPushButton(Helper::pickColor);
  mpLinePickColorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  mpLinePickColorButton->setAutoDefault(false);
  connect(mpLinePickColorButton, SIGNAL(clicked()), SLOT(linePickColor()));
  setLineColor(Qt::black);
  setLinePickColorButtonIcon();
  // Line Pattern
  mpLinePatternLabel = new Label(Helper::pattern);
  mpLinePatternComboBox = StringHandler::getLinePatternComboBox();
  setLinePattern(StringHandler::getLinePatternString(StringHandler::LineSolid));
  // Line Thickness
  mpLineThicknessLabel = new Label(Helper::thickness);
  mpLineThicknessSpinBox = new DoubleSpinBox;
  mpLineThicknessSpinBox->setRange(0, std::numeric_limits<double>::max());
  mpLineThicknessSpinBox->setValue(0.25);
  mpLineThicknessSpinBox->setSingleStep(0.25);
  // Line Arrow
  mpLineStartArrowLabel = new Label(Helper::startArrow);
  mpLineStartArrowComboBox = StringHandler::getStartArrowComboBox();
  mpLineEndArrowLabel = new Label(Helper::endArrow);
  mpLineEndArrowComboBox = StringHandler::getEndArrowComboBox();
  mpLineArrowSizeLabel = new Label(Helper::arrowSize);
  mpLineArrowSizeSpinBox = new DoubleSpinBox;
  mpLineArrowSizeSpinBox->setRange(0, std::numeric_limits<double>::max());
  mpLineArrowSizeSpinBox->setValue(3);
  mpLineArrowSizeSpinBox->setSingleStep(1);
  // Line smooth
  mpLineSmoothLabel = new Label(Helper::smooth);
  mpLineSmoothCheckBox = new QCheckBox(Helper::bezier);
  // set the layout
  QGridLayout *pLineStyleLayout = new QGridLayout;
  pLineStyleLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pLineStyleLayout->addWidget(mpLineColorLabel, 0, 0);
  pLineStyleLayout->addWidget(mpLinePickColorButton, 0, 1);
  pLineStyleLayout->addWidget(mpLinePatternLabel, 2, 0);
  pLineStyleLayout->addWidget(mpLinePatternComboBox, 2, 1);
  pLineStyleLayout->addWidget(mpLineThicknessLabel, 3, 0);
  pLineStyleLayout->addWidget(mpLineThicknessSpinBox, 3, 1);
  pLineStyleLayout->addWidget(mpLineStartArrowLabel, 4, 0);
  pLineStyleLayout->addWidget(mpLineStartArrowComboBox, 4, 1);
  pLineStyleLayout->addWidget(mpLineEndArrowLabel, 5, 0);
  pLineStyleLayout->addWidget(mpLineEndArrowComboBox, 5, 1);
  pLineStyleLayout->addWidget(mpLineArrowSizeLabel, 6, 0);
  pLineStyleLayout->addWidget(mpLineArrowSizeSpinBox, 6, 1);
  pLineStyleLayout->addWidget(mpLineSmoothLabel, 7, 0);
  pLineStyleLayout->addWidget(mpLineSmoothCheckBox, 7, 1);
  mpLineStyleGroupBox->setLayout(pLineStyleLayout);
  QVBoxLayout *pMainLayout = new QVBoxLayout;
  pMainLayout->setAlignment(Qt::AlignTop);
  pMainLayout->setContentsMargins(0, 0, 0, 0);
  pMainLayout->addWidget(mpLineStyleGroupBox);
  setLayout(pMainLayout);
}

//! Sets the pen color
//! @param color to set.
void LineStylePage::setLineColor(QColor color)
{
  mLineColor = color;
}

//! Returns the pen color
QColor LineStylePage::getLineColor()
{
  return mLineColor;
}

void LineStylePage::setLinePickColorButtonIcon()
{
  QPixmap pixmap(Helper::iconSize);
  pixmap.fill(getLineColor());
  mpLinePickColorButton->setIcon(pixmap);
}

//! Sets the pen pattern
//! @param pattern to set.
void LineStylePage::setLinePattern(QString pattern)
{
  int index = mpLinePatternComboBox->findText(pattern, Qt::MatchExactly);
  if (index != -1)
    mpLinePatternComboBox->setCurrentIndex(index);
}

QString LineStylePage::getLinePattern()
{
  return mpLinePatternComboBox->currentText();
}

//! Sets the pen thickness
//! @param thickness to set.
void LineStylePage::setLineThickness(qreal thickness)
{
  if (thickness <= 0)
    thickness = 0.25;
  mpLineThicknessSpinBox->setValue(thickness);
}

//! Returns the pen thickness
qreal LineStylePage::getLineThickness()
{
  return mpLineThicknessSpinBox->value();
}

void LineStylePage::setLineStartArrow(QString startArrow)
{
  int index = mpLineStartArrowComboBox->findText(startArrow, Qt::MatchExactly);
  if (index != -1)
    mpLineStartArrowComboBox->setCurrentIndex(index);
}

QString LineStylePage::getLineStartArrow()
{
  return mpLineStartArrowComboBox->currentText();
}

void LineStylePage::setLineEndArrow(QString endArrow)
{
  int index = mpLineEndArrowComboBox->findText(endArrow, Qt::MatchExactly);
  if (index != -1)
    mpLineEndArrowComboBox->setCurrentIndex(index);
}

QString LineStylePage::getLineEndArrow()
{
  return mpLineEndArrowComboBox->currentText();
}

void LineStylePage::setLineArrowSize(qreal size)
{
  if (size <= 0)
    size = 3;
  mpLineArrowSizeSpinBox->setValue(size);
}

qreal LineStylePage::getLineArrowSize()
{
  return mpLineArrowSizeSpinBox->value();
}

//! Sets whether the pen used will be smooth (for splines) or not.
//! @param smooth
void LineStylePage::setLineSmooth(bool smooth)
{
  mpLineSmoothCheckBox->setChecked(smooth);
}

//! Returns the pen smooth
bool LineStylePage::getLineSmooth()
{
  return mpLineSmoothCheckBox->isChecked();
}

//! Opens the color picker dialog. The user selects the color and the color saved as a pen color.
void LineStylePage::linePickColor()
{
  QColor color = QColorDialog::getColor(getLineColor());
  // if user press ESC
  if (!color.isValid())
    return;
  setLineColor(color);
  setLinePickColorButtonIcon();
}

//! @class FillStylePage
//! @brief Creates an interface for fill style settings.

//! Constructor
//! @param pParent is the pointer to OptionsDialog
FillStylePage::FillStylePage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpFillStyleGroupBox = new QGroupBox(Helper::fillStyle);
  // Fill Color
  mpFillColorLabel = new Label(Helper::color);
  mpFillPickColorButton = new QPushButton(Helper::pickColor);
  mpFillPickColorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  mpFillPickColorButton->setAutoDefault(false);
  connect(mpFillPickColorButton, SIGNAL(clicked()), SLOT(fillPickColor()));
  setFillColor(Qt::black);
  setFillPickColorButtonIcon();
  // Fill Pattern
  mpFillPatternLabel = new Label(Helper::pattern);
  mpFillPatternComboBox = StringHandler::getFillPatternComboBox();
  setFillPattern(StringHandler::getFillPatternString(StringHandler::FillNone));
  // set the layout
  QGridLayout *pFillStyleLayout = new QGridLayout;
  pFillStyleLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pFillStyleLayout->addWidget(mpFillColorLabel, 0, 0);
  pFillStyleLayout->addWidget(mpFillPickColorButton, 0, 1);
  pFillStyleLayout->addWidget(mpFillPatternLabel, 2, 0);
  pFillStyleLayout->addWidget(mpFillPatternComboBox, 2, 1);
  mpFillStyleGroupBox->setLayout(pFillStyleLayout);
  QVBoxLayout *pMainLayout = new QVBoxLayout;
  pMainLayout->setAlignment(Qt::AlignTop);
  pMainLayout->setContentsMargins(0, 0, 0, 0);
  pMainLayout->addWidget(mpFillStyleGroupBox);
  setLayout(pMainLayout);
}

void FillStylePage::setFillColor(QColor color)
{
  mFillColor = color;
}

QColor FillStylePage::getFillColor()
{
  return mFillColor;
}

void FillStylePage::setFillPickColorButtonIcon()
{
  QPixmap pixmap(Helper::iconSize);
  pixmap.fill(getFillColor());
  mpFillPickColorButton->setIcon(pixmap);
}

void FillStylePage::setFillPattern(QString pattern)
{
  int index = mpFillPatternComboBox->findText(pattern, Qt::MatchExactly);
  if (index != -1)
    mpFillPatternComboBox->setCurrentIndex(index);
}

QString FillStylePage::getFillPattern()
{
  return mpFillPatternComboBox->currentText();
}

void FillStylePage::fillPickColor()
{
  QColor color = QColorDialog::getColor(getFillColor());
  // if user press ESC
  if (!color.isValid())
    return;
  setFillColor(color);
  setFillPickColorButtonIcon();
}

//! @class CurveStylePage
//! @brief Creates an interface for curve style settings.

//! Constructor
//! @param pParent is the pointer to OptionsDialog
CurveStylePage::CurveStylePage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpOptionsDialog = pParent;
  mpCurveStyleGroupBox = new QGroupBox(Helper::curveStyle);
  // Curve Pattern
  mpCurvePatternLabel = new Label(Helper::pattern);
  mpCurvePatternComboBox = new QComboBox;
  mpCurvePatternComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  mpCurvePatternComboBox->addItem("SolidLine", 1);
  mpCurvePatternComboBox->addItem("DashLine", 2);
  mpCurvePatternComboBox->addItem("DotLine", 3);
  mpCurvePatternComboBox->addItem("DashDotLine", 4);
  mpCurvePatternComboBox->addItem("DashDotDotLine", 5);
  mpCurvePatternComboBox->addItem("Sticks", 6);
  mpCurvePatternComboBox->addItem("Steps", 7);
  // Curve Thickness
  mpCurveThicknessLabel = new Label(Helper::thickness);
  mpCurveThicknessSpinBox = new DoubleSpinBox;
  mpCurveThicknessSpinBox->setRange(0, std::numeric_limits<double>::max());
  mpCurveThicknessSpinBox->setValue(1);
  mpCurveThicknessSpinBox->setSingleStep(1);
  // set the layout
  QGridLayout *pCurveStyleLayout = new QGridLayout;
  pCurveStyleLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pCurveStyleLayout->addWidget(mpCurvePatternLabel, 0, 0);
  pCurveStyleLayout->addWidget(mpCurvePatternComboBox, 0, 1);
  pCurveStyleLayout->addWidget(mpCurveThicknessLabel, 1, 0);
  pCurveStyleLayout->addWidget(mpCurveThicknessSpinBox, 1, 1);
  mpCurveStyleGroupBox->setLayout(pCurveStyleLayout);
  QVBoxLayout *pMainLayout = new QVBoxLayout;
  pMainLayout->setAlignment(Qt::AlignTop);
  pMainLayout->setContentsMargins(0, 0, 0, 0);
  pMainLayout->addWidget(mpCurveStyleGroupBox);
  setLayout(pMainLayout);
}

//! Sets the pen pattern
//! @param pattern to set.
void CurveStylePage::setCurvePattern(int pattern)
{
  int index = mpCurvePatternComboBox->findData(pattern);
  if (index != -1)
    mpCurvePatternComboBox->setCurrentIndex(index);
}

int CurveStylePage::getCurvePattern()
{
  return mpCurvePatternComboBox->itemData(mpCurvePatternComboBox->currentIndex()).toInt();
}

//! Sets the pen thickness
//! @param thickness to set.
void CurveStylePage::setCurveThickness(qreal thickness)
{
  if (thickness <= 0)
    thickness = 1.0;
  mpCurveThicknessSpinBox->setValue(thickness);
}

//! Returns the pen thickness
qreal CurveStylePage::getCurveThickness()
{
  return mpCurveThicknessSpinBox->value();
}

//! @class FigaroPage
//! @brief Creates an interface for Figaro settings.

//! Constructor
//! @param pParent is the pointer to OptionsDialog
FigaroPage::FigaroPage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpOptionsDialog = pParent;
  mpFigaroGroupBox = new QGroupBox(Helper::figaro);
  // Figaro database file
  mpFigaroDatabaseFileLabel = new Label(tr("Figaro Database File:"));
  mpFigaroDatabaseFileTextBox = new QLineEdit;
  mpBrowseFigaroDatabaseFileButton = new QPushButton(Helper::browse);
  connect(mpBrowseFigaroDatabaseFileButton, SIGNAL(clicked()), SLOT(browseFigaroLibraryFile()));
  // Figaro model
  mpFigaroModeLabel = new Label(tr("Figaro Mode:"));
  mpFigaroModeComboBox = new QComboBox;
  mpFigaroModeComboBox->addItem(tr("figaro0"), "figaro0");
  mpFigaroModeComboBox->addItem(tr("fault-tree"), "fault-tree");
  // Figaro options file
  mpFigaroOptionsFileLabel = new Label(tr("Figaro Options File:"));
  mpFigaroOptionsFileTextBox = new QLineEdit;
  mpBrowseFigaroOptionsFileButton = new QPushButton(Helper::browse);
  connect(mpBrowseFigaroOptionsFileButton, SIGNAL(clicked()), SLOT(browseFigaroOptionsFile()));
  // figaro process
  mpFigaroProcessLabel = new Label(tr("Figaro Process:"));
  mpFigaroProcessTextBox = new QLineEdit;
  mpBrowseFigaroProcessButton = new QPushButton(Helper::browse);
  connect(mpBrowseFigaroProcessButton, SIGNAL(clicked()), SLOT(browseFigaroProcessFile()));
  // set the layout
  QGridLayout *pFigaroLayout = new QGridLayout;
  pFigaroLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pFigaroLayout->addWidget(mpFigaroDatabaseFileLabel, 0, 0);
  pFigaroLayout->addWidget(mpFigaroDatabaseFileTextBox, 0, 1);
  pFigaroLayout->addWidget(mpBrowseFigaroDatabaseFileButton, 0, 2);
  pFigaroLayout->addWidget(mpFigaroModeLabel, 1, 0);
  pFigaroLayout->addWidget(mpFigaroModeComboBox, 1, 1, 1, 2);
  pFigaroLayout->addWidget(mpFigaroOptionsFileLabel, 2, 0);
  pFigaroLayout->addWidget(mpFigaroOptionsFileTextBox, 2, 1);
  pFigaroLayout->addWidget(mpBrowseFigaroOptionsFileButton, 2, 2);
  pFigaroLayout->addWidget(mpFigaroProcessLabel, 3, 0);
  pFigaroLayout->addWidget(mpFigaroProcessTextBox, 3, 1);
  pFigaroLayout->addWidget(mpBrowseFigaroProcessButton, 3, 2);
  mpFigaroGroupBox->setLayout(pFigaroLayout);
  QVBoxLayout *pMainLayout = new QVBoxLayout;
  pMainLayout->setAlignment(Qt::AlignTop);
  pMainLayout->setContentsMargins(0, 0, 0, 0);
  pMainLayout->addWidget(mpFigaroGroupBox);
  setLayout(pMainLayout);
}

void FigaroPage::browseFigaroLibraryFile()
{
  mpFigaroDatabaseFileTextBox->setText(StringHandler::getOpenFileName(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseFile),
                                                                      NULL, Helper::xmlFileTypes, NULL));
}

void FigaroPage::browseFigaroOptionsFile()
{
  mpFigaroOptionsFileTextBox->setText(StringHandler::getOpenFileName(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseFile),
                                                                     NULL, Helper::xmlFileTypes, NULL));
}

void FigaroPage::browseFigaroProcessFile()
{
  mpFigaroProcessTextBox->setText(StringHandler::getOpenFileName(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseFile),
                                                                 NULL, Helper::exeFileTypes, NULL));
}

/*!
  \class DebuggerPage
  \brief Creates an interface for debugger settings.
  */
/*!
  \param pParent - pointer to OptionsDialog
  */
DebuggerPage::DebuggerPage(OptionsDialog *pParent)
  : QWidget(pParent)
{
  mpOptionsDialog = pParent;
  mpAlgorithmicDebuggerGroupBox = new QGroupBox(Helper::algorithmicDebugger);
  // GDB Path
  mpGDBPathLabel = new Label(tr("GDB Path:"));
#ifdef WIN32
  const char *OMDEV = getenv("OMDEV");
  if (QString(OMDEV).isEmpty()) {
    mpGDBPathTextBox = new QLineEdit(QString(Helper::OpenModelicaHome).append("/MinGW/bin/gdb.exe"));
  } else {
    QString qOMDEV = QString(OMDEV).replace("\\", "/");
    mpGDBPathTextBox = new QLineEdit(QString(qOMDEV).append("/tools/mingw/bin/gdb.exe"));
  }
#else
  mpGDBPathTextBox = new QLineEdit("gdb");
#endif
  mpGDBPathBrowseButton = new QPushButton(Helper::browse);
  connect(mpGDBPathBrowseButton, SIGNAL(clicked()), SLOT(browseGDBPath()));
  /* GDB Commanf Timeout */
  mpGDBCommandTimeoutLabel = new Label(tr("GDB Command Timeout:"));
  mpGDBCommandTimeoutSpinBox = new QSpinBox;
  mpGDBCommandTimeoutSpinBox->setSuffix(tr(" seconds"));
  mpGDBCommandTimeoutSpinBox->setRange(30, std::numeric_limits<int>::max());
  mpGDBCommandTimeoutSpinBox->setSingleStep(10);
  mpGDBCommandTimeoutSpinBox->setValue(40);
  // Display C Frames
  mpDisplayCFramesCheckBox = new QCheckBox(tr("Display C frames"));
  mpDisplayCFramesCheckBox->setChecked(true);
  // Display Unknown Frames
  mpDisplayUnknownFramesCheckBox = new QCheckBox(tr("Display unknown frames"));
  mpDisplayUnknownFramesCheckBox->setChecked(true);
  // clear output on new run
  mpClearOutputOnNewRunCheckBox = new QCheckBox(tr("Clear old output on a new run"));
  mpClearOutputOnNewRunCheckBox->setChecked(true);
  // clear log on new run
  mpClearLogOnNewRunCheckBox = new QCheckBox(tr("Clear old log on a new run"));
  mpClearLogOnNewRunCheckBox->setChecked(true);
  /* set the debugger group box layout */
  QGridLayout *pDebuggerLayout = new QGridLayout;
  pDebuggerLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pDebuggerLayout->addWidget(mpGDBPathLabel, 0, 0);
  pDebuggerLayout->addWidget(mpGDBPathTextBox, 0, 1);
  pDebuggerLayout->addWidget(mpGDBPathBrowseButton, 0, 2);
  pDebuggerLayout->addWidget(mpGDBCommandTimeoutLabel, 1, 0);
  pDebuggerLayout->addWidget(mpGDBCommandTimeoutSpinBox, 1, 1, 1, 2);
  pDebuggerLayout->addWidget(mpDisplayCFramesCheckBox, 2, 0, 1, 2);
  pDebuggerLayout->addWidget(mpDisplayUnknownFramesCheckBox, 3, 0, 1, 2);
  pDebuggerLayout->addWidget(mpClearOutputOnNewRunCheckBox, 4, 0, 1, 2);
  pDebuggerLayout->addWidget(mpClearLogOnNewRunCheckBox, 5, 0, 1, 2);
  mpAlgorithmicDebuggerGroupBox->setLayout(pDebuggerLayout);
  /* Transformational Debugger */
  mpTransformationalDebuggerGroupBox = new QGroupBox(Helper::transformationalDebugger);
  mpAlwaysShowTransformationsCheckBox = new QCheckBox(tr("Always show %1 after compilation").arg(Helper::transformationalDebugger));
  mpGenerateOperationsCheckBox = new QCheckBox(tr("Generate operations in the info xml"));
  // set the layout of Transformational Debugger group
  QGridLayout *pTransformationalDebuggerLayout = new QGridLayout;
  pTransformationalDebuggerLayout->setAlignment(Qt::AlignTop);
  pTransformationalDebuggerLayout->addWidget(mpAlwaysShowTransformationsCheckBox, 0, 0);
  pTransformationalDebuggerLayout->addWidget(mpGenerateOperationsCheckBox, 1, 0);
  mpTransformationalDebuggerGroupBox->setLayout(pTransformationalDebuggerLayout);
  // set the layout
  QVBoxLayout *pMainLayout = new QVBoxLayout;
  pMainLayout->setAlignment(Qt::AlignTop);
  pMainLayout->setContentsMargins(0, 0, 0, 0);
  pMainLayout->addWidget(mpAlgorithmicDebuggerGroupBox);
  pMainLayout->addWidget(mpTransformationalDebuggerGroupBox);
  setLayout(pMainLayout);
}

void DebuggerPage::setGDBPath(QString path)
{
  mpGDBPathTextBox->setText(path.isEmpty() ? "gdb" : path);
}

QString DebuggerPage::getGDBPath()
{
  if (mpGDBPathTextBox->text().isEmpty())
    return "gdb";
  else
    return mpGDBPathTextBox->text();
}

void DebuggerPage::browseGDBPath()
{
  QString GDBPath = StringHandler::getOpenFileName(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseFile),
                                                   NULL, "", NULL);
  if (GDBPath.isEmpty())
    return;
  mpGDBPathTextBox->setText(GDBPath);
}
