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

#include <QtSvg/QSvgGenerator>

#include "MainWindow.h"
#include "VariablesWidget.h"
#include "Helper.h"
#include "SimulationOutputWidget.h"

MainWindow::MainWindow(QSplashScreen *pSplashScreen, QWidget *parent)
  : QMainWindow(parent), mExitApplicationStatus(false), mDebugApplication(false)
{
  // This is a very convoluted way of asking for the default system font in Qt
  QFont systmFont("Monospace");
  systmFont.setStyleHint(QFont::System);
  Helper::systemFontInfo = QFontInfo(systmFont);
  // This is a very convoluted way of asking for the default monospace font in Qt
  QFont monospaceFont("Monospace");
  monospaceFont.setStyleHint(QFont::TypeWriter);
  Helper::monospacedFontInfo = QFontInfo(monospaceFont);
  /*! @note Register the RecentFile, FindText and DebuggerConfiguration struct in the Qt's meta system
   * Don't remove/move the following lines.
   * Because RecentFile, FindText and DebuggerConfiguration structs should be registered before reading the recentFilesList, FindText and
     DebuggerConfiguration section respectively from the settings file.
   */
  qRegisterMetaTypeStreamOperators<RecentFile>("RecentFile");
  qRegisterMetaTypeStreamOperators<FindText>("FindText");
  qRegisterMetaTypeStreamOperators<DebuggerConfiguration>("DebuggerConfiguration");
  /*! @note The above three lines registers the structs as QMetaObjects. Do not remove/move them. */
  setObjectName("MainWindow");
  setWindowTitle(Helper::applicationName + " - "  + Helper::applicationIntroText);
  setWindowIcon(QIcon(":/Resources/icons/modeling.png"));
  setMinimumSize(400, 300);
  resize(800, 600);
  setContentsMargins(1, 1, 1, 1);
  // Create an object of MessagesWidget.
  mpMessagesWidget = new MessagesWidget(this);
  // Create the OMCProxy object.
  pSplashScreen->showMessage(tr("Connecting to OpenModelica Compiler"), Qt::AlignRight, Qt::white);
  mpOMCProxy = new OMCProxy(this);
  if (getExitApplicationStatus())
    return;
  pSplashScreen->showMessage(tr("Reading Settings"), Qt::AlignRight, Qt::white);
  mpOptionsDialog = new OptionsDialog(this);
  //Set the name and size of the main window
  pSplashScreen->showMessage(tr("Loading Widgets"), Qt::AlignRight, Qt::white);
  // Create MessagesDockWidget dock
  mpMessagesDockWidget = new QDockWidget(tr("Messages Browser"), this);
  mpMessagesDockWidget->setObjectName("Messages");
  mpMessagesDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
  mpMessagesDockWidget->setWidget(mpMessagesWidget);
  addDockWidget(Qt::BottomDockWidgetArea, mpMessagesDockWidget);
  mpMessagesDockWidget->hide();
  connect(mpMessagesWidget, SIGNAL(MessageAdded()), mpMessagesDockWidget, SLOT(show()));
  // Create an object of SearchClassWidget
  mpSearchClassWidget = new SearchClassWidget(this);
  // Create LibraryTreeWidget dock
  mpSearchClassDockWidget = new QDockWidget(tr("Search Browser"), this);
  mpSearchClassDockWidget->setObjectName("SearchClassWidget");
  mpSearchClassDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  mpSearchClassDockWidget->setWidget(mpSearchClassWidget);
  addDockWidget(Qt::LeftDockWidgetArea, mpSearchClassDockWidget);
  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  connect(mpSearchClassDockWidget, SIGNAL(visibilityChanged(bool)), SLOT(focusSearchClassWidget(bool)));
  mpSearchClassDockWidget->hide();
  // Create an object of LibraryTreeWidget
  mpLibraryTreeWidget = new LibraryTreeWidget(false, this);
  // Loads and adds the OM Standard Library into the Library Widget.
  mpLibraryTreeWidget->addModelicaLibraries(pSplashScreen);
  // Create LibraryTreeWidget dock
  mpLibraryTreeDockWidget = new QDockWidget(tr("Libraries Browser"), this);
  mpLibraryTreeDockWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  mpLibraryTreeDockWidget->setObjectName("Libraries");
  mpLibraryTreeDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  mpLibraryTreeDockWidget->setWidget(mpLibraryTreeWidget);
  addDockWidget(Qt::LeftDockWidgetArea, mpLibraryTreeDockWidget);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  // create an object of DocumentationWidget
  mpDocumentationWidget = new DocumentationWidget(this);
  // Create DocumentationWidget dock
  mpDocumentationDockWidget = new QDockWidget(tr("Documentation Browser"), this);
  mpDocumentationDockWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  mpDocumentationDockWidget->setObjectName("Documentation");
  mpDocumentationDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  mpDocumentationDockWidget->setWidget(mpDocumentationWidget);
  addDockWidget(Qt::RightDockWidgetArea, mpDocumentationDockWidget);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  mpDocumentationDockWidget->hide();
  // Create VariablesWidget dock
  mpVariablesDockWidget = new QDockWidget(Helper::variablesBrowser, this);
  mpVariablesDockWidget->setObjectName("Variables");
  mpVariablesDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  addDockWidget(Qt::RightDockWidgetArea, mpVariablesDockWidget);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  mpVariablesDockWidget->hide();
  //Create Actions, Toolbar and Menus
  pSplashScreen->showMessage(tr("Creating Widgets"), Qt::AlignRight, Qt::white);
  setAcceptDrops(true);
  createActions();
  createToolbars();
  createMenus();
  // Create find/replace dialog
  mpFindReplaceDialog = new FindReplaceDialog(this);
  // Create simulation dialog
  mpSimulationDialog = new SimulationDialog(this);
  // Create an object of PlotWindowContainer
  mpPlotWindowContainer = new PlotWindowContainer(this);
  // create an object of VariablesWidget
  mpVariablesWidget = new VariablesWidget(this);
  mpVariablesDockWidget->setWidget(mpVariablesWidget);
  // Create an object of InteractiveSimulationTabWidget
  //mpInteractiveSimualtionTabWidget = new InteractiveSimulationTabWidget(this);
  /* Debugger MainWindow */
  /* Important. Create the instance of DebuggerMainWindow before ModelWidgetContainer otherwise the ctrl+tab changer does not work. */
  mpDebuggerMainWindow = new DebuggerMainWindow(this);
  // Create an object of ModelWidgetContainer
  mpModelWidgetContainer = new ModelWidgetContainer(this);
  // Create an object of WelcomePageWidget
  mpWelcomePageWidget = new WelcomePageWidget(this);
  updateRecentFileActions();
  // create the OMEdit About widget
  mpAboutOMEditDialog = new AboutOMEditWidget(this);
  mpAboutOMEditDialog->hide();
  // create an instance of InfoBar
  mpInfoBar = new InfoBar(this);
  mpInfoBar->hide();
  //Create a centralwidget for the main window
  QWidget *pCentralwidget = new QWidget;
  mpCentralStackedWidget = new QStackedWidget;
  mpCentralStackedWidget->addWidget(mpWelcomePageWidget);
  mpCentralStackedWidget->addWidget(mpModelWidgetContainer);
  mpCentralStackedWidget->addWidget(mpPlotWindowContainer);
  // set the layout
  QGridLayout *pCentralgrid = new QGridLayout;
  pCentralgrid->setVerticalSpacing(4);
  pCentralgrid->setContentsMargins(0, 1, 0, 0);
  pCentralgrid->addWidget(mpInfoBar, 0, 0);
  pCentralgrid->addWidget(mpCentralStackedWidget, 1, 0);
  pCentralwidget->setLayout(pCentralgrid);
  //Set the centralwidget
  setCentralWidget(pCentralwidget);
  // Create an object of QStatusBar
  mpStatusBar = new QStatusBar();
  mpStatusBar->setObjectName("statusBar");
  mpStatusBar->setContentsMargins(0, 0, 0, 0);
  // Create an object of QProgressBar
  mpProgressBar = new QProgressBar;
  mpProgressBar->setMaximumWidth(300);
  mpProgressBar->setTextVisible(false);
  mpProgressBar->setVisible(false);
  // pointer position Label
  mpPointerXPositionLabel = new Label;
  mpPointerXPositionLabel->setMinimumWidth(60);
  mpPointerYPositionLabel = new Label;
  mpPointerYPositionLabel->setMinimumWidth(60);
  // add progressbar to statusbar
  mpStatusBar->addPermanentWidget(mpProgressBar);
  mpStatusBar->addPermanentWidget(mpPointerXPositionLabel);
  mpStatusBar->addPermanentWidget(mpPointerYPositionLabel);
  mpPerspectiveTabbar = new QTabBar;
  mpPerspectiveTabbar->setDocumentMode(true);
  mpPerspectiveTabbar->setShape(QTabBar::RoundedSouth);
  mpPerspectiveTabbar->addTab(QIcon(":/Resources/icons/omedit.png"), tr("Welcome"));
  mpPerspectiveTabbar->addTab(QIcon(":/Resources/icons/modeling.png"), tr("Modeling"));
  mpPerspectiveTabbar->addTab(QIcon(":/Resources/icons/omplot.png"), tr("Plotting"));
  connect(mpPerspectiveTabbar, SIGNAL(currentChanged(int)), SLOT(perspectiveTabChanged(int)));
  mpStatusBar->addPermanentWidget(mpPerspectiveTabbar);
  // set status bar for MainWindow
  setStatusBar(mpStatusBar);
  // set the matching algorithm.
  mpOMCProxy->setMatchingAlgorithm(mpOptionsDialog->getSimulationPage()->getMatchingAlgorithmComboBox()->currentText());
  // set the index reduction methods.
  mpOMCProxy->setIndexReductionMethod(mpOptionsDialog->getSimulationPage()->getIndexReductionMethodComboBox()->currentText());
  // set the OMC Flags.
  if (!mpOptionsDialog->getSimulationPage()->getOMCFlagsTextBox()->text().isEmpty())
    mpOMCProxy->setCommandLineOptions(mpOptionsDialog->getSimulationPage()->getOMCFlagsTextBox()->text());
  if (mpOptionsDialog->getDebuggerPage()->getGenerateOperationsCheckBox()->isChecked())
    mpOMCProxy->setCommandLineOptions("+d=infoXmlOperations");
  // restore OMEdit widgets state
  QSettings *pSettings = OpenModelica::getApplicationSettings();
  if (mpOptionsDialog->getGeneralSettingsPage()->getPreserveUserCustomizations())
  {
    restoreGeometry(pSettings->value("application/geometry").toByteArray());
    bool restoreMessagesWidget = !mpMessagesWidget->getMessagesTextBrowser()->toPlainText().isEmpty();
    restoreState(pSettings->value("application/windowState").toByteArray());
    if (restoreMessagesWidget) {
      mpMessagesDockWidget->show();
    }
  }
  switchToWelcomePerspective();
  // read last Open Directory location
  if (pSettings->contains("lastOpenDirectory")) {
    StringHandler::setLastOpenDirectory(pSettings->value("lastOpenDirectory").toString());
  }
  // read the grid lines
  if (pSettings->contains("modeling/gridLines")) {
    mpShowGridLinesAction->setChecked(pSettings->value("modeling/gridLines").toBool());
  }
  // create the auto save timer
  mpAutoSaveTimer = new QTimer(this);
  connect(mpAutoSaveTimer, SIGNAL(timeout()), SLOT(autoSave()));
  // read auto save settings
  if (mpOptionsDialog->getGeneralSettingsPage()->getEnableAutoSaveGroupBox()->isChecked())
  {
    mpAutoSaveTimer->start(mpOptionsDialog->getGeneralSettingsPage()->getAutoSaveIntervalSpinBox()->value() * 1000);
  }
}

//! Returns the instance of OMCProxy.
//! @return Instance of OMCProxy.
OMCProxy* MainWindow::getOMCProxy()
{
  return mpOMCProxy;
}

//! sets the value of ExitApplicationStatus variable.
void MainWindow::setExitApplicationStatus(bool status)
{
  mExitApplicationStatus = status;
}

//! Returns the value of ExitApplicationStatus variable.
bool MainWindow::getExitApplicationStatus()
{
  return mExitApplicationStatus;
}

void MainWindow::setDebugApplication(bool debug)
{
  mDebugApplication = debug;
}

bool MainWindow::getDebugApplication()
{
  return mDebugApplication;
}

OptionsDialog* MainWindow::getOptionsDialog()
{
  return mpOptionsDialog;
}

MessagesWidget* MainWindow::getMessagesWidget()
{
  return mpMessagesWidget;
}

LibraryTreeWidget* MainWindow::getLibraryTreeWidget()
{
  return mpLibraryTreeWidget;
}

DocumentationWidget* MainWindow::getDocumentationWidget()
{
  return mpDocumentationWidget;
}

QDockWidget* MainWindow::getDocumentationDockWidget()
{
  return mpDocumentationDockWidget;
}

VariablesWidget* MainWindow::getVariablesWidget()
{
  return mpVariablesWidget;
}

QDockWidget* MainWindow::getVariablesDockWidget()
{
  return mpVariablesDockWidget;
}

SimulationDialog* MainWindow::getSimulationDialog()
{
  return mpSimulationDialog;
}

PlotWindowContainer* MainWindow::getPlotWindowContainer()
{
  return mpPlotWindowContainer;
}

//InteractiveSimulationTabWidget* MainWindow::getInteractiveSimulationTabWidget()
//{
//  return mpInteractiveSimualtionTabWidget;
//}

ModelWidgetContainer* MainWindow::getModelWidgetContainer()
{
  return mpModelWidgetContainer;
}

WelcomePageWidget* MainWindow::getWelcomePageWidget()
{
  return mpWelcomePageWidget;
}

InfoBar* MainWindow::getInfoBar()
{
  return mpInfoBar;
}

QStatusBar* MainWindow::getStatusBar()
{
  return mpStatusBar;
}

QProgressBar* MainWindow::getProgressBar()
{
  return mpProgressBar;
}

Label* MainWindow::getPointerXPositionLabel()
{
  return mpPointerXPositionLabel;
}

Label* MainWindow::getPointerYPositionLabel()
{
  return mpPointerYPositionLabel;
}

QTabBar* MainWindow::getPerspectiveTabBar()
{
  return mpPerspectiveTabbar;
}

QAction* MainWindow::getSaveAction()
{
  return mpSaveAction;
}

QAction* MainWindow::getSaveAsAction()
{
  return mpSaveAsAction;
}

QAction* MainWindow::getPrintModelAction()
{
  return mpPrintModelAction;
}

QAction* MainWindow::getSaveAllAction()
{
  return mpSaveAllAction;
}

QAction* MainWindow::getShowGridLinesAction()
{
  return mpShowGridLinesAction;
}

QAction* MainWindow::getResetZoomAction()
{
  return mpResetZoomAction;
}

QAction* MainWindow::getZoomInAction()
{
  return mpZoomInAction;
}

QAction* MainWindow::getZoomOutAction()
{
  return mpZoomOutAction;
}

QAction* MainWindow::getSimulateModelAction()
{
  return mpSimulateModelAction;
}

QAction* MainWindow::getSimulationSetupAction()
{
  return mpSimulationSetupAction;
}

QAction* MainWindow::getInstantiateModelAction()
{
  return mpInstantiateModelAction;
}

QAction* MainWindow::getCheckModelAction()
{
  return mpCheckModelAction;
}

QAction* MainWindow::getExportFMUAction()
{
  return mpExportFMUAction;
}

QAction* MainWindow::getExportXMLAction()
{
  return mpExportXMLAction;
}

QAction* MainWindow::getExportFigaroAction()
{
  return mpExportFigaroAction;
}

QAction* MainWindow::getLineShapeAction()
{
  return mpLineShapeAction;
}

QAction* MainWindow::getPolygonShapeAction()
{
  return mpPolygonShapeAction;
}

QAction* MainWindow::getRectangleShapeAction()
{
  return mpRectangleShapeAction;
}

QAction* MainWindow::getEllipseShapeAction()
{
  return mpEllipseShapeAction;
}

QAction* MainWindow::getTextShapeAction()
{
  return mpTextShapeAction;
}

QAction* MainWindow::getBitmapShapeAction()
{
  return mpBitmapShapeAction;
}

QAction* MainWindow::getExportAsImageAction()
{
  return mpExportAsImageAction;
}

QAction* MainWindow::getExportToOMNotebookAction()
{
  return mpExportToOMNotebookAction;
}

QAction* MainWindow::getImportFromOMNotebookAction()
{
  return mpImportFromOMNotebookAction;
}

QAction* MainWindow::getImportNgspiceNetlistAction()
{
  return mpImportNgspiceNetlistAction;
}

QAction* MainWindow::getConnectModeAction()
{
  return mpConnectModeAction;
}

QAction* MainWindow::getFindReplaceAction()
{
  return mpFindReplaceAction;
}

QAction* MainWindow::getClearFindReplaceTextsAction()
{
  return mpClearFindReplaceTextsAction;
}

QAction* MainWindow::getGotoLineNumberAction()
{
  return mpGotoLineNumberAction;
}

//! Adds the currently opened file to the recentFilesList settings.
void MainWindow::addRecentFile(const QString &fileName, const QString &encoding)
{
  QSettings *pSettings = OpenModelica::getApplicationSettings();
  QList<QVariant> files = pSettings->value("recentFilesList/files").toList();
  // remove the already present RecentFile instance from the list.
  foreach (QVariant file, files)
  {
    RecentFile recentFile = qvariant_cast<RecentFile>(file);
    QFileInfo file1(recentFile.fileName);
    QFileInfo file2(fileName);
    if (file1.absoluteFilePath().compare(file2.absoluteFilePath()) == 0)
      files.removeOne(file);
  }
  RecentFile recentFile;
  recentFile.fileName = fileName;
  recentFile.encoding = encoding;
  files.prepend(QVariant::fromValue(recentFile));
  while (files.size() > MaxRecentFiles)
    files.removeLast();
  pSettings->setValue("recentFilesList/files", files);
  updateRecentFileActions();
}

//! Updates the actions of the recent files menu items.
void MainWindow::updateRecentFileActions()
{
  /* first set all recent files actions visibility to false. */
  for (int i = 0; i < MaxRecentFiles; ++i)
    mpRecentFileActions[i]->setVisible(false);
  /* read the new recent files list */
  QSettings *pSettings = OpenModelica::getApplicationSettings();
  QList<QVariant> files = pSettings->value("recentFilesList/files").toList();
  int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);
  for (int i = 0; i < numRecentFiles; ++i)
  {
    RecentFile recentFile = qvariant_cast<RecentFile>(files[i]);
    mpRecentFileActions[i]->setText(recentFile.fileName);
    QStringList dataList;
    dataList << recentFile.fileName << recentFile.encoding;
    mpRecentFileActions[i]->setData(dataList);
    mpRecentFileActions[i]->setVisible(true);
  }
  mpWelcomePageWidget->addRecentFilesListItems();
}

//! Event triggered re-implemented method that closes the main window.
//! First all tabs (models) are closed, if the user do not push Cancel
//! (closeAllProjectTabs then returns 'false') the event is accepted and
//! the main window is closed.
//! @param event contains information of the closing operation.
void MainWindow::closeEvent(QCloseEvent *event)
{
  SaveChangesDialog *pSaveChangesDialog = new SaveChangesDialog(this);
  if (pSaveChangesDialog->exec())
  {
    if (askForExit())
    {
      beforeClosingMainWindow();
      event->accept();
    }
    else
    {
      event->ignore();
    }
  }
  else
  {
    event->ignore();
  }
}

int MainWindow::askForExit()
{
  if (!mpOptionsDialog->getNotificationsPage()->getQuitApplicationCheckBox()->isChecked())
  {
    NotificationsDialog *pNotificationsDialog = new NotificationsDialog(NotificationsDialog::QuitApplication,
                                                                        NotificationsDialog::QuestionIcon, this);
    return pNotificationsDialog->exec();
  }
  return true;
}

void MainWindow::beforeClosingMainWindow()
{
  mpOMCProxy->stopServer();
  delete mpOMCProxy;
  delete mpModelWidgetContainer;
  delete mpDebuggerMainWindow;
  delete mpSimulationDialog;
  /* save the TransformationsWidget last window geometry and splitters state. */
  QSettings *pSettings = OpenModelica::getApplicationSettings();
  QHashIterator<QString, TransformationsWidget*> transformationsWidgets(mTransformationsWidgetHash);
  if (mTransformationsWidgetHash.size() > 0)
  {
    transformationsWidgets.toBack();
    transformationsWidgets.previous();
    TransformationsWidget *pTransformationsWidget = transformationsWidgets.value();
    if (pTransformationsWidget)
    {
      pSettings->beginGroup("transformationalDebugger");
      pSettings->setValue("geometry", pTransformationsWidget->saveGeometry());
      pSettings->setValue("variablesNestedHorizontalSplitter", pTransformationsWidget->getVariablesNestedHorizontalSplitter()->saveState());
      pSettings->setValue("variablesNestedVerticalSplitter", pTransformationsWidget->getVariablesNestedVerticalSplitter()->saveState());
      pSettings->setValue("variablesHorizontalSplitter", pTransformationsWidget->getVariablesHorizontalSplitter()->saveState());
      pSettings->setValue("equationsNestedHorizontalSplitter", pTransformationsWidget->getEquationsNestedHorizontalSplitter()->saveState());
      pSettings->setValue("equationsNestedVerticalSplitter", pTransformationsWidget->getEquationsNestedVerticalSplitter()->saveState());
      pSettings->setValue("equationsHorizontalSplitter", pTransformationsWidget->getEquationsHorizontalSplitter()->saveState());
      pSettings->setValue("transformationsVerticalSplitter", pTransformationsWidget->getTransformationsVerticalSplitter()->saveState());
      pSettings->setValue("transformationsHorizontalSplitter", pTransformationsWidget->getTransformationsHorizontalSplitter()->saveState());
      pSettings->endGroup();
    }
  }
  /* delete the TransformationsWidgets */
  transformationsWidgets.toFront();
  while (transformationsWidgets.hasNext())
  {
    transformationsWidgets.next();
    TransformationsWidget *pTransformationsWidget = transformationsWidgets.value();
    delete pTransformationsWidget;
  }
  mTransformationsWidgetHash.clear();
  /* save OMEdit MainWindow geometry state */
  pSettings->setValue("application/geometry", saveGeometry());
  pSettings->setValue("application/windowState", saveState());
  // save last Open Directory location
  pSettings->setValue("lastOpenDirectory", StringHandler::getLastOpenDirectory());
  // save the grid lines
  pSettings->setValue("modeling/gridLines", mpShowGridLinesAction->isChecked());
  delete pSettings;
}

void MainWindow::openDroppedFile(QDropEvent *event)
{
  int progressValue = 0;
  mpProgressBar->setRange(0, event->mimeData()->urls().size());
  showProgressBar();
  //retrieves the filenames of all the dragged files in list and opens the valid files.
  foreach (QUrl fileUrl, event->mimeData()->urls())
  {
    QFileInfo fileInfo(fileUrl.toLocalFile());
    // show file loading message
    mpStatusBar->showMessage(QString(Helper::loading).append(": ").append(fileInfo.absoluteFilePath()));
    mpProgressBar->setValue(++progressValue);
    // check the file extension
    QRegExp resultFilesRegExp("\\b(mat|plt|csv)\\b");
    if (fileInfo.suffix().compare("mo", Qt::CaseInsensitive) == 0)
    {
      mpLibraryTreeWidget->openFile(fileInfo.absoluteFilePath(), Helper::utf8, false);
    }
    else if (resultFilesRegExp.indexIn(fileInfo.suffix()) != -1)
    {
      openResultFiles(QStringList(fileInfo.absoluteFilePath()));
    }
    else
    {
      QMessageBox *pMessageBox = new QMessageBox(this);
      pMessageBox->setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::error));
      pMessageBox->setIcon(QMessageBox::Critical);
      pMessageBox->setText(GUIMessages::getMessage(GUIMessages::FILE_FORMAT_NOT_SUPPORTED).arg(fileInfo.fileName())
                           .arg(Helper::omFileTypes));
      pMessageBox->setStandardButtons(QMessageBox::Ok);
      pMessageBox->exec();
    }
  }
  mpStatusBar->clearMessage();
  hideProgressBar();
}

void MainWindow::openResultFiles(QStringList fileNames)
{
  QString currentDirectory = mpOMCProxy->changeDirectory();
  foreach (QString fileName, fileNames)
  {
    QFileInfo fileInfo(fileName);
    mpOMCProxy->changeDirectory(fileInfo.absoluteDir().absolutePath());
    QStringList list = mpOMCProxy->readSimulationResultVars(fileInfo.fileName());
    // close the simulation result file.
    mpOMCProxy->closeSimulationResultFile();
    if (list.size() > 0) {
      mpPerspectiveTabbar->setCurrentIndex(2);
      mpVariablesWidget->insertVariablesItemsToTree(fileInfo.fileName(), fileInfo.absoluteDir().absolutePath(), list, SimulationOptions());
      mpVariablesDockWidget->show();
    }
  }
  mpOMCProxy->changeDirectory(currentDirectory);
}

void MainWindow::simulate(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  mpSimulationDialog->directSimulate(pLibraryTreeNode, false, false);
}

void MainWindow::simulateWithTransformationalDebugger(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  mpSimulationDialog->directSimulate(pLibraryTreeNode, true, false);
}

void MainWindow::simulateWithAlgorithmicDebugger(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  mpSimulationDialog->directSimulate(pLibraryTreeNode, false, true);
}

void MainWindow::simulationSetup(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  mpSimulationDialog->show(pLibraryTreeNode, false, SimulationOptions());
}

void MainWindow::instantiatesModel(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  // set the status message.
  mpStatusBar->showMessage(QString(Helper::instantiateModel).append(" ").append(pLibraryTreeNode->getNameStructure()));
  // show the progress bar
  mpProgressBar->setRange(0, 0);
  showProgressBar();
  bool instantiateModelSuccess = false;
  QString instantiateModelResult = mpOMCProxy->instantiateModel(pLibraryTreeNode->getNameStructure(), &instantiateModelSuccess);
  QString windowTitle = QString(Helper::instantiateModel).append(" - ").append(pLibraryTreeNode->getName());
  InformationDialog *pInformationDialog = new InformationDialog(windowTitle, instantiateModelResult, instantiateModelSuccess, this);
  pInformationDialog->show();
  // hide progress bar
  hideProgressBar();
  // clear the status bar message
  mpStatusBar->clearMessage();
}

void MainWindow::checkModel(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  // set the status message.
  mpStatusBar->showMessage(QString(Helper::checkModel).append(" ").append(pLibraryTreeNode->getNameStructure()));
  // show the progress bar
  mpProgressBar->setRange(0, 0);
  showProgressBar();
  QString checkModelResult = mpOMCProxy->checkModel(pLibraryTreeNode->getNameStructure());
  QString windowTitle = QString(Helper::checkModel).append(" - ").append(pLibraryTreeNode->getName());
  InformationDialog *pInformationDialog = new InformationDialog(windowTitle, checkModelResult, false, this);
  pInformationDialog->show();
  // hide progress bar
  hideProgressBar();
  // clear the status bar message
  mpStatusBar->clearMessage();
}

void MainWindow::checkAllModels(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  // set the status message.
  mpStatusBar->showMessage(QString(Helper::checkModel).append(" ").append(pLibraryTreeNode->getNameStructure()));
  // show the progress bar
  mpProgressBar->setRange(0, 0);
  showProgressBar();
  QString checkModelResult = mpOMCProxy->checkAllModelsRecursive(pLibraryTreeNode->getNameStructure());
  if (checkModelResult.length())
  {
    QString windowTitle = QString(Helper::checkModel).append(" - ").append(pLibraryTreeNode->getName());
    InformationDialog *pInformationDialog = new InformationDialog(windowTitle, checkModelResult, false, this);
    pInformationDialog->show();
  }
  else
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, "Check of " + pLibraryTreeNode->getName() + " failed.",
                                                    Helper::scriptingKind, Helper::notificationLevel, 0));
  }
  // hide progress bar
  hideProgressBar();
  // clear the status bar message
  mpStatusBar->clearMessage();
}

void MainWindow::exportModelFMU(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  // set the status message.
  mpStatusBar->showMessage(tr("Exporting model as FMU"));
  // show the progress bar
  mpProgressBar->setRange(0, 0);
  showProgressBar();
  if (mpOMCProxy->translateModelFMU(pLibraryTreeNode->getNameStructure()))
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::FMU_GENERATED)
                                                    .arg(mpOMCProxy->changeDirectory()).arg(pLibraryTreeNode->getNameStructure()),
                                                    Helper::scriptingKind, Helper::notificationLevel, 0));
  }
  // hide progress bar
  hideProgressBar();
  // clear the status bar message
  mpStatusBar->clearMessage();
}

void MainWindow::exportModelXML(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  // set the status message.
  mpStatusBar->showMessage(tr("Exporting model as XML"));
  // show the progress bar
  mpProgressBar->setRange(0, 0);
  showProgressBar();
  if (mpOMCProxy->translateModelXML(pLibraryTreeNode->getNameStructure()))
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::XML_GENERATED)
                                                    .arg(mpOMCProxy->changeDirectory()).arg(pLibraryTreeNode->getNameStructure()),
                                                    Helper::scriptingKind, Helper::notificationLevel, 0));
  }
  // hide progress bar
  hideProgressBar();
  // clear the status bar message
  mpStatusBar->clearMessage();
}

void MainWindow::exportModelFigaro(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  // set the status message.
  mpStatusBar->showMessage(tr("Exporting model as Figaro"));
  // show the progress bar
  mpProgressBar->setRange(0, 0);
  showProgressBar();
  FigaroPage *pFigaroPage = mpOptionsDialog->getFigaroPage();
  QString library = pFigaroPage->getFigaroDatabaseFileTextBox()->text();
  QString mode = pFigaroPage->getFigaroModeComboBox()->itemData(pFigaroPage->getFigaroModeComboBox()->currentIndex()).toString();
  QString options = pFigaroPage->getFigaroOptionsTextBox()->text();
  QString processor = pFigaroPage->getFigaroProcessTextBox()->text();
  if (mpOMCProxy->exportToFigaro(pLibraryTreeNode->getNameStructure(), library, mode, options, processor))
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::FIGARO_GENERATED),
                                                    Helper::scriptingKind, Helper::notificationLevel, 0));
  }
  // hide progress bar
  hideProgressBar();
  // clear the status bar message
  mpStatusBar->clearMessage();
}

void MainWindow::exportModelToOMNotebook(LibraryTreeNode *pLibraryTreeNode)
{
  /* if Modelica text is changed manually by user then validate it before saving. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return;
  }
  QString omnotebookFileName = StringHandler::getSaveFileName(this, QString(Helper::applicationName).append(" - ").append(Helper::exportToOMNotebook),
                                                              NULL, Helper::omnotebookFileTypes, NULL, "onb", &pLibraryTreeNode->getName());
  // if user cancels the operation. or closes the export dialog box.
  if (omnotebookFileName.isEmpty())
    return;
  // create a progress bar
  int endtime = 6;    // since in total we do six things while exporting to OMNotebook
  int value = 1;
  // set the status message.
  mpStatusBar->showMessage(tr("Exporting model to OMNotebook"));
  // show the progress bar
  mpProgressBar->setRange(0, endtime);
  showProgressBar();
  // create the xml for the omnotebook file.
  QDomDocument xmlDocument;
  // create Notebook element
  QDomElement notebookElement = xmlDocument.createElement("Notebook");
  xmlDocument.appendChild(notebookElement);
  mpProgressBar->setValue(value++);
  // create title cell
  createOMNotebookTitleCell(pLibraryTreeNode, xmlDocument, notebookElement);
  mpProgressBar->setValue(value++);
  // create image cell
  QStringList pathList = omnotebookFileName.split('/');
  pathList.removeLast();
  QString modelImagePath(pathList.join("/"));
  createOMNotebookImageCell(pLibraryTreeNode, xmlDocument, notebookElement, modelImagePath);
  mpProgressBar->setValue(value++);
  // create a code cell
  createOMNotebookCodeCell(pLibraryTreeNode, xmlDocument, notebookElement);
  mpProgressBar->setValue(value++);
  // create a file object and write the xml in it.
  QFile omnotebookFile(omnotebookFileName);
  omnotebookFile.open(QIODevice::WriteOnly);
  QTextStream textStream(&omnotebookFile);
  textStream.setCodec(Helper::utf8.toStdString().data());
  textStream.setGenerateByteOrderMark(false);
  textStream << xmlDocument.toString();
  omnotebookFile.close();
  mpProgressBar->setValue(value++);
  // hide the progressbar and clear the message in status bar
  mpStatusBar->clearMessage();
  hideProgressBar();
}

//! creates a title cell in omnotebook xml file
void MainWindow::createOMNotebookTitleCell(LibraryTreeNode *pLibraryTreeNode, QDomDocument xmlDocument, QDomElement domElement)
{
  QDomElement textCellElement = xmlDocument.createElement("TextCell");
  textCellElement.setAttribute("style", "Text");
  domElement.appendChild(textCellElement);
  // create text Element
  QDomElement textElement = xmlDocument.createElement("Text");
  textElement.appendChild(xmlDocument.createTextNode("<html><head><meta name=\"qrichtext\" content=\"1\" /><head><body style=\"white-space: pre-wrap; font-family:MS Shell Dlg; font-size:8.25pt; font-weight:400; font-style:normal; text-decoration:none;\"><p style=\"margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-family:Arial; font-size:38pt; font-weight:600; color:#000000;\">" + pLibraryTreeNode->getName() + "</p></body></html>"));
  textCellElement.appendChild(textElement);
}

//! creates a image cell in omnotebook xml file
void MainWindow::createOMNotebookImageCell(LibraryTreeNode *pLibraryTreeNode, QDomDocument xmlDocument, QDomElement domElement,
                                           QString filePath)
{
  GraphicsView *pGraphicsView = pLibraryTreeNode->getModelWidget()->getDiagramGraphicsView();
  QPixmap modelImage(pGraphicsView->viewport()->size());
  modelImage.fill(QColor(Qt::transparent));
  QPainter painter(&modelImage);
  painter.setWindow(pGraphicsView->viewport()->rect());
  // paint the background color first
  painter.fillRect(modelImage.rect(), pGraphicsView->palette().background());
  // paint all the items
  pGraphicsView->render(&painter, QRectF(painter.viewport()), pGraphicsView->viewport()->rect());
  painter.end();
  // create textcell element
  QDomElement textCellElement = xmlDocument.createElement("TextCell");
  domElement.appendChild(textCellElement);
  // create text Element
  QDomElement textElement = xmlDocument.createElement("Text");
  textElement.appendChild(xmlDocument.createTextNode("<img src=\""+QString(filePath).append("/OMNotebook_tempfiles/1.png")+"\" />"));
  textCellElement.appendChild(textElement);
  // create rule Element
  QDomElement ruleElement = xmlDocument.createElement("Rule");
  ruleElement.setAttribute("name", "TextAlignment");
  ruleElement.appendChild(xmlDocument.createTextNode("Center"));
  textCellElement.appendChild(ruleElement);
  // create image Element
  QDomElement imageElement = xmlDocument.createElement("Image");
  imageElement.setAttribute("name", QString(filePath).append("/OMNotebook_tempfiles/1.png"));
  // get the base64 encoding of image
  QBuffer imageBuffer;
  imageBuffer.open(QBuffer::WriteOnly);
  QDataStream out(&imageBuffer);
  out << modelImage;
  imageBuffer.close();
  imageElement.appendChild(xmlDocument.createTextNode(imageBuffer.buffer().toBase64()));
  textCellElement.appendChild(imageElement);
}

//! creates a code cell in omnotebook xml file
void MainWindow::createOMNotebookCodeCell(LibraryTreeNode *pLibraryTreeNode, QDomDocument xmlDocument, QDomElement domElement)
{
  QDomElement textCellElement = xmlDocument.createElement("InputCell");
  domElement.appendChild(textCellElement);
  // create input Element
  QDomElement inputElement = xmlDocument.createElement("Input");
  inputElement.appendChild(xmlDocument.createTextNode(mpOMCProxy->list(pLibraryTreeNode->getNameStructure())));
  textCellElement.appendChild(inputElement);
  // create output Element
  QDomElement outputElement = xmlDocument.createElement("Output");
  outputElement.appendChild(xmlDocument.createTextNode(""));
  textCellElement.appendChild(outputElement);
}

TransformationsWidget *MainWindow::showTransformationsWidget(QString fileName)
{
  TransformationsWidget *pTransformationsWidget = mTransformationsWidgetHash.value(fileName, 0);
  if (!pTransformationsWidget)
  {
    pTransformationsWidget = new TransformationsWidget(fileName, this);
    mTransformationsWidgetHash.insert(fileName, pTransformationsWidget);
  }
  else
  {
    pTransformationsWidget->reloadTransformations();
  }
  pTransformationsWidget->show();
  pTransformationsWidget->raise();
  pTransformationsWidget->activateWindow();
  pTransformationsWidget->setWindowState(pTransformationsWidget->windowState() & (~Qt::WindowMinimized | Qt::WindowActive));
  return pTransformationsWidget;
}

//! Opens the new model widget.
void MainWindow::createNewModelicaClass()
{
  ModelicaClassDialog *pModelicaClassDialog = new ModelicaClassDialog(this);
  pModelicaClassDialog->exec();
}

void MainWindow::createNewTLMFile()
{
  LibraryTreeNode *pLibraryTreeNode = mpLibraryTreeWidget->addLibraryTreeNode("TLM", false);
  pLibraryTreeNode->setSaveContentsType(LibraryTreeNode::SaveInOneFile);
  mpLibraryTreeWidget->addToExpandedLibraryTreeNodesList(pLibraryTreeNode);
  mpLibraryTreeWidget->showModelWidget(pLibraryTreeNode);
}

void MainWindow::openModelicaFile()
{
  QStringList fileNames;
  fileNames = StringHandler::getOpenFileNames(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseFiles),
                                              NULL, Helper::omFileTypes, NULL);
  if (fileNames.isEmpty())
    return;
  int progressValue = 0;
  mpProgressBar->setRange(0, fileNames.size());
  showProgressBar();
  foreach (QString file, fileNames)
  {
    file = file.replace("\\", "/");
    mpStatusBar->showMessage(QString(Helper::loading).append(": ").append(file));
    mpProgressBar->setValue(++progressValue);
    // if file doesn't exists
    if (!QFile::exists(file))
    {
      QMessageBox *pMessageBox = new QMessageBox(this);
      pMessageBox->setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::error));
      pMessageBox->setIcon(QMessageBox::Critical);
      pMessageBox->setText(QString(GUIMessages::getMessage(GUIMessages::UNABLE_TO_LOAD_FILE).arg(file)));
      pMessageBox->setInformativeText(QString(GUIMessages::getMessage(GUIMessages::FILE_NOT_FOUND).arg(file)));
      pMessageBox->setStandardButtons(QMessageBox::Ok);
      pMessageBox->exec();
    }
    else
    {
      mpLibraryTreeWidget->openFile(file, Helper::utf8, false);
    }
  }
  mpStatusBar->clearMessage();
  hideProgressBar();
}

void MainWindow::showOpenModelicaFileDialog()
{
  OpenModelicaFile *pOpenModelicaFile = new OpenModelicaFile(this);
  pOpenModelicaFile->show();
}

void MainWindow::loadModelicaLibrary()
{
  QString libraryPath = StringHandler::getExistingDirectory(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseDirectory), NULL);
  if (libraryPath.isEmpty())
    return;
  libraryPath = libraryPath + QDir::separator() + "package.mo";
  libraryPath = libraryPath.replace("\\", "/");
  mpLibraryTreeWidget->openFile(libraryPath, Helper::utf8, true, true);
}

void MainWindow::showOpenResultFileDialog()
{
  QStringList fileNames = StringHandler::getOpenFileNames(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseFiles),
                                                          NULL, Helper::omResultFileTypes, NULL);
  if (fileNames.isEmpty())
    return;
  openResultFiles(fileNames);
}

void MainWindow::showOpenTransformationFileDialog()
{
  QString fileName = StringHandler::getOpenFileName(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseFile),
                                                    NULL, Helper::infoXmlFileTypes, NULL);
  if (fileName.isEmpty())
    return;

  showTransformationsWidget(fileName);
}

void MainWindow::loadSystemLibrary()
{
  QAction *pAction = qobject_cast<QAction*>(sender());
  if (pAction)
  {
    /* check if library is already loaded. */
    if (mpOMCProxy->existClass(pAction->text()))
    {
      QMessageBox *pMessageBox = new QMessageBox(this);
      pMessageBox->setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::information));
      pMessageBox->setIcon(QMessageBox::Information);
      pMessageBox->setText(QString(GUIMessages::getMessage(GUIMessages::UNABLE_TO_LOAD_FILE).arg(pAction->text())));
      pMessageBox->setInformativeText(QString(GUIMessages::getMessage(GUIMessages::REDEFINING_EXISTING_CLASSES))
                                      .arg(pAction->text()).append("\n")
                                      .append(GUIMessages::getMessage(GUIMessages::DELETE_AND_LOAD).arg(pAction->text())));
      pMessageBox->setStandardButtons(QMessageBox::Ok);
      pMessageBox->exec();
    }
    /* if library is not loaded then load it. */
    else
    {
      mpProgressBar->setRange(0, 0);
      showProgressBar();
      mpStatusBar->showMessage(QString(Helper::loading).append(": ").append(pAction->text()));
      if (mpOMCProxy->loadModel(pAction->text()))
      {
        /* since few libraries load dependent libraries automatically. So if the dependent library is not loaded then load it. */
        QStringList systemLibs = mpOMCProxy->getClassNames("");
        foreach (QString systemLib, systemLibs)
        {
          LibraryTreeNode* pLoadedLibraryTreeNode = mpLibraryTreeWidget->getLibraryTreeNode(systemLib);
          if (!pLoadedLibraryTreeNode)
          {
            LibraryTreeNode *pLibraryTreeNode = mpLibraryTreeWidget->addLibraryTreeNode(systemLib,
                                                                                        mpOMCProxy->getClassRestriction(pAction->text()), "");
            pLibraryTreeNode->setSystemLibrary(true);
            /* since LibraryTreeWidget::addLibraryTreeNode clears the status bar message, so we should set it one more time. */
            mpStatusBar->showMessage(tr("Parsing").append(": ").append(systemLib));
            // create library tree nodes
            mpLibraryTreeWidget->createLibraryTreeNodes(pLibraryTreeNode);
          }
        }
      }
      mpStatusBar->clearMessage();
      hideProgressBar();
    }
  }
}

void MainWindow::focusSearchClassWidget(bool visible)
{
  if (visible)
    mpSearchClassWidget->getSearchClassTextBox()->setFocus();
}

void MainWindow::showFindReplaceDialog()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    mpFindReplaceDialog->setTextEdit(pModelWidget->getModelicaTextEditor());
    mpFindReplaceDialog->show();
    mpFindReplaceDialog->raise();
    mpFindReplaceDialog->activateWindow();
  }
}

void MainWindow::showGotoLineNumberDialog()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    GotoLineDialog *pGotoLineWidget = new GotoLineDialog(pModelWidget->getModelicaTextEditor(), this);
    pGotoLineWidget->show();
  }
}

//! Opens the recent file.
void MainWindow::openRecentFile()
{
  QAction *pAction = qobject_cast<QAction*>(sender());
  if (pAction)
  {
    QStringList dataList = pAction->data().toStringList();
    mpLibraryTreeWidget->openFile(dataList.at(0), dataList.at(1), true, true);
  }
}

void MainWindow::clearRecentFilesList()
{
  QSettings *pSettings = OpenModelica::getApplicationSettings();
  pSettings->remove("recentFilesList/files");
  updateRecentFileActions();
  mpWelcomePageWidget->addRecentFilesListItems();
}

void MainWindow::clearFindReplaceTexts()
{
  QSettings *pSettings = OpenModelica::getApplicationSettings();
  pSettings->remove("findReplaceDialog/textsToFind");
  mpFindReplaceDialog->readFindTextFromSettings();
}

void MainWindow::setShowGridLines(bool showLines)
{
  mpModelWidgetContainer->setShowGridLines(showLines);
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    if (pModelWidget->getDiagramGraphicsView()->isVisible())
      pModelWidget->getDiagramGraphicsView()->scene()->update();
    else if (pModelWidget->getIconGraphicsView()->isVisible())
      pModelWidget->getIconGraphicsView()->scene()->update();
  }
}

//! Tells the current model to reset zoom to 100%.
//! @see zoomIn()
//! @see zoomOut()
void MainWindow::resetZoom()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    if (pModelWidget->getDiagramGraphicsView()->isVisible())
      pModelWidget->getDiagramGraphicsView()->resetZoom();
    else if (pModelWidget->getIconGraphicsView()->isVisible())
      pModelWidget->getIconGraphicsView()->resetZoom();
  }
}

//! Tells the current model to increase its zoom factor.
//! @see resetZoom()
//! @see zoomOut()
void MainWindow::zoomIn()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    if (pModelWidget->getDiagramGraphicsView()->isVisible())
      pModelWidget->getDiagramGraphicsView()->zoomIn();
    else if (pModelWidget->getIconGraphicsView()->isVisible())
      pModelWidget->getIconGraphicsView()->zoomIn();
  }
}

//! Tells the current model to decrease its zoom factor.
//! @see resetZoom()
//! @see zoomIn()
void MainWindow::zoomOut()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    if (pModelWidget->getDiagramGraphicsView()->isVisible())
      pModelWidget->getDiagramGraphicsView()->zoomOut();
    else if (pModelWidget->getIconGraphicsView()->isVisible())
      pModelWidget->getIconGraphicsView()->zoomOut();
  }
}

void MainWindow::showAlgorithmicDebugger()
{
  mpDebuggerMainWindow->show();
  mpDebuggerMainWindow->raise();
  mpDebuggerMainWindow->activateWindow();
  mpDebuggerMainWindow->setWindowState(mpDebuggerMainWindow->windowState() & (~Qt::WindowMinimized | Qt::WindowActive));
  mpDebuggerMainWindow->restoreWindows();
}

/*!
  Slot activated when mpCascadeWindowsAction triggered signal is raised.\n
  Arranges all child windows in a cascade pattern.
  */
void MainWindow::cascadeSubWindows()
{
  switch (mpCentralStackedWidget->currentIndex()) {
    case 1:
      mpModelWidgetContainer->cascadeSubWindows();
      break;
    case 2:
      mpPlotWindowContainer->cascadeSubWindows();
    default:
      break;
  }
}

/*!
  Slot activated when mpTileWindowsHorizontallyAction triggered signal is raised.\n
  Arranges all child windows in a horizontally tiled pattern.
  */
void MainWindow::tileSubWindowsHorizontally()
{
  switch (mpCentralStackedWidget->currentIndex()) {
    case 1:
      tileSubWindows(mpModelWidgetContainer, true);
      break;
    case 2:
      tileSubWindows(mpPlotWindowContainer, true);
    default:
      break;
  }
}

/*!
  Slot activated when mpTileWindowsVerticallyAction triggered signal is raised.\n
  Arranges all child windows in a vertically tiled pattern.
  */
void MainWindow::tileSubWindowsVertically()
{
  switch (mpCentralStackedWidget->currentIndex()) {
    case 1:
      tileSubWindows(mpModelWidgetContainer, false);
      break;
    case 2:
      tileSubWindows(mpPlotWindowContainer, false);
    default:
      break;
  }
}

void MainWindow::instantiatesModel()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode)
      instantiatesModel(pLibraryTreeNode);
  }
  else
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::NO_MODELICA_CLASS_OPEN)
                                                    .arg(tr("instantiating")), Helper::scriptingKind, Helper::notificationLevel, 0));
  }
}

void MainWindow::checkModel()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode)
      checkModel(pLibraryTreeNode);
  }
  else
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::NO_MODELICA_CLASS_OPEN)
                                                    .arg(tr("checking")), Helper::scriptingKind, Helper::notificationLevel, 0));
  }
}

void MainWindow::checkAllModels()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode)
      checkAllModels(pLibraryTreeNode);
  }
  else
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::NO_MODELICA_CLASS_OPEN)
                                                    .arg(tr("checking")), Helper::scriptingKind, Helper::notificationLevel, 0));
  }
}

/*!
  Simualtes the model directly.
  */
//!
void MainWindow::simulateModel()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode)
      simulate(pLibraryTreeNode);
  }
}

/*!
  Simualtes the model with transformational debugger
  */
void MainWindow::simulateModelWithTransformationalDebugger()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode)
      simulateWithTransformationalDebugger(pLibraryTreeNode);
  }
}

/*!
  Simualtes the model with algorithmic debugger
  */
void MainWindow::simulateModelWithAlgorithmicDebugger()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode)
      simulateWithAlgorithmicDebugger(pLibraryTreeNode);
  }
}

/*!
  Opens the Simualtion Dialog
  */
void MainWindow::openSimulationDialog()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget) {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode) {
      simulationSetup(pLibraryTreeNode);
    }
  }
}

//! Open Interactive Simulation Window
void MainWindow::openInteractiveSimulation()
{
  //mpSimulationDialog->show(true);
}

//! Exports the current model to FMU
void MainWindow::exportModelFMU()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode)
      exportModelFMU(pLibraryTreeNode);
  }
  else
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::NO_MODELICA_CLASS_OPEN)
                                                    .arg(tr("making FMU")), Helper::scriptingKind, Helper::notificationLevel, 0));
  }
}

//! Exports the current model to XML
void MainWindow::exportModelXML()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode)
      exportModelXML(pLibraryTreeNode);
  }
  else
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::NO_MODELICA_CLASS_OPEN)
                                                    .arg(tr("making XML")), Helper::scriptingKind, Helper::notificationLevel, 0));
  }
}

//! Exports the current model to XML
void MainWindow::exportModelFigaro()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode)
      exportModelFigaro(pLibraryTreeNode);
  }
  else
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::NO_MODELICA_CLASS_OPEN)
                                                    .arg(tr("exporting to Figaro")), Helper::scriptingKind, Helper::notificationLevel, 0));
  }
}

//! Imports the model from FMU
void MainWindow::importModelFMU()
{
  ImportFMUDialog *pImportFMUDialog = new ImportFMUDialog(this);
  pImportFMUDialog->show();
}

//! Exports the current model to OMNotebook.
//! Creates a new onb file and add the model text and model image in it.
//! @see importModelfromOMNotebook();
void MainWindow::exportModelToOMNotebook()
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget)
  {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode)
      exportModelToOMNotebook(pLibraryTreeNode);
  }
  else
  {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::NO_MODELICA_CLASS_OPEN)
                                                    .arg(tr("exporting to OMNotebook")), Helper::scriptingKind, Helper::notificationLevel, 0));
  }
}

//! Imports the models from OMNotebook.
//! @see exportModelToOMNotebook();
void MainWindow::importModelfromOMNotebook()
{
  QString fileName = StringHandler::getOpenFileName(this, QString(Helper::applicationName).append(" - ").append(Helper::importFromOMNotebook),
                                                    NULL, Helper::omnotebookFileTypes);
  if (fileName.isEmpty())
    return;
  // create a progress bar
  int endtime = 3;    // since in total we do three things while importing from OMNotebook
  int value = 1;
  // show the progressbar and set the message in status bar
  mpStatusBar->showMessage(tr("Importing model(s) from OMNotebook"));
  mpProgressBar->setRange(0, endtime);
  showProgressBar();
  // open the xml file
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly))
  {
    QMessageBox::critical(this, QString(Helper::applicationName).append(" - ").append(Helper::error),
                          GUIMessages::getMessage(GUIMessages::ERROR_OPENING_FILE).arg(fileName).arg(file.errorString()), Helper::ok);
    hideProgressBar();
    return;
  }
  mpProgressBar->setValue(value++);
  // create the xml from the omnotebook file.
  QDomDocument xmlDocument;
  if (!xmlDocument.setContent(&file))
  {
    QMessageBox::critical(this, QString(Helper::applicationName).append(" - ").append(Helper::error),
                          tr("Error reading the xml file"), Helper::ok);
    hideProgressBar();
    return;
  }
  mpProgressBar->setValue(value++);
  // read the file
  QDomNodeList nodes = xmlDocument.elementsByTagName("Input");
  endtime = endtime + nodes.size();
  mpProgressBar->setMaximum(endtime);
  for (int i = 0; i < nodes.size(); i++)
  {
    if (nodes.at(i).toElement().text().toLower().startsWith("model"))
    {
      mpLibraryTreeWidget->parseAndLoadModelicaText(nodes.at(i).toElement().text());
    }
    mpProgressBar->setValue(value++);
  }
  file.close();
  // hide the progressbar and clear the message in status bar
  mpStatusBar->clearMessage();
  hideProgressBar();
}

// Tool to convert ngspice netlist to modelica code - added by Rakhi
void MainWindow::importNgspiceNetlist()
{
  QString fileName = StringHandler::getOpenFileName(this, QString(Helper::applicationName).append(" - ").append(Helper::importNgspiceNetlist),
                                                    NULL, Helper::ngspiceNetlistFileTypes);
  if (fileName.isEmpty())
    return;
  // create a progress bar
  int endtime = 0;
  // show the progress bar and set the message in status bar
  mpStatusBar->showMessage(tr("Importing ngspice netlist and converting to Modelica code"));
  mpProgressBar->setRange(0, endtime);
  showProgressBar();
  if (mpOMCProxy->ngspicetoModelica(fileName))
  {
    QFileInfo fileInfo(fileName);
    QString modelicaFile = QString(fileInfo.absoluteDir().absolutePath()).append("/").append(fileInfo.baseName()).append(".mo");
    mpLibraryTreeWidget->openFile(modelicaFile, Helper::utf8, true, true);
  }
  // hide the progress bar and clear the message in status bar
  mpStatusBar->clearMessage();
  hideProgressBar();
}

//! Exports the current model as image
void MainWindow::exportModelAsImage(bool copyToClipboard)
{
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget) {
    LibraryTreeNode *pLibraryTreeNode = pModelWidget->getLibraryTreeNode();
    if (pLibraryTreeNode) {
      QString fileName;
      if (!copyToClipboard) {
        fileName = StringHandler::getSaveFileName(this, QString(Helper::applicationName).append(" - ").append(Helper::exportAsImage),
                                                  NULL, Helper::imageFileTypes, NULL, "svg", &pLibraryTreeNode->getName());
        // if user cancels the operation. or closes the export dialog box.
        if (fileName.isEmpty()) {
          return;
        }
      }
      bool oldSkipDrawBackground;
      // show the progressbar and set the message in status bar
      mpProgressBar->setRange(0, 0);
      showProgressBar();
      mpStatusBar->showMessage(tr("Exporting model as an Image"));
      QPainter painter;
      QSvgGenerator svgGenerator;
      GraphicsView *pGraphicsView;
      if (pLibraryTreeNode->getModelWidget()->getIconGraphicsView()->isVisible()) {
        pGraphicsView = pLibraryTreeNode->getModelWidget()->getIconGraphicsView();
      } else {
        pGraphicsView = pLibraryTreeNode->getModelWidget()->getDiagramGraphicsView();
      }
      QRect destinationRect = pGraphicsView->itemsBoundingRect().toAlignedRect();
      QImage modelImage(destinationRect.size(), QImage::Format_ARGB32_Premultiplied);
      // export svg
      if (fileName.endsWith(".svg")) {
        svgGenerator.setTitle(QString(Helper::applicationName).append(" - ").append(Helper::applicationIntroText));
        svgGenerator.setDescription("Generated by OMEdit - OpenModelica Connection Editor");
        svgGenerator.setSize(destinationRect.size());
        svgGenerator.setViewBox(QRect(0, 0, destinationRect.width(), destinationRect.height()));
        svgGenerator.setFileName(fileName);
        painter.begin(&svgGenerator);
      } else {
        if (fileName.endsWith(".png") || fileName.endsWith(".tiff")) {
          modelImage.fill(QColor(Qt::transparent));
        } else if (fileName.endsWith(".bmp") || copyToClipboard) {
          modelImage.fill(QColor(Qt::white));
        }
        painter.begin(&modelImage);
      }
      painter.setWindow(destinationRect);
      // paint all the items
      oldSkipDrawBackground = pGraphicsView->mSkipBackground;
      pGraphicsView->mSkipBackground = true;
      pGraphicsView->render(&painter, destinationRect, destinationRect);
      painter.end();
      pGraphicsView->mSkipBackground = oldSkipDrawBackground;
      if (!fileName.endsWith(".svg") && !copyToClipboard) {
        if (!modelImage.save(fileName)) {
          QMessageBox::critical(this, QString(Helper::applicationName).append(" - ").append(Helper::error),
                                tr("Error saving the image file"), Helper::ok);
        }
      } else if (copyToClipboard) {
        QClipboard *pClipboard = QApplication::clipboard();
        pClipboard->setImage(modelImage);
      }
      // hide the progressbar and clear the message in status bar
      mpStatusBar->clearMessage();
      hideProgressBar();
    }
  } else {
    mpMessagesWidget->addGUIMessage(new MessageItem("", false, 0, 0, 0, 0, GUIMessages::getMessage(GUIMessages::NO_MODELICA_CLASS_OPEN)
                                                    .arg(tr("exporting to Image")), Helper::scriptingKind, Helper::notificationLevel, 0));
  }
}

/*!
  Slot activated when mpExportToClipboardAction triggered signal is raised.\n
  Copies the current model to clipboard.
  */
void MainWindow::exportToClipboard()
{
  exportModelAsImage(true);
}

void MainWindow::openConfigurationOptions()
{
  mpOptionsDialog->show();
}

void MainWindow::openUsersGuide()
{
  QUrl usersGuidePath (QString("file:///").append(QString(Helper::OpenModelicaHome).replace("\\", "/"))
                       .append("/share/doc/omc/OpenModelicaUsersGuide.pdf"));
  QDesktopServices::openUrl(usersGuidePath);
}

void MainWindow::openSystemDocumentation()
{
  QUrl systemDocumentationPath (QString("file:///").append(QString(Helper::OpenModelicaHome).replace("\\", "/"))
                                .append("/share/doc/omc/OpenModelicaSystem.pdf"));
  QDesktopServices::openUrl(systemDocumentationPath);
}

void MainWindow::openOpenModelicaScriptingDocumentation()
{
  QUrl openModelicaScriptingUrl (QUrl("https://build.openmodelica.org/Documentation/OpenModelica.Scripting.html"));
  QDesktopServices::openUrl(openModelicaScriptingUrl);
}

void MainWindow::openModelicaDocumentation()
{
  QUrl modelicaDocumentationUrl (QUrl("https://build.openmodelica.org/Documentation/index.html"));
  QDesktopServices::openUrl(modelicaDocumentationUrl);
}

void MainWindow::openModelicaByExample()
{
  QUrl modelicaByExampleUrl (QUrl("http://book.xogeny.com"));
  QDesktopServices::openUrl(modelicaByExampleUrl);
}

void MainWindow::openModelicaWebReference()
{
  QUrl modelicaWebReference (QUrl("http://modref.xogeny.com"));
  QDesktopServices::openUrl(modelicaWebReference);
}

void MainWindow::openAboutOMEdit()
{
  mpAboutOMEditDialog->setGeometry(QRect(rect().center() - QPoint(262, 235), rect().center() + QPoint(262, 235)));
  mpAboutOMEditDialog->setFocus(Qt::ActiveWindowFocusReason);
  mpAboutOMEditDialog->raise();
  mpAboutOMEditDialog->show();
}

void MainWindow::toggleShapesButton()
{
  QAction *clickedAction = qobject_cast<QAction*>(const_cast<QObject*>(sender()));
  QList<QAction*> shapeActions = mpShapesActionGroup->actions();
  foreach (QAction *shapeAction, shapeActions) {
    if (shapeAction != clickedAction) {
      shapeAction->setChecked(false);
    }
  }
  // cancel connection if another tool is chosen
  ModelWidget *pModelWidget = mpModelWidgetContainer->getCurrentModelWidget();
  if (pModelWidget) {
    GraphicsView *pGraphicsView = pModelWidget->getDiagramGraphicsView();
    if (pGraphicsView->isCreatingConnection()) {
      pGraphicsView->removeConnection();
    }
  }
}

void MainWindow::openRecentModelWidget()
{
  QAction *action = qobject_cast<QAction*>(sender());
  if (action)
  {
    LibraryTreeNode *pLibraryTreeNode = mpLibraryTreeWidget->getLibraryTreeNode(action->data().toString());
    mpModelWidgetContainer->addModelWidget(pLibraryTreeNode->getModelWidget(), false);
  }
}

/*!
  Slot activated when Re-simulateAction triggered signal is raised.\n
  Re-simulates the model.
  */
void MainWindow::reSimulateModel()
{
  mpVariablesWidget->directReSimulate();
}

/*!
  Slot activated when Re-simulateSetupAction triggered signal is raised.\n
  Shows the simulation setup for re-simulation.
  */
void MainWindow::showReSimulateSetup()
{
  mpVariablesWidget->showReSimulateSetup();
}

void MainWindow::addNewPlotWindow()
{
  mpPlotWindowContainer->addPlotWindow();
}

void MainWindow::addNewParametricPlotWindow()
{
  mpPlotWindowContainer->addParametricPlotWindow();
}

void MainWindow::clearPlotWindow()
{
  mpPlotWindowContainer->clearPlotWindow();
}

//! shows the progress bar contained inside the status bar
//! @see hideProgressBar()
void MainWindow::showProgressBar()
{
  mpProgressBar->setVisible(true);
}

//! hides the progress bar contained inside the status bar
//! @see hideProgressBar()
void MainWindow::hideProgressBar()
{
  mpProgressBar->setVisible(false);
}

void MainWindow::updateModelSwitcherMenu(QMdiSubWindow *pActivatedWindow)
{
  Q_UNUSED(pActivatedWindow);
  ModelWidgetContainer *pModelWidgetContainer = qobject_cast<ModelWidgetContainer*>(sender());
  // get list of opened Model Widgets
  QList<QMdiSubWindow*> subWindowsList = mpModelWidgetContainer->subWindowList(QMdiArea::ActivationHistoryOrder);
  if (subWindowsList.isEmpty() && pModelWidgetContainer)
  {
    mpModelSwitcherToolButton->setEnabled(false);
    return;
  }
  int j = 0;
  for (int i = subWindowsList.size() - 1 ; i >= 0 ; i--)
  {
    if (j >= MaxRecentFiles)
      break;
    ModelWidget *pModelWidget = qobject_cast<ModelWidget*>(subWindowsList.at(i)->widget());
    if (pModelWidget)
    {
      mpModelSwitcherActions[j]->setText(pModelWidget->getLibraryTreeNode()->getNameStructure());
      mpModelSwitcherActions[j]->setData(pModelWidget->getLibraryTreeNode()->getNameStructure());
      mpModelSwitcherActions[j]->setVisible(true);
    }
    j++;
  }
  // if subwindowlist size is less than MaxRecentFiles then hide the remaining actions
  int numRecentModels = qMin(subWindowsList.size(), (int)MaxRecentFiles);
  for (j = numRecentModels ; j < MaxRecentFiles ; j++)
    mpModelSwitcherActions[j]->setVisible(false);
}

void MainWindow::toggleAutoSave()
{
  if (mpOptionsDialog->getGeneralSettingsPage()->getEnableAutoSaveGroupBox()->isChecked())
  {
    mpAutoSaveTimer->start(mpOptionsDialog->getGeneralSettingsPage()->getAutoSaveIntervalSpinBox()->value() * 1000);
  }
  else
  {
    mpAutoSaveTimer->stop();
  }
}

void MainWindow::perspectiveTabChanged(int tabIndex)
{
  switch (tabIndex)
  {
    case 0:
      switchToWelcomePerspective();
      break;
    case 1:
      switchToModelingPerspective();
      break;
    case 2:
      switchToPlottingPerspective();
      break;
    default:
      switchToWelcomePerspective();
      break;
  }
}

void MainWindow::autoSave()
{
  bool autoSaveForSingleClasses = mpOptionsDialog->getGeneralSettingsPage()->getEnableAutoSaveForSingleClassesCheckBox()->isChecked();
  bool autoSaveForOneFilePackages = mpOptionsDialog->getGeneralSettingsPage()->getEnableAutoSaveForOneFilePackagesCheckBox()->isChecked();
  bool autoSaveForFolderPackages = false;
  // if auto save for any class type is enabled.
  if (autoSaveForSingleClasses || autoSaveForOneFilePackages || autoSaveForFolderPackages)
  {
    foreach (LibraryTreeNode* pLibraryTreeNode, mpLibraryTreeWidget->getLibraryTreeNodesList())
    {
      if (!pLibraryTreeNode->isSaved() && !pLibraryTreeNode->getFileName().isEmpty())
      {
        // if auto save for single file class is enabled.
        if (pLibraryTreeNode->getParentName().isEmpty() && pLibraryTreeNode->childCount() == 0 && autoSaveForSingleClasses)
        {
          mpLibraryTreeWidget->saveLibraryTreeNode(pLibraryTreeNode);
        }
        // if auto save for one file package is enabled.
        else if (pLibraryTreeNode->getParentName().isEmpty() && pLibraryTreeNode->childCount() > 0 && autoSaveForOneFilePackages)
        {
          mpLibraryTreeWidget->saveLibraryTreeNode(pLibraryTreeNode);
        }
        // if auto save for folder package is enabled.
        else if (autoSaveForFolderPackages)
        {
          LibraryTreeNode *pParentLibraryTreeNode = mpLibraryTreeWidget->getLibraryTreeNode(StringHandler::getFirstWordBeforeDot(pLibraryTreeNode->getNameStructure()));
          if (pParentLibraryTreeNode)
          {
            QFileInfo fileInfo(pParentLibraryTreeNode->getFileName());
            if ((pParentLibraryTreeNode->getSaveContentsType() == LibraryTreeNode::SaveFolderStructure) || (fileInfo.fileName().compare("package.mo") == 0))
            {
              mpLibraryTreeWidget->saveLibraryTreeNode(pParentLibraryTreeNode);
            }
          }
        }
      }
    }
  }
}

//! Defines the actions used by the toolbars
void MainWindow::createActions()
{
  /* Menu Actions */
  // File Menu
  // create new Modelica class action
  mpNewModelicaClassAction = new QAction(QIcon(":/Resources/icons/new.svg"), Helper::newModelicaClass, this);
  mpNewModelicaClassAction->setStatusTip(Helper::createNewModelicaClass);
  mpNewModelicaClassAction->setShortcut(QKeySequence("Ctrl+n"));
  connect(mpNewModelicaClassAction, SIGNAL(triggered()), SLOT(createNewModelicaClass()));
  // create new TLM file action
  mpNewTLMFileAction = new QAction(QIcon(":/Resources/icons/new.svg"), Helper::newTLMFile, this);
  mpNewTLMFileAction->setStatusTip(Helper::createNewTLMFile);
  //mpNewTLMFileAction->setShortcut(QKeySequence("Ctrl+x"));
  connect(mpNewTLMFileAction, SIGNAL(triggered()), SLOT(createNewTLMFile()));
  // open Modelica file action
  mpOpenModelicaFileAction = new QAction(QIcon(":/Resources/icons/open.svg"), Helper::openModelicaFiles, this);
  mpOpenModelicaFileAction->setShortcut(QKeySequence("Ctrl+o"));
  mpOpenModelicaFileAction->setStatusTip(tr("Opens the Modelica file(s)"));
  connect(mpOpenModelicaFileAction, SIGNAL(triggered()), SLOT(openModelicaFile()));
  // open Modelica file with encoding action
  mpOpenModelicaFileWithEncodingAction = new QAction(Helper::openConvertModelicaFiles, this);
  mpOpenModelicaFileWithEncodingAction->setStatusTip(tr("Opens and converts the Modelica file(s) with encoding"));
  connect(mpOpenModelicaFileWithEncodingAction, SIGNAL(triggered()), SLOT(showOpenModelicaFileDialog()));
  // load Modelica library action
  mpLoadModelicaLibraryAction = new QAction(tr("Load Library"), this);
  mpLoadModelicaLibraryAction->setStatusTip(tr("Loads the Modelica library"));
  connect(mpLoadModelicaLibraryAction, SIGNAL(triggered()), SLOT(loadModelicaLibrary()));
  // open result file action
  mpOpenResultFileAction = new QAction(tr("Open Result File(s)"), this);
  mpOpenResultFileAction->setShortcut(QKeySequence("Ctrl+shift+o"));
  mpOpenResultFileAction->setStatusTip(tr("Opens the OpenModelica Result file"));
  connect(mpOpenResultFileAction, SIGNAL(triggered()), SLOT(showOpenResultFileDialog()));
  // open transformations file action
  mpOpenTransformationFileAction = new QAction(tr("Open Transformations File"), this);
  mpOpenTransformationFileAction->setStatusTip(tr("Opens the class transformations file"));
  connect(mpOpenTransformationFileAction, SIGNAL(triggered()), SLOT(showOpenTransformationFileDialog()));
  // save file action
  mpSaveAction = new QAction(QIcon(":/Resources/icons/save.svg"), tr("Save"), this);
  mpSaveAction->setShortcut(QKeySequence("Ctrl+s"));
  mpSaveAction->setStatusTip(tr("Save a file"));
  mpSaveAction->setEnabled(false);
  // save as file action
  mpSaveAsAction = new QAction(QIcon(":/Resources/icons/saveas.svg"), tr("Save As"), this);
  mpSaveAsAction->setStatusTip(tr("Save As a File"));
  mpSaveAsAction->setEnabled(false);
  // save all file action
  mpSaveAllAction = new QAction(QIcon(":/Resources/icons/saveall.svg"), tr("Save All"), this);
  mpSaveAllAction->setStatusTip(tr("Save All Files"));
  mpSaveAllAction->setEnabled(false);
  // Dump Total Model action
  mpSaveTotalModelAction = new QAction(tr("Save Total Model"), this);
  mpSaveTotalModelAction->setStatusTip(tr("Dumps the total model to a file"));
  mpSaveTotalModelAction->setEnabled(false);
  // recent files action
  for (int i = 0; i < MaxRecentFiles; ++i)
  {
    mpRecentFileActions[i] = new QAction(this);
    mpRecentFileActions[i]->setVisible(false);
    connect(mpRecentFileActions[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
  }
  // clear recent files action
  mpClearRecentFilesAction = new QAction(Helper::clearRecentFiles, this);
  mpClearRecentFilesAction->setStatusTip(tr("Clears the recent files list"));
  connect(mpClearRecentFilesAction, SIGNAL(triggered()), SLOT(clearRecentFilesList()));
  // print  action
  mpPrintModelAction = new QAction(QIcon(":/Resources/icons/print.svg"), tr("Print..."), this);
  mpPrintModelAction->setShortcut(QKeySequence("Ctrl+p"));
  mpPrintModelAction->setEnabled(false);
  // close OMEdit action
  mpQuitAction = new QAction(QIcon(":/Resources/icons/quit.svg"), tr("Quit"), this);
  mpQuitAction->setStatusTip(tr("Quit the ").append(Helper::applicationIntroText));
  mpQuitAction->setShortcut(QKeySequence("Ctrl+q"));
  connect(mpQuitAction, SIGNAL(triggered()), SLOT(close()));
  // Edit Menu
  // cut action
  mpCutAction = new QAction(QIcon(":/Resources/icons/cut.svg"), tr("Cut"), this);
  mpCutAction->setShortcut(QKeySequence("Ctrl+x"));
  // copy action
  mpCopyAction = new QAction(QIcon(":/Resources/icons/copy.svg"), Helper::copy, this);
  //! @todo opening this will stop copying data from messages window.
  //mpCopyAction->setShortcut(QKeySequence("Ctrl+c"));
  // paste action
  mpPasteAction = new QAction(QIcon(":/Resources/icons/paste.svg"), tr("Paste"), this);
  mpPasteAction->setShortcut(QKeySequence("Ctrl+v"));
  // find replace class action
  mpFindReplaceAction = new QAction(QString(Helper::findReplaceModelicaText), this);
  mpFindReplaceAction->setStatusTip(tr("Shows the Find/Replace window"));
  mpFindReplaceAction->setShortcut(QKeySequence("Ctrl+f"));
  mpFindReplaceAction->setEnabled(false);
  connect(mpFindReplaceAction, SIGNAL(triggered()), SLOT(showFindReplaceDialog()));
  // clear find/replace texts action
  mpClearFindReplaceTextsAction = new QAction(tr("Clear Find/Replace Texts"), this);
  mpClearFindReplaceTextsAction->setStatusTip(tr("Clears the Find/Replace text items"));
  connect(mpClearFindReplaceTextsAction, SIGNAL(triggered()), SLOT(clearFindReplaceTexts()));
  // goto line action
  mpGotoLineNumberAction = new QAction(tr("Go to Line"), this);
  mpGotoLineNumberAction->setStatusTip(tr("Shows the Go to Line Number window"));
  mpGotoLineNumberAction->setShortcut(QKeySequence("Ctrl+l"));
  mpGotoLineNumberAction->setEnabled(false);
  connect(mpGotoLineNumberAction, SIGNAL(triggered()), SLOT(showGotoLineNumberDialog()));
  // View Menu
  // show/hide gridlines action
  mpShowGridLinesAction = new QAction(QIcon(":/Resources/icons/grid.svg"), tr("Grid Lines"), this);
  mpShowGridLinesAction->setStatusTip(tr("Show/Hide the grid lines"));
  mpShowGridLinesAction->setCheckable(true);
  mpShowGridLinesAction->setChecked(true);
  mpShowGridLinesAction->setEnabled(false);
  connect(mpShowGridLinesAction, SIGNAL(toggled(bool)), SLOT(setShowGridLines(bool)));
  // reset zoom action
  mpResetZoomAction = new QAction(QIcon(":/Resources/icons/zoomReset.svg"), tr("Reset Zoom"), this);
  mpResetZoomAction->setStatusTip(tr("Resets the zoom"));
  mpResetZoomAction->setShortcut(QKeySequence("Ctrl+0"));
  mpResetZoomAction->setEnabled(false);
  connect(mpResetZoomAction, SIGNAL(triggered()), SLOT(resetZoom()));
  // zoom in action
  mpZoomInAction = new QAction(QIcon(":/Resources/icons/zoomIn.svg"), tr("Zoom In"), this);
  mpZoomInAction->setStatusTip(tr("Zoom in"));
  mpZoomInAction->setShortcut(QKeySequence("Ctrl++"));
  mpZoomInAction->setEnabled(false);
  connect(mpZoomInAction, SIGNAL(triggered()), SLOT(zoomIn()));
  // zoom out action
  mpZoomOutAction = new QAction(QIcon(":/Resources/icons/zoomOut.svg"), tr("Zoom Out"), this);
  mpZoomOutAction->setStatusTip(tr("Zoom out"));
  mpZoomOutAction->setShortcut(QKeySequence("Ctrl+-"));
  mpZoomOutAction->setEnabled(false);
  connect(mpZoomOutAction, SIGNAL(triggered()), SLOT(zoomOut()));
  // Algorithmic Debugger action
  mpShowAlgorithmicDebuggerAction = new QAction(Helper::algorithmicDebugger, this);
  mpShowAlgorithmicDebuggerAction->setShortcut(QKeySequence("ctrl+t"));
  mpShowAlgorithmicDebuggerAction->setStatusTip(Helper::algorithmicDebugger);
  connect(mpShowAlgorithmicDebuggerAction, SIGNAL(triggered()), SLOT(showAlgorithmicDebugger()));
  // Cascade windows action
  mpCascadeWindowsAction = new QAction(tr("Cascade Windows"), this);
  mpCascadeWindowsAction->setStatusTip(tr("Arranges all the child windows in a cascade pattern"));
  connect(mpCascadeWindowsAction, SIGNAL(triggered()), SLOT(cascadeSubWindows()));
  // Tile windows Horizontally action
  mpTileWindowsHorizontallyAction = new QAction(tr("Tile Windows Horizontally"), this);
  mpTileWindowsHorizontallyAction->setStatusTip(tr("Arranges all child windows in a horizontally tiled pattern"));
  connect(mpTileWindowsHorizontallyAction, SIGNAL(triggered()), SLOT(tileSubWindowsHorizontally()));
  // Tile windows Vertically action
  mpTileWindowsVerticallyAction = new QAction(tr("Tile Windows Vertically"), this);
  mpTileWindowsVerticallyAction->setStatusTip(tr("Arranges all child windows in a vertically tiled pattern"));
  connect(mpTileWindowsVerticallyAction, SIGNAL(triggered()), SLOT(tileSubWindowsVertically()));
  // Simulation Menu
  // instantiate model action
  mpInstantiateModelAction = new QAction(QIcon(":/Resources/icons/flatmodel.svg"), tr("Instantiate Model"), this);
  mpInstantiateModelAction->setStatusTip(tr("Instantiates the modelica model"));
  mpInstantiateModelAction->setEnabled(false);
  connect(mpInstantiateModelAction, SIGNAL(triggered()), SLOT(instantiatesModel()));
  // check model action
  mpCheckModelAction = new QAction(QIcon(":/Resources/icons/check.svg"), Helper::checkModel, this);
  mpCheckModelAction->setStatusTip(Helper::checkModelTip);
  mpCheckModelAction->setEnabled(false);
  connect(mpCheckModelAction, SIGNAL(triggered()), SLOT(checkModel()));
  // check all models action
  mpCheckAllModelsAction = new QAction(QIcon(":/Resources/icons/check-all.svg"), Helper::checkAllModels, this);
  mpCheckAllModelsAction->setStatusTip(Helper::checkAllModelsTip);
  mpCheckAllModelsAction->setEnabled(false);
  connect(mpCheckAllModelsAction, SIGNAL(triggered()), SLOT(checkAllModels()));
  // simulate action
  mpSimulateModelAction = new QAction(QIcon(":/Resources/icons/simulate.svg"), Helper::simulate, this);
  mpSimulateModelAction->setStatusTip(Helper::simulateTip);
  mpSimulateModelAction->setShortcut(QKeySequence("Ctrl+b"));
  mpSimulateModelAction->setEnabled(false);
  connect(mpSimulateModelAction, SIGNAL(triggered()), SLOT(simulateModel()));
  // simulate with transformational debugger action
  mpSimulateWithTransformationalDebuggerAction = new QAction(QIcon(":/Resources/icons/simulate-equation.svg"), Helper::simulateWithTransformationalDebugger, this);
  mpSimulateWithTransformationalDebuggerAction->setStatusTip(Helper::simulateWithTransformationalDebuggerTip);
  mpSimulateWithTransformationalDebuggerAction->setEnabled(false);
  connect(mpSimulateWithTransformationalDebuggerAction, SIGNAL(triggered()), SLOT(simulateModelWithTransformationalDebugger()));
  // simulate with algorithmic debugger action
  mpSimulateWithAlgorithmicDebuggerAction = new QAction(QIcon(":/Resources/icons/simulate-debug.svg"), Helper::simulateWithAlgorithmicDebugger, this);
  mpSimulateWithAlgorithmicDebuggerAction->setStatusTip(Helper::simulateWithAlgorithmicDebuggerTip);
  mpSimulateWithAlgorithmicDebuggerAction->setEnabled(false);
  connect(mpSimulateWithAlgorithmicDebuggerAction, SIGNAL(triggered()), SLOT(simulateModelWithAlgorithmicDebugger()));
  // simulation setup action
  mpSimulationSetupAction = new QAction(QIcon(":/Resources/icons/simulation-center.svg"), Helper::simulationSetup, this);
  mpSimulationSetupAction->setStatusTip(Helper::simulationSetupTip);
  mpSimulationSetupAction->setEnabled(false);
  connect(mpSimulationSetupAction, SIGNAL(triggered()), SLOT(openSimulationDialog()));
  // FMI Menu
  // export FMU action
  mpExportFMUAction = new QAction(QIcon(":/Resources/icons/export-fmu.svg"), Helper::exportFMU, this);
  mpExportFMUAction->setStatusTip(Helper::exportFMUTip);
  mpExportFMUAction->setEnabled(false);
  connect(mpExportFMUAction, SIGNAL(triggered()), SLOT(exportModelFMU()));
  // import FMU action
  mpImportFMUAction = new QAction(QIcon(":/Resources/icons/import-fmu.svg"), Helper::importFMU, this);
  mpImportFMUAction->setStatusTip(Helper::importFMUTip);
  connect(mpImportFMUAction, SIGNAL(triggered()), SLOT(importModelFMU()));
  // XML Menu
  // export XML action
  mpExportXMLAction = new QAction(QIcon(":/Resources/icons/export-xml.svg"), Helper::exportXML, this);
  mpExportXMLAction->setStatusTip(Helper::exportXMLTip);
  mpExportXMLAction->setEnabled(false);
  connect(mpExportXMLAction, SIGNAL(triggered()), SLOT(exportModelXML()));
  // export XML action
  mpExportFigaroAction = new QAction(QIcon(":/Resources/icons/console.svg"), Helper::exportFigaro, this);
  mpExportFigaroAction->setStatusTip(Helper::exportFigaroTip);
  mpExportFigaroAction->setEnabled(false);
  connect(mpExportFigaroAction, SIGNAL(triggered()), SLOT(exportModelFigaro()));
  // Tools Menu
  // show OMC Logger widget action
  mpShowOMCLoggerWidgetAction = new QAction(QIcon(":/Resources/icons/console.svg"), tr("OMC Logger"), this);
  mpShowOMCLoggerWidgetAction->setStatusTip(tr("Shows OMC Logger Window"));
  connect(mpShowOMCLoggerWidgetAction, SIGNAL(triggered()), mpOMCProxy, SLOT(openOMCLoggerWidget()));
  // export to OMNotebook action
  mpExportToOMNotebookAction = new QAction(QIcon(":/Resources/icons/export-omnotebook.svg"), Helper::exportToOMNotebook, this);
  mpExportToOMNotebookAction->setStatusTip(Helper::exportToOMNotebookTip);
  mpExportToOMNotebookAction->setEnabled(false);
  connect(mpExportToOMNotebookAction, SIGNAL(triggered()), SLOT(exportModelToOMNotebook()));
  // import from OMNotebook action
  mpImportFromOMNotebookAction = new QAction(QIcon(":/Resources/icons/import-omnotebook.svg"), Helper::importFromOMNotebook, this);
  mpImportFromOMNotebookAction->setStatusTip(Helper::importFromOMNotebookTip);
  connect(mpImportFromOMNotebookAction, SIGNAL(triggered()), SLOT(importModelfromOMNotebook()));
    // import ngspice netlist action
  mpImportNgspiceNetlistAction = new QAction(Helper::importNgspiceNetlist, this);
  mpImportNgspiceNetlistAction->setStatusTip(Helper::importNgspiceNetlistTip);
  connect(mpImportNgspiceNetlistAction, SIGNAL(triggered()), SLOT(importNgspiceNetlist()));
  // open options action
  mpOptionsAction = new QAction(QIcon(":/Resources/icons/options.svg"), tr("Options"), this);
  mpOptionsAction->setStatusTip(tr("Shows the options window"));
  mpOptionsAction->setMenuRole(QAction::PreferencesRole);
  connect(mpOptionsAction, SIGNAL(triggered()), SLOT(openConfigurationOptions()));
  // Help Menu
  // users guide action
  mpUsersGuideAction = new QAction(tr("OpenModelica Users Guide"), this);
  mpUsersGuideAction->setStatusTip(tr("Opens the OpenModelica Users Guide"));
  mpUsersGuideAction->setShortcut(QKeySequence(Qt::Key_F1));
  connect(mpUsersGuideAction, SIGNAL(triggered()), SLOT(openUsersGuide()));
  // system documentation action
  mpSystemDocumentationAction = new QAction(tr("OpenModelica System Documentation"), this);
  mpSystemDocumentationAction->setStatusTip(tr("Opens the OpenModelica System Documentation"));
  connect(mpSystemDocumentationAction, SIGNAL(triggered()), SLOT(openSystemDocumentation()));
  // OpenModelica Scripting documentation action
  mpOpenModelicaScriptingAction = new QAction(tr("OpenModelica Scripting Documentation"), this);
  mpOpenModelicaScriptingAction->setStatusTip(tr("Opens the OpenModelica Scripting Documentation"));
  connect(mpOpenModelicaScriptingAction, SIGNAL(triggered()), SLOT(openOpenModelicaScriptingDocumentation()));
  // Modelica documentation action
  mpModelicaDocumentationAction = new QAction(tr("Modelica Documentation"), this);
  mpModelicaDocumentationAction->setStatusTip(tr("Opens the Modelica Documentation"));
  connect(mpModelicaDocumentationAction, SIGNAL(triggered()), SLOT(openModelicaDocumentation()));
  // Modelica By Example action
  mpModelicaByExampleAction = new QAction(tr("Modelica By Example"), this);
  mpModelicaByExampleAction->setStatusTip(tr("Opens the Modelica By Example online book"));
  connect(mpModelicaByExampleAction, SIGNAL(triggered()), SLOT(openModelicaByExample()));
  // Modelica Web Reference action
  mpModelicaWebReferenceAction = new QAction(tr("Modelica Web Reference"), this);
  mpModelicaWebReferenceAction->setStatusTip(tr("Opens the Modelica Web Reference"));
  connect(mpModelicaWebReferenceAction, SIGNAL(triggered()), SLOT(openModelicaWebReference()));
  // about OMEdit action
  mpAboutOMEditAction = new QAction(tr("About OMEdit"), this);
  mpAboutOMEditAction->setStatusTip(tr("Information about OMEdit"));
  connect(mpAboutOMEditAction, SIGNAL(triggered()), SLOT(openAboutOMEdit()));
  /* Toolbar Actions */
  // custom shapes group
  mpShapesActionGroup = new QActionGroup(this);
  mpShapesActionGroup->setExclusive(false);
  // line creation action
  mpLineShapeAction = new QAction(QIcon(":/Resources/icons/line-shape.svg"), Helper::line, mpShapesActionGroup);
  mpLineShapeAction->setStatusTip(tr("Draws a line shape"));
  mpLineShapeAction->setCheckable(true);
  connect(mpLineShapeAction, SIGNAL(triggered()), SLOT(toggleShapesButton()));
  // polygon creation action
  mpPolygonShapeAction = new QAction(QIcon(":/Resources/icons/polygon-shape.svg"), tr("Polygon"), mpShapesActionGroup);
  mpPolygonShapeAction->setStatusTip(tr("Draws a polygon shape"));
  mpPolygonShapeAction->setCheckable(true);
  connect(mpPolygonShapeAction, SIGNAL(triggered()), SLOT(toggleShapesButton()));
  // rectangle creation action
  mpRectangleShapeAction = new QAction(QIcon(":/Resources/icons/rectangle-shape.svg"), tr("Rectangle"), mpShapesActionGroup);
  mpRectangleShapeAction->setStatusTip(tr("Draws a rectangle shape"));
  mpRectangleShapeAction->setCheckable(true);
  connect(mpRectangleShapeAction, SIGNAL(triggered()), SLOT(toggleShapesButton()));
  // ellipse creation action
  mpEllipseShapeAction = new QAction(QIcon(":/Resources/icons/ellipse-shape.svg"), tr("Ellipse"), mpShapesActionGroup);
  mpEllipseShapeAction->setStatusTip(tr("Draws an ellipse shape"));
  mpEllipseShapeAction->setCheckable(true);
  connect(mpEllipseShapeAction, SIGNAL(triggered()), SLOT(toggleShapesButton()));
  // text creation action
  mpTextShapeAction = new QAction(QIcon(":/Resources/icons/text-shape.svg"), tr("Text"), mpShapesActionGroup);
  mpTextShapeAction->setStatusTip(tr("Draws a text shape"));
  mpTextShapeAction->setCheckable(true);
  connect(mpTextShapeAction, SIGNAL(triggered()), SLOT(toggleShapesButton()));
  // bitmap creation action
  mpBitmapShapeAction = new QAction(QIcon(":/Resources/icons/bitmap-shape.svg"), tr("Bitmap"), mpShapesActionGroup);
  mpBitmapShapeAction->setStatusTip(tr("Inserts a bitmap"));
  mpBitmapShapeAction->setCheckable(true);
  connect(mpBitmapShapeAction, SIGNAL(triggered()), SLOT(toggleShapesButton()));
  // connect/unconnect action
  mpConnectModeAction = new QAction(QIcon(":/Resources/icons/connect-mode.svg"), tr("Connect/Unconnect Mode"), mpShapesActionGroup);
  mpConnectModeAction->setStatusTip(tr("Changes to/from connect mode"));
  mpConnectModeAction->setCheckable(true);
  mpConnectModeAction->setChecked(true);
  connect(mpConnectModeAction, SIGNAL(triggered()), SLOT(toggleShapesButton()));
  // model switcher actions
  for (int i = 0; i < MaxRecentFiles; ++i)
  {
    mpModelSwitcherActions[i] = new QAction(this);
    mpModelSwitcherActions[i]->setVisible(false);
    connect(mpModelSwitcherActions[i], SIGNAL(triggered()), this, SLOT(openRecentModelWidget()));
  }
  // resimulate action
  mpReSimulateModelAction = new QAction(QIcon(":/Resources/icons/re-simulate.svg"), Helper::reSimulate, this);
  mpReSimulateModelAction->setStatusTip(Helper::reSimulateTip);
  connect(mpReSimulateModelAction, SIGNAL(triggered()), SLOT(reSimulateModel()));
  // resimulate setup action
  mpReSimulateSetupAction = new QAction(QIcon(":/Resources/icons/re-simulation-center.svg"), Helper::reSimulateSetup, this);
  mpReSimulateSetupAction->setStatusTip(Helper::reSimulateSetupTip);
  connect(mpReSimulateSetupAction, SIGNAL(triggered()), SLOT(showReSimulateSetup()));
  // new plot window action
  mpNewPlotWindowAction = new QAction(QIcon(":/Resources/icons/plot-window.svg"), tr("New Plot Window"), this);
  mpNewPlotWindowAction->setStatusTip(tr("Inserts new plot window"));
  connect(mpNewPlotWindowAction, SIGNAL(triggered()), SLOT(addNewPlotWindow()));
  // new parametric plot action
  mpNewParametricPlotWindowAction = new QAction(QIcon(":/Resources/icons/parametric-plot-window.svg"), tr("New Parametric Plot Window"), this);
  mpNewParametricPlotWindowAction->setStatusTip(tr("Inserts new parametric plot window"));
  connect(mpNewParametricPlotWindowAction, SIGNAL(triggered()), SLOT(addNewParametricPlotWindow()));
  // clear plot window action
  mpClearPlotWindowAction = new QAction(QIcon(":/Resources/icons/clear.svg"), tr("Clear Plot Window"), this);
  mpClearPlotWindowAction->setStatusTip(tr("Clears all the curves from the plot window"));
  connect(mpClearPlotWindowAction, SIGNAL(triggered()), SLOT(clearPlotWindow()));
  // Other Actions
  // export as image action
  mpExportAsImageAction = new QAction(QIcon(":/Resources/icons/bitmap-shape.svg"), Helper::exportAsImage, this);
  mpExportAsImageAction->setStatusTip(Helper::exportAsImageTip);
  mpExportAsImageAction->setEnabled(false);
  connect(mpExportAsImageAction, SIGNAL(triggered()), SLOT(exportModelAsImage()));
  mpExportToClipboardAction = new QAction(tr("Export to Clipboard"), this);
  mpExportToClipboardAction->setStatusTip(Helper::exportAsImageTip);
  mpExportToClipboardAction->setEnabled(false);
  connect(mpExportToClipboardAction, SIGNAL(triggered()), SLOT(exportToClipboard()));
}

//! Creates the menus
void MainWindow::createMenus()
{
  //Create the menubar
  //Create the menus
  // File menu
  QMenu *pFileMenu = new QMenu(menuBar());
  pFileMenu->setObjectName("menuFile");
  pFileMenu->setTitle(tr("&File"));
  // add actions to File menu
  pFileMenu->addAction(mpNewModelicaClassAction);
  //pFileMenu->addAction(mpNewTLMFileAction);
  pFileMenu->addAction(mpOpenModelicaFileAction);
  pFileMenu->addAction(mpOpenModelicaFileWithEncodingAction);
  pFileMenu->addAction(mpLoadModelicaLibraryAction);
  pFileMenu->addAction(mpOpenResultFileAction);
  pFileMenu->addAction(mpOpenTransformationFileAction);
  pFileMenu->addAction(mpSaveAction);
  pFileMenu->addAction(mpSaveAsAction);
  //menuFile->addAction(saveAllAction);
  pFileMenu->addAction(mpSaveTotalModelAction);
  pFileMenu->addSeparator();
  mpLibrariesMenu = new QMenu(menuBar());
  mpLibrariesMenu->setObjectName("LibrariesMenu");
  mpLibrariesMenu->setTitle(tr("&System Libraries"));
  // get the available libraries.
  QStringList libraries = mpOMCProxy->getAvailableLibraries();
  for (int i = 0; i < libraries.size(); ++i)
  {
    QAction *pAction = new QAction(libraries[i], this);
    connect(pAction, SIGNAL(triggered()), SLOT(loadSystemLibrary()));
    mpLibrariesMenu->addAction(pAction);
  }
  pFileMenu->addMenu(mpLibrariesMenu);
  pFileMenu->addSeparator();
  mpRecentFilesMenu = new QMenu(menuBar());
  mpRecentFilesMenu->setObjectName("RecentFilesMenu");
  mpRecentFilesMenu->setTitle(tr("Recent &Files"));
  for (int i = 0; i < MaxRecentFiles; ++i)
    mpRecentFilesMenu->addAction(mpRecentFileActions[i]);
  pFileMenu->addMenu(mpRecentFilesMenu);
  pFileMenu->addAction(mpClearRecentFilesAction);
  pFileMenu->addSeparator();
  pFileMenu->addAction(mpPrintModelAction);
  pFileMenu->addSeparator();
  pFileMenu->addAction(mpQuitAction);
  // add File menu to menu bar
  menuBar()->addAction(pFileMenu->menuAction());
  // Edit menu
  QMenu *pEditMenu = new QMenu(menuBar());
  pEditMenu->setTitle(tr("&Edit"));
  // add actions to Edit menu
  pEditMenu->addAction(mpCutAction);
  pEditMenu->addAction(mpCopyAction);
  pEditMenu->addAction(mpPasteAction);
  pEditMenu->addSeparator();
  pEditMenu->addAction(mpFindReplaceAction);
  pEditMenu->addAction(mpClearFindReplaceTextsAction);
  pEditMenu->addSeparator();
  QAction *pSearchClassAction = mpSearchClassDockWidget->toggleViewAction();
  pSearchClassAction->setShortcut(QKeySequence("Ctrl+Shift+f"));
  pEditMenu->addAction(pSearchClassAction);
  pEditMenu->addSeparator();
  pEditMenu->addAction(mpGotoLineNumberAction);
  // add Edit menu to menu bar
  menuBar()->addAction(pEditMenu->menuAction());
  // View menu
  QMenu *pViewMenu = new QMenu(menuBar());
  pViewMenu->setTitle(tr("&View"));
  // Toolbars View Menu
  QMenu *pViewToolbarsMenu = new QMenu(menuBar());
  pViewToolbarsMenu->setObjectName("ToolbarsViewMenu");
  pViewToolbarsMenu->setTitle(tr("Toolbars"));
  // Windows View Menu
  QMenu *pViewWindowsMenu = new QMenu(menuBar());
  pViewWindowsMenu->setObjectName("WindowsViewMenu");
  pViewWindowsMenu->setTitle(tr("Windows"));
  // add actions to View menu
  // Add Actions to Toolbars View Sub Menu
  pViewToolbarsMenu->addAction(mpFileToolBar->toggleViewAction());
  pViewToolbarsMenu->addAction(mpEditToolBar->toggleViewAction());
  pViewToolbarsMenu->addAction(mpViewToolBar->toggleViewAction());
  pViewToolbarsMenu->addAction(mpShapesToolBar->toggleViewAction());
  pViewToolbarsMenu->addAction(mpSimulationToolBar->toggleViewAction());
  pViewToolbarsMenu->addAction(mpPlotToolBar->toggleViewAction());
  pViewToolbarsMenu->addAction(mpModelSwitcherToolBar->toggleViewAction());
  // Add Actions to Windows View Sub Menu
  pViewWindowsMenu->addAction(pSearchClassAction);
  pViewWindowsMenu->addAction(mpLibraryTreeDockWidget->toggleViewAction());
  pViewWindowsMenu->addAction(mpDocumentationDockWidget->toggleViewAction());
  //  pViewWindowsMenu->addAction(mpSimulationDockWidget->toggleViewAction());
  pViewWindowsMenu->addAction(mpVariablesDockWidget->toggleViewAction());
  pViewWindowsMenu->addAction(mpMessagesDockWidget->toggleViewAction());
  pViewWindowsMenu->addAction(mpShowAlgorithmicDebuggerAction);
  pViewWindowsMenu->addSeparator();
  pViewWindowsMenu->addAction(mpCascadeWindowsAction);
  pViewWindowsMenu->addAction(mpTileWindowsHorizontallyAction);
  pViewWindowsMenu->addAction(mpTileWindowsVerticallyAction);
  pViewMenu->addAction(pViewToolbarsMenu->menuAction());
  pViewMenu->addAction(pViewWindowsMenu->menuAction());
  pViewMenu->addSeparator();
  pViewMenu->addAction(mpShowGridLinesAction);
  pViewMenu->addAction(mpResetZoomAction);
  pViewMenu->addAction(mpZoomInAction);
  pViewMenu->addAction(mpZoomOutAction);
  // add View menu to menu bar
  menuBar()->addAction(pViewMenu->menuAction());
  // Simulation Menu
  QMenu *pSimulationMenu = new QMenu(menuBar());
  pSimulationMenu->setTitle(tr("&Simulation"));
  // add actions to Simulation menu
  pSimulationMenu->addAction(mpInstantiateModelAction);
  pSimulationMenu->addAction(mpCheckModelAction);
  pSimulationMenu->addAction(mpCheckAllModelsAction);
  pSimulationMenu->addAction(mpSimulateModelAction);
  pSimulationMenu->addAction(mpSimulateWithTransformationalDebuggerAction);
  pSimulationMenu->addAction(mpSimulateWithAlgorithmicDebuggerAction);
  pSimulationMenu->addAction(mpSimulationSetupAction);
  // add Simulation menu to menu bar
  menuBar()->addAction(pSimulationMenu->menuAction());
  // FMI menu
  QMenu *pFMIMenu = new QMenu(menuBar());
  pFMIMenu->setTitle(tr("F&MI"));
  // add actions to FMI menu
  pFMIMenu->addAction(mpExportFMUAction);
  pFMIMenu->addAction(mpImportFMUAction);
  // add FMI menu to menu bar
  menuBar()->addAction(pFMIMenu->menuAction());
  // Export menu
  QMenu *pExportMenu = new QMenu(menuBar());
  pExportMenu->setTitle(tr("&Export"));
  // add actions to Export menu
  pExportMenu->addAction(mpExportXMLAction);
  pExportMenu->addAction(mpExportFigaroAction);
  // add Export menu to menu bar
  menuBar()->addAction(pExportMenu->menuAction());
  // Tools menu
  QMenu *pToolsMenu = new QMenu(menuBar());
  pToolsMenu->setTitle(tr("&Tools"));
  // add actions to Tools menu
  pToolsMenu->addAction(mpShowOMCLoggerWidgetAction);
  pToolsMenu->addSeparator();
  pToolsMenu->addAction(mpExportToOMNotebookAction);
  pToolsMenu->addAction(mpImportFromOMNotebookAction);
  pToolsMenu->addSeparator();
  pToolsMenu->addAction(mpImportNgspiceNetlistAction);
  pToolsMenu->addSeparator();
  pToolsMenu->addAction(mpOptionsAction);
  // add Tools menu to menu bar
  menuBar()->addAction(pToolsMenu->menuAction());
  // Help menu
  QMenu *pHelpMenu = new QMenu(menuBar());
  pHelpMenu->setTitle(tr("&Help"));
  // add actions to Help menu
  pHelpMenu->addAction(mpUsersGuideAction);
  pHelpMenu->addAction(mpSystemDocumentationAction);
  pHelpMenu->addAction(mpOpenModelicaScriptingAction);
  pHelpMenu->addAction(mpModelicaDocumentationAction);
  pHelpMenu->addSeparator();
  //  pHelpMenu->addAction(mpModelicaByExampleAction);
  //  pHelpMenu->addAction(mpModelicaWebReferenceAction);
  //  pHelpMenu->addSeparator();
  pHelpMenu->addAction(mpAboutOMEditAction);
  // add Help menu to menu bar
  menuBar()->addAction(pHelpMenu->menuAction());
}

/*!
  Stores the window states and geometry of all Plot Windows.
  */
/*
  The application window title and window icon gets corrupted when we switch between modeling & plotting perspective.
  To solve this we tile the plot windows when we switch to modeling and welcome perspective. But before calling tileSubWindows() we save all
  the plot windows states & geometry and then restore it when switching back to plotting view.
  */
void MainWindow::storePlotWindowsStateAndGeometry()
{
  if (mPlotWindowsStatesList.isEmpty() && mPlotWindowsGeometriesList.isEmpty()) {
    foreach (QMdiSubWindow *pWindow, mpPlotWindowContainer->subWindowList()) {
      mPlotWindowsStatesList.append(pWindow->windowState());
      mPlotWindowsGeometriesList.append(pWindow->saveGeometry());
    }
  }
}

void MainWindow::switchToWelcomePerspective()
{
  storePlotWindowsStateAndGeometry();
  mpPlotWindowContainer->tileSubWindows();
  mpCentralStackedWidget->setCurrentWidget(mpWelcomePageWidget);
  mpModelWidgetContainer->currentModelWidgetChanged(0);
  mpModelSwitcherToolButton->setEnabled(false);
  mpVariablesDockWidget->hide();
  mpPlotToolBar->setEnabled(false);
}

void MainWindow::switchToModelingPerspective()
{
  storePlotWindowsStateAndGeometry();
  mpPlotWindowContainer->tileSubWindows();
  mpCentralStackedWidget->setCurrentWidget(mpModelWidgetContainer);
  mpModelWidgetContainer->currentModelWidgetChanged(mpModelWidgetContainer->getCurrentMdiSubWindow());
  mpModelSwitcherToolButton->setEnabled(true);
  mpVariablesDockWidget->hide();
  mpPlotToolBar->setEnabled(false);
}

void MainWindow::switchToPlottingPerspective()
{
  mpCentralStackedWidget->setCurrentWidget(mpPlotWindowContainer);
  int i = 0;
  foreach (QMdiSubWindow *pWindow, mpPlotWindowContainer->subWindowList()) {
    // sanity check
    if (mPlotWindowsStatesList.size() > i && mPlotWindowsGeometriesList.size() > i) {
      pWindow->setWindowState(pWindow->windowState() & (mPlotWindowsStatesList[i]));
      pWindow->restoreGeometry(mPlotWindowsGeometriesList[i]);
    }
    i++;
  }
  mPlotWindowsStatesList.clear();
  mPlotWindowsGeometriesList.clear();
  mpModelWidgetContainer->currentModelWidgetChanged(0);
  mpModelSwitcherToolButton->setEnabled(false);
  // if not plotwindow is opened then open one for user
  if (mpPlotWindowContainer->subWindowList().size() == 0) {
    mpPlotWindowContainer->addPlotWindow(true);
  }
  mpVariablesDockWidget->show();
  mpPlotToolBar->setEnabled(true);
}

/*!
  Arranges all child windows in a horizontally tiled pattern.
  \param pMdiArea - the subwindows parent mdi area.
  */
void MainWindow::tileSubWindows(QMdiArea *pMdiArea, bool horizontally)
{
  QList<QMdiSubWindow*> subWindowsList = pMdiArea->subWindowList(QMdiArea::ActivationHistoryOrder);
  if (subWindowsList.count() < 2) {
    pMdiArea->tileSubWindows();
    return;
  }
  QPoint position(0, 0);
  for (int i = subWindowsList.size() - 1 ; i >= 0 ; i--) {
    QMdiSubWindow *pSubWindow = subWindowsList[i];
    if (!pSubWindow->isVisible() || (pSubWindow->isMinimized() && !pSubWindow->isShaded())) {
      continue;
    }
    if (pSubWindow->isMaximized() || pSubWindow->isShaded()) {
      pSubWindow->showNormal();
    }
    QRect rect;
    if (horizontally) {
      rect = QRect(0, 0, pMdiArea->width(), qMax(pSubWindow->minimumSizeHint().height(), pMdiArea->height() / subWindowsList.count()));
    } else {
      rect = QRect(0, 0, qMax(pSubWindow->minimumSizeHint().width(), pMdiArea->width() / subWindowsList.count()), pMdiArea->height());
    }
    pSubWindow->setGeometry(rect);
    pSubWindow->move(position);
    if (horizontally) {
      position.setY(position.y() + pSubWindow->height());
    } else {
      position.setX(position.x() + pSubWindow->width());
    }
  }
}

//! Creates the toolbars
void MainWindow::createToolbars()
{
  int toolbarIconSize = mpOptionsDialog->getGeneralSettingsPage()->getToolbarIconSizeSpinBox()->value();
  setIconSize(QSize(toolbarIconSize, toolbarIconSize));
  // File Toolbar
  mpFileToolBar = addToolBar(tr("File Toolbar"));
  mpFileToolBar->setObjectName("File Toolbar");
  mpFileToolBar->setAllowedAreas(Qt::TopToolBarArea);
  // add actions to File Toolbar
  mpFileToolBar->addAction(mpNewModelicaClassAction);
  mpFileToolBar->addAction(mpOpenModelicaFileAction);
  mpFileToolBar->addAction(mpSaveAction);
  mpFileToolBar->addAction(mpSaveAsAction);
  //mpFileToolBar->addAction(mpSaveAllAction);
  // Edit Toolbar
  mpEditToolBar = addToolBar(tr("Edit Toolbar"));
  mpEditToolBar->setObjectName("Edit Toolbar");
  mpEditToolBar->setAllowedAreas(Qt::TopToolBarArea);
  // add actions to edit toolbar
  mpEditToolBar->addAction(mpCutAction);
  mpEditToolBar->addAction(mpCopyAction);
  mpEditToolBar->addAction(mpPasteAction);
  // View Toolbar
  mpViewToolBar = addToolBar(tr("View Toolbar"));
  mpViewToolBar->setObjectName("View Toolbar");
  mpViewToolBar->setAllowedAreas(Qt::TopToolBarArea);
  // add actions to View Toolbar
  mpViewToolBar->addAction(mpShowGridLinesAction);
  mpViewToolBar->addSeparator();
  mpViewToolBar->addAction(mpResetZoomAction);
  mpViewToolBar->addAction(mpZoomInAction);
  mpViewToolBar->addAction(mpZoomOutAction);
  // Shapes Toolbar
  mpShapesToolBar = addToolBar(tr("Shapes Toolbar"));
  mpShapesToolBar->setObjectName("Shapes Toolbar");
  mpShapesToolBar->setAllowedAreas(Qt::TopToolBarArea);
  // add actions to Shapes Toolbar
  mpShapesToolBar->addAction(mpLineShapeAction);
  mpShapesToolBar->addAction(mpPolygonShapeAction);
  mpShapesToolBar->addAction(mpRectangleShapeAction);
  mpShapesToolBar->addAction(mpEllipseShapeAction);
  mpShapesToolBar->addAction(mpTextShapeAction);
  mpShapesToolBar->addAction(mpBitmapShapeAction);
  mpShapesToolBar->addSeparator();
  mpShapesToolBar->addAction(mpConnectModeAction);
  // Simulation Toolbar
  mpSimulationToolBar = addToolBar(tr("Simulation Toolbar"));
  mpSimulationToolBar->setObjectName("Simulation Toolbar");
  mpSimulationToolBar->setAllowedAreas(Qt::TopToolBarArea);
  // add actions to Simulation Toolbar
  mpSimulationToolBar->addAction(mpInstantiateModelAction);
  mpSimulationToolBar->addAction(mpCheckModelAction);
  mpSimulationToolBar->addAction(mpCheckAllModelsAction);
  mpSimulationToolBar->addAction(mpSimulateModelAction);
  mpSimulationToolBar->addAction(mpSimulateWithTransformationalDebuggerAction);
  mpSimulationToolBar->addAction(mpSimulateWithAlgorithmicDebuggerAction);
  mpSimulationToolBar->addAction(mpSimulationSetupAction);
  // Model Swithcer Toolbar
  mpModelSwitcherToolBar = addToolBar(tr("ModelSwitcher Toolbar"));
  mpModelSwitcherToolBar->setObjectName("ModelSwitcher Toolbar");
  mpModelSwitcherToolBar->setAllowedAreas(Qt::TopToolBarArea);
  // Model Switcher Menu
  mpModelSwitcherMenu = new QMenu;
  for (int i = 0; i < MaxRecentFiles; ++i)
  {
    mpModelSwitcherMenu->addAction(mpModelSwitcherActions[i]);
  }
  // Model Switcher ToolButton
  mpModelSwitcherToolButton = new QToolButton;
  mpModelSwitcherToolButton->setEnabled(false);
  mpModelSwitcherToolButton->setMenu(mpModelSwitcherMenu);
  mpModelSwitcherToolButton->setPopupMode(QToolButton::MenuButtonPopup);
  mpModelSwitcherToolButton->setIcon(QIcon(":/Resources/icons/switch.svg"));
  mpModelSwitcherToolBar->addWidget(mpModelSwitcherToolButton);
  // Plot Toolbar
  mpPlotToolBar = addToolBar(tr("Plot Toolbar"));
  mpPlotToolBar->setObjectName("Plot Toolbar");
  mpPlotToolBar->setAllowedAreas(Qt::TopToolBarArea);
  // add actions to Plot Toolbar
  mpPlotToolBar->addAction(mpReSimulateModelAction);
  mpPlotToolBar->addAction(mpReSimulateSetupAction);
  mpPlotToolBar->addSeparator();
  mpPlotToolBar->addAction(mpNewPlotWindowAction);
  mpPlotToolBar->addAction(mpNewParametricPlotWindowAction);
  mpPlotToolBar->addAction(mpClearPlotWindowAction);
}

//! when the dragged object enters the main window
//! @param event contains information of the drag operation.
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  event->setDropAction(Qt::CopyAction);
  event->accept();
}

//! Defines what happens when moving an object in the MainWindow.
//! @param event contains information of the drag operation.
void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
  if(event->mimeData()->hasFormat(Helper::modelicaFileFormat))
  {
    event->setDropAction(Qt::CopyAction);
    event->accept();
  }
  else
  {
    event->ignore();
  }
}

//! Defines what happens when drop a file in a Main Window.
//! @param event contains information of the drop operation.
void MainWindow::dropEvent(QDropEvent *event)
{
  if (!event->mimeData()->hasFormat(Helper::modelicaFileFormat))
  {
    event->ignore();
    return;
  }
  openDroppedFile(event);
  event->accept();
}

/*!
  Reimplementation of resizeEvent.\n
  Resizes the AboutOMEditWidget whenever the MainWindow is resized.
  */
void MainWindow::resizeEvent(QResizeEvent *event)
{
  if (mpAboutOMEditDialog)
    if (mpAboutOMEditDialog->isVisible())
      mpAboutOMEditDialog->setGeometry(QRect(rect().center() - QPoint(262, 235), rect().center() + QPoint(262, 235)));
  QMainWindow::resizeEvent(event);
}

InfoBar::InfoBar(QWidget *pParent)
  : QFrame(pParent)
{
  QPalette pal = palette();
  pal.setColor(QPalette::Window, QColor(255, 255, 225));
  pal.setColor(QPalette::WindowText, Qt::black);
  setPalette(pal);
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);
  mpInfoLabel = new Label;
  mpInfoLabel->setWordWrap(true);
  mpCloseButton = new QToolButton;
  mpCloseButton->setAutoRaise(true);
  mpCloseButton->setIcon(QIcon(":/Resources/icons/delete.svg"));
  mpCloseButton->setToolTip(Helper::close);
  connect(mpCloseButton, SIGNAL(clicked()), SLOT(hide()));
  // set the layout
  QHBoxLayout *pMainLayout = new QHBoxLayout;
  pMainLayout->setContentsMargins(0, 0, 0, 0);
  pMainLayout->setMargin(2);
  pMainLayout->addWidget(mpInfoLabel);
  pMainLayout->addWidget(mpCloseButton, 0, Qt::AlignTop);
  setLayout(pMainLayout);
}

void InfoBar::showMessage(QString message)
{
  mpInfoLabel->setText(message);
  show();
}

/*!
  \class AboutOMEditWidget
  \brief Creates a widget that shows the about text of OMEdit.

  Information about OpenModelica Connection Editor. Shows the list of OMEdit contributors.
  */

/*!
  \param pParent - pointer to MainWindow
  */
AboutOMEditWidget::AboutOMEditWidget(MainWindow *pMainWindow)
  : QWidget(pMainWindow)
{
  setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::information));
  setMinimumSize(525, 470);
  setMaximumSize(525, 470);
  mBackgroundPixmap.load(":/Resources/icons/about-us.png");
#ifdef Q_OS_MAC
  int MAC_FONT_FACTOR = 5;  /* the system font size in MAC is too small. */
#else
  int MAC_FONT_FACTOR = 0;
#endif
  // OMEdit intro text
  Label *pIntroLabel = new Label(Helper::applicationIntroText);
  pIntroLabel->setFont(QFont(Helper::systemFontInfo.family(), Helper::systemFontInfo.pointSize() + 3 + MAC_FONT_FACTOR));
  // OpenModelica compiler info
  Label *pConnectedLabel = new Label(QString("Connected to OpenModelica ").append(Helper::OpenModelicaVersion));
  pConnectedLabel->setFont(QFont(Helper::systemFontInfo.family(), Helper::systemFontInfo.pointSize() - 2 + MAC_FONT_FACTOR));
  // about text
  QString aboutText = QString("Copyright <b>Open Source Modelica Consortium (OSMC)</b>.<br />")
      .append("Distributed under OSMC-PL and GPL, see <u><a href=\"http://www.openmodelica.org\">www.openmodelica.org</a></u>.<br /><br />")
      .append("Initially developed by <b>Adeel Asghar</b> and <b>Sonia Tariq</b> as part of their final master thesis.");
  Label *pAboutTextLabel = new Label;
  pAboutTextLabel->setTextFormat(Qt::RichText);
  pAboutTextLabel->setTextInteractionFlags(pAboutTextLabel->textInteractionFlags() |Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
  pAboutTextLabel->setOpenExternalLinks(true);
  pAboutTextLabel->setFont(QFont(Helper::systemFontInfo.family(), Helper::systemFontInfo.pointSize() - 4 + MAC_FONT_FACTOR));
  pAboutTextLabel->setWordWrap(true);
  pAboutTextLabel->setText(aboutText);
  QVBoxLayout *pAboutLayout = new QVBoxLayout;
  pAboutLayout->setContentsMargins(0, 0, 0, 0);
  pAboutLayout->addWidget(pAboutTextLabel);
  // contributors heading
  QString contributorsHeading = QString("<b>Contributors:</b>");
  Label *pContributorsHeading = new Label;
  pContributorsHeading->setTextFormat(Qt::RichText);
  pContributorsHeading->setFont(QFont(Helper::systemFontInfo.family(), Helper::systemFontInfo.pointSize() - 4 + MAC_FONT_FACTOR));
  pContributorsHeading->setText(contributorsHeading);
  // contributors text
  QString contributorsText = QString("<ul style=\"margin: 0px 0px 0px -32px; padding: 2px;\">")
      .append("<li>Adeel Asghar - <u><a href=\"mailto:adeel.asghar@liu.se\">adeel.asghar@liu.se</a></u></li>")
      .append("<li>Sonia Tariq</li>")
      .append("<li>Martin Sjölund - <u><a href=\"mailto:martin.sjolund@liu.se\">martin.sjolund@liu.se</a></u></li>")
      .append("<li>Haris Kapidzic</li>")
      .append("<li>Abhinn Kothari</li>")
      .append("<li>Dr. Henning Kiel</li>")
      .append("<li>Alachew Shitahun</li>")
      .append("</ul>");
  Label *pContributorsLabel = new Label;
  pContributorsLabel->setTextFormat(Qt::RichText);
  pContributorsLabel->setTextInteractionFlags(pContributorsLabel->textInteractionFlags() | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
  pContributorsLabel->setOpenExternalLinks(true);
  pContributorsLabel->setFont(QFont(Helper::systemFontInfo.family(), Helper::systemFontInfo.pointSize() - 4 + MAC_FONT_FACTOR));
  pContributorsLabel->setText(contributorsText);
  // QScrollArea
  QScrollArea *pScrollArea = new QScrollArea;
  pScrollArea->setFrameShape(QFrame::NoFrame);
  pScrollArea->setBackgroundRole(QPalette::Base);
  pScrollArea->setWidgetResizable(true);
  pScrollArea->setWidget(pContributorsLabel);
  // close button
  QPushButton *pCloseButton = new QPushButton(Helper::close);
  connect(pCloseButton, SIGNAL(clicked()), SLOT(hide()));
  // set the layout
  QGridLayout *pMainLayout = new QGridLayout;
  pMainLayout->setContentsMargins(45, 200, 45, 20);
  pMainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pMainLayout->addWidget(pIntroLabel, 1, 0, 1, 1, Qt::AlignHCenter);
  pMainLayout->addWidget(pConnectedLabel, 2, 0, 1, 1, Qt::AlignHCenter);
  pMainLayout->addLayout(pAboutLayout, 3, 0, 1, 1);
  pMainLayout->addWidget(pContributorsHeading, 4, 0, 1, 1);
  pMainLayout->addWidget(pScrollArea, 5, 0, 1, 1);
  pMainLayout->addWidget(pCloseButton, 6, 0, 1, 1, Qt::AlignRight);
  setLayout(pMainLayout);
}

/*!
  Reimplementation of paintEvent.\n
  Draws the background image.
  \param event - pointer to QPaintEvent
  */
void AboutOMEditWidget::paintEvent(QPaintEvent *pEvent)
{
  QWidget::paintEvent(pEvent);
  QPainter painter(this);
  painter.drawPixmap((size().width() - mBackgroundPixmap.size().width())/2,
                     (size().height() - mBackgroundPixmap.size().height())/2, mBackgroundPixmap);
}

/*!
  Reimplementation of keyPressEvent.\n
  Hides the widget.
  */
void AboutOMEditWidget::keyPressEvent(QKeyEvent *pEvent)
{
  if (pEvent->key() == Qt::Key_Escape)
    hide();
  QWidget::keyPressEvent(pEvent);
}
