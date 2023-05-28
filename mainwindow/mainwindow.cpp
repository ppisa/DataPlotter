//  Copyright (C) 2020-2021  Jiří Maier

//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "mainwindow.h"

#include "mainwindow/version.h"
#include "ui_developeroptions.h"
#include "ui_freqtimeplotdialog.h"
#include "ui_manualinputdialog.h"
#include "ui_serialsettingsdialog.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), serialSettingsDialog(new SerialSettingsDialog(this)) {
  ui->setupUi(this);
  qApp->setStyle("Fusion");
  this->setAttribute(Qt::WA_NativeWindow);

  bool versionCahnged = true;
  configFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/config.ini";
  QFile version(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/version.txt");
  QByteArray versionstring;
  for (int i : ApplicationVersion)
    versionstring.append(QString(QString::number(i) + ".").toUtf8());
  versionstring.remove(versionstring.length() - 1, 1);
  if (version.open(QIODevice::ReadOnly)) {
    if (version.readAll() == versionstring)
      versionCahnged = false;
    version.close();
  }
  if (versionCahnged) {
    qDebug() << "Version changed";
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dir.removeRecursively();
    dir.mkpath(".");
    if (version.open(QIODevice::WriteOnly)) {
      version.write(versionstring);
      version.close();
    }
  }

  developerOptions = new DeveloperOptions(this, ui->quickWidget);
  freqTimePlotDialog = new FreqTimePlotDialog(nullptr);
  simulatedInputDialog.reset(new ManualInputDialog(nullptr));

  ui->doubleSpinBoxRangeVerticalRange->trimDecimalZeroes = true;
  ui->doubleSpinBoxRangeVerticalRange->emptyDefaultValue = 1;
  ui->doubleSpinBoxRangeHorizontal->trimDecimalZeroes = true;
  ui->doubleSpinBoxRangeHorizontal->emptyDefaultValue = 1;

  for (int i = 0; i < ANALOG_COUNT; i++)
    averagerCounts[i] = 8;

  freqTimePlotDialog->getUi()->plotPeak->setUptimeTimer(&uptime);
  uptime.start();

  initQmlTerminal();

  lightPalette = qApp->palette();

  QColor darkGray(53, 53, 53);
  QColor gray(128, 128, 128);
  QColor black(25, 25, 25);
  QColor blue(42, 130, 218);

  darkPalette.setColor(QPalette::Window, darkGray);
  darkPalette.setColor(QPalette::WindowText, Qt::white);
  darkPalette.setColor(QPalette::Base, black);
  darkPalette.setColor(QPalette::AlternateBase, darkGray);
  darkPalette.setColor(QPalette::ToolTipBase, blue);
  darkPalette.setColor(QPalette::ToolTipText, Qt::white);
  darkPalette.setColor(QPalette::Text, Qt::white);
  darkPalette.setColor(QPalette::Button, darkGray);
  darkPalette.setColor(QPalette::ButtonText, Qt::white);
  darkPalette.setColor(QPalette::Link, blue);
  darkPalette.setColor(QPalette::Highlight, blue);
  darkPalette.setColor(QPalette::HighlightedText, Qt::black);

  darkPalette.setColor(QPalette::Active, QPalette::Button, gray.darker());
  darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
  darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
  darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
  darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

  this->resize(1024, 768);
  on_radioButtonDark_toggled(ui->radioButtonDark->isChecked());

  auto newItem = new QListWidgetItem();
  newItem->setText(tr("Simulated"));
  newItem->setData(Qt::UserRole, "~SPECIAL~SIM");
  ui->listWidgetCom->addItem(newItem);

  auto newItem2 = new QListWidgetItem();
  newItem2->setText(tr("Telnet"));
  newItem2->setData(Qt::UserRole, "~SPECIAL~TELNET");
  ui->listWidgetCom->addItem(newItem2);
}

void MainWindow::closeEvent(QCloseEvent *event) {
  freqTimePlotDialog->close();
  simulatedInputDialog->close();
  developerOptions->close();
  ui->quickWidget->setSource(QUrl());
  ui->quickWidget->engine()->clearComponentCache();
  saveDefaultSettings();
  event->accept();
}

MainWindow::~MainWindow() {
  delete serialSettingsDialog;
  delete qmlTerminalInterface;
  delete developerOptions;
  delete freqTimePlotDialog;
  delete ui;
}

void MainWindow::setComboboxItemVisible(QComboBox &comboBox, int index, bool visible) {
  auto *model = qobject_cast<QStandardItemModel *>(comboBox.model());
  auto *item = model->item(index);
  item->setEnabled(visible);
  QListView *view = qobject_cast<QListView *>(comboBox.view());
  view->setRowHidden(index, !visible);
}

void MainWindow::setChStyleSelection(GraphType::enumGraphType type) {
  setComboboxItemVisible(*ui->comboBoxGraphStyle, GraphStyle::line, type == GraphType::analog || type == GraphType::math);
  setComboboxItemVisible(*ui->comboBoxGraphStyle, GraphStyle::point, type == GraphType::analog || type == GraphType::math);
  setComboboxItemVisible(*ui->comboBoxGraphStyle, GraphStyle::linePoint, type == GraphType::analog || type == GraphType::math);
  setComboboxItemVisible(*ui->comboBoxGraphStyle, GraphStyle::logic, type == GraphType::logic);
  setComboboxItemVisible(*ui->comboBoxGraphStyle, GraphStyle::logicFilled, type == GraphType::logic);
  setComboboxItemVisible(*ui->comboBoxGraphStyle, GraphStyle::logicpoints, type == GraphType::logic);
  setComboboxItemVisible(*ui->comboBoxGraphStyle, GraphStyle::logicSquare, type == GraphType::logic);
  setComboboxItemVisible(*ui->comboBoxGraphStyle, GraphStyle::logicSquareFilled, type == GraphType::logic);
}

void MainWindow::init(QTranslator *translator, const PlotData *plotData, const PlotMath *plotMath, SerialReader *serialReader, const Averager *avg) {
  // Načte ikony které se mění za běhu
  iconRun = QIcon(":/images/icons/run.png");
  iconPause = QIcon(":/images/icons/pause.png");
  iconCross = QIcon(":/images/icons/cross.png");
  iconHidden = QIcon(":/images/icons/hidden.png");
  iconVisible = QIcon(":/images/icons/visible.png");
  iconConnected = QIcon(":/images/icons/connected.png");
  iconNotConnected = QIcon(":/images/icons/disconnected.png");
  iconAbsoluteCursor = QIcon(":/images/icons/rangeTab.png");
  iconMaximize = QIcon(":/images/icons/maximize.png");
  iconUnMaximize = QIcon(":/images/icons/unmaximize.png");

  serialReader->setSimInputDialog(simulatedInputDialog);

  fillChannelSelect(); // Vytvoří seznam kanálů pro výběr

  QObject::connect(plotMath, &PlotMath::sendResult, ui->plot, &MyMainPlot::newDataVector);
  QObject::connect(plotData, &PlotData::addVectorToPlot, ui->plot, &MyMainPlot::newDataVector);
  QObject::connect(plotData, &PlotData::addPointToPlot, ui->plot, &MyMainPlot::newDataPoint);
  QObject::connect(plotData, &PlotData::clearLogic, ui->plot, &MyMainPlot::clearLogicGroup);
  QObject::connect(&fileSender, &FileSender::transmit, serialReader, &SerialReader::write);
  QObject::connect(qmlTerminalInterface, &QmlTerminalInterface::dataTransmitted, serialReader, &SerialReader::write);
  QObject::connect(avg, &Averager::addVectorToPlot, ui->plot, &MyMainPlot::newDataVector);
  QObject::connect(avg, &Averager::addPointToPlot, ui->plot, &MyMainPlot::newDataPoint);

  // Odpojit port když se změní pokročilá nastavení
  QObject::connect(serialSettingsDialog, &SerialSettingsDialog::settingChanged, serialReader, &SerialReader::end);

  this->translator = translator;
  setGuiArrays();
  initSetables();
  connectSignals();
  changeLanguage();
  setGuiDefaults();
  updateChScale();
  setAdaptiveSpinBoxes();
  setUp();
  startTimers();
}

void MainWindow::changeLanguage(QString code) {
  if (!translator->load(QString(":/translations/translations/translation_%1.qm").arg(code))) {
    qDebug() << "Can not load " << QString(":/translations/translations/translation_%1.qm").arg(code);
    return;
  }
  qApp->installTranslator(translator);
  ui->retranslateUi(this);
  serialSettingsDialog->getUi()->retranslateUi(serialSettingsDialog);
  developerOptions->getUi()->retranslateUi(developerOptions);
  freqTimePlotDialog->getUi()->retranslateUi(freqTimePlotDialog);
  freqTimePlotDialog->getUi()->plotPeak->setInfoText();
  simulatedInputDialog->getUi()->retranslateUi(simulatedInputDialog.data());
}

void MainWindow::showPlotStatus(PlotStatus::enumPlotStatus type) {
  if (type == PlotStatus::pause) {
    ui->pushButtonPause->setIcon(iconPause);
    ui->pushButtonPause->setToolTip(tr("Paused (click to resume)"));
  } else if (type == PlotStatus::run) {
    ui->pushButtonPause->setIcon(iconRun);
    ui->pushButtonPause->setToolTip(tr("Running (click to pause)"));
  }
}

void MainWindow::updateChScale() {
  if (ui->comboBoxSelectedChannel->currentIndex() < ANALOG_COUNT + MATH_COUNT) {
    double perDiv = ui->plot->getCHDiv(ui->comboBoxSelectedChannel->currentIndex());
    if (valuesUseUnits)
      ui->labelChScale->setText(floatToNiceString(perDiv, 3, true, false) + ui->plot->getYUnit() + tr(" / Div"));
    else
      ui->labelChScale->setText(QString::number(perDiv, 'g', 3) + ui->plot->getYUnit() + tr(" / Div"));
  } else
    ui->labelChScale->setText("---");
}

void MainWindow::serialConnectResult(bool connected, QString message, QString details) {
  ui->pushButtonConnect->setIcon(connected ? iconConnected : iconNotConnected);
  ui->labelPortInfo->setText(message);
  ui->labelPortInfo->setToolTip(details);
  if (connected && developerOptions->getUi()->checkBoxClearOnReconnect->isChecked()) {
    ui->plot->resetChannels();
    ui->plotxy->clear();
    ui->plotFFT->clear(0);
    ui->plotFFT->clear(1);
    freqTimePlotDialog->getUi()->plotPeak->clear();
    ansiTerminalModel.clear();
    ansiTerminalModel.setActive(false);
    ui->plainTextEditConsole->clear();
    ui->plainTextEditConsole_3->clear();
    emit resetChannels();
    emit resetAverager();
    for (int i = 0; i < ANALOG_COUNT; i++)
      channelExpectedRanges[i].unknown = true;
    pendingDeviceMessage = false;
    messageModel.clear();
    resetQmlTerminal();
    pendingMessagePart.clear();
  }
  if (connected && !developerOptions->getUi()->lineEditResetCmd->text().isEmpty()) {
    // Poslat reset příkaz
    QByteArray data = developerOptions->getUi()->textEditTerminalDebug->toPlainText().toLocal8Bit();
    data.replace("\\n", "\n");
    data.replace("\\r", "\r");
    emit writeToSerial(developerOptions->getUi()->lineEditResetCmd->text().toLocal8Bit());
  }

  autoAutosetPending = developerOptions->getUi()->checkBoxAutoAutoSet->isChecked();
}

void MainWindow::updateDivs() {
  updateChScale();
  if (ui->labelHDiv->isEnabled()) {
    QString unit = ui->plot->getXUnit();
    if (timeUseUnits)
      ui->labelHDiv->setText(floatToNiceString(ui->plot->getHDiv(), 1, false, false) + unit + tr("/Div"));
    else
      ui->labelHDiv->setText(QString::number(ui->plot->getHDiv(), 'g', 3) + tr("/Div"));

  } else
    ui->labelHDiv->setText("---");

  QString unit = ui->plot->getYUnit();
  if (valuesUseUnits)
    ui->labelVDiv->setText(floatToNiceString(ui->plot->getVDiv(), 1, false, false) + unit + tr("/Div"));
  else
    ui->labelVDiv->setText(QString::number(ui->plot->getVDiv(), 'g', 3) + unit + tr("/Div"));
}

void MainWindow::printMessage(QString messageHeader, QByteArray messageBody, int type, MessageTarget::enumMessageTarget target) {
  QString color = "<font color=gray>";
  switch (type) {
  case MessageLevel::warning:
    color = "<font color=orange>";
    break;
  case MessageLevel::error:
    color = "<font color=red>";
    break;
  case MessageLevel::info:
    color = "<font color=green>";
    break;
  default:
    color = "<font color=gray>";
  }

  QString stringMessage;
  stringMessage = messageBody;

  if (target == MessageTarget::serial1)
    // ui->plainTextEditConsole->appendHtml(color + QString(messageHeader) +
    // "</font color>" + (stringMessage.isEmpty() ? "" : ": ") + stringMessage);
    consoleBuffer.append(color + QString(messageHeader) + "</font color>" + (stringMessage.isEmpty() ? "" : ": ") + stringMessage);
  else
    developerOptions->getUi()->plainTextEditConsole_2->appendHtml(color + QString(messageHeader) + "</font color>" + (stringMessage.isEmpty() ? "" : ": ") + stringMessage);
}

void MainWindow::printDeviceMessage(QByteArray message, bool warning, bool ended) {
  if (message.contains('\a')) {
    QApplication::beep(); // Bell character
    message.replace('\a', "");
  }

  if (!pendingDeviceMessage && !message.isEmpty()) {
    if (warning)
      ui->plainTextEditConsole->appendHtml(tr("<font color=darkred>Device warning:</font color> "));
    else
      ui->plainTextEditConsole->appendHtml(tr("<font color=darkgreen>Device message:</font color> "));
    pendingMessageType = warning;
  }

  pendingMessagePart.append(message);
  if (ended) {
    messageModel.addMessage(pendingMessagePart, pendingMessageType ? 'w' : 'i');
    pendingMessagePart.clear();
  }

  ui->plainTextEditConsole->moveCursor(QTextCursor::End);
  ui->plainTextEditConsole->insertPlainText(message);
  ui->plainTextEditConsole->moveCursor(QTextCursor::End);
  QScrollBar *scroll = ui->plainTextEditConsole->horizontalScrollBar();
  scroll->setValue(scroll->minimum());
  pendingDeviceMessage = !ended;
}

void MainWindow::updateMathNow(int number) {
  emit setMathFirst(number, mathEn[number - 1]->isChecked() ? mathFirst[number - 1]->currentIndex() + 1 : 0);
  emit setMathSecond(number, mathEn[number - 1]->isChecked() ? mathSecond[number - 1]->currentIndex() + 1 : 0);
  emit clearMath(number);
  ui->plot->clearCh(getAnalogChId(number, ChannelType::math));
  if (mathEn[number - 1]->isChecked()) {
    MathOperations::enumMathOperations operation = (MathOperations::enumMathOperations)mathOp[number - 1]->currentIndex();
    QSharedPointer<QCPGraphDataContainer> in1, in2;

    if (mathFirst[number - 1]->currentIndex() < ANALOG_COUNT)
      in1 = ui->plot->graph(getAnalogChId(mathFirst[number - 1]->currentIndex() + 1, ChannelType::analog))->data();
    else
      in1 = ui->plot->graph(getAnalogChId(1, ChannelType::analog))->data();

    if (mathSecond[number - 1]->currentIndex() < ANALOG_COUNT)
      in2 = ui->plot->graph(getAnalogChId(mathSecond[number - 1]->currentIndex() + 1, ChannelType::analog))->data();
    else
      in2 = ui->plot->graph(getAnalogChId(1, ChannelType::analog))->data();

    emit resetMath(number, operation, in1, in2, mathFirst[number - 1]->currentIndex() == ANALOG_COUNT, mathSecond[number - 1]->currentIndex() == ANALOG_COUNT, mathScalarFirst[number - 1]->value(), mathScalarSecond[number - 1]->value());
  }
}

void MainWindow::interpolationResult(int chID, QSharedPointer<QCPGraphDataContainer> dataOriginal, QSharedPointer<QCPGraphDataContainer> dataInterpolated, bool dataIsFromInterpolationBuffer) {
  ui->plot->newInterpolatedVector(chID, dataOriginal, dataInterpolated, dataIsFromInterpolationBuffer);
  interpolationsRunning--;
  if (interpolationsRunning == 0)
    interpolationTimer.start();
}

void MainWindow::deviceError(QByteArray message, MessageTarget::enumMessageTarget source) {
  if (source == MessageTarget::manual) {
    QMessageBox msgBox(this);
    msgBox.setInformativeText(message);
    msgBox.setWindowTitle(tr("Device error"));
    msgBox.setText(tr("Error message sent from manual input"));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
  } else {
    emit disconnectSerial();
    QMessageBox msgBox(this);
    msgBox.setInformativeText("Message: " + message);
    msgBox.setWindowTitle(tr("Device error"));
    msgBox.setText(tr("Device reported error (port disconnected)"));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
  }
}

void MainWindow::on_pushButtonSerialSetting_clicked() { serialSettingsDialog->show(); }

void MainWindow::on_pushButtonSerialMoreInfo_clicked() {
  QString portinfo;

  auto portlist = QSerialPortInfo::availablePorts();
  QSerialPortInfo port;

  if (ui->listWidgetCom->currentItem() != NULL) {
    auto name = ui->listWidgetCom->currentItem()->data(Qt::UserRole);
    for (const auto &prt : qAsConst(portlist)) {
      if (name == prt.portName()) {
        port = prt;
      }
    }
    auto port = portlist.at(ui->listWidgetCom->currentRow());
  }

  portinfo.append(tr("Description: %1\n").arg(port.description()));
  portinfo.append(tr("Manufacturer: %1\n").arg(port.manufacturer()));
  portinfo.append(tr("Serial number: %1\n").arg(port.serialNumber()));
  portinfo.append(tr("Location: %1\n").arg(port.systemLocation()));
  portinfo.append(tr("Vendor Identifier: %1\n").arg(port.vendorIdentifier()));
  portinfo.append(tr("Product Identifier: %1").arg(port.productIdentifier()));

  QMessageBox msgBox(this);
  msgBox.setWindowTitle(port.portName());
  msgBox.setText(portinfo);
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
}

void MainWindow::on_comboBoxBaud_editTextChanged(const QString &arg1) {
  for (int i = 0; i < arg1.length(); i++) {
    if (!arg1.at(i).isDigit()) {
      ui->comboBoxBaud->setEditText(arg1.left(i));
      return;
    }
  }
}

void MainWindow::on_pushButtonHideCur1_clicked() {
  ui->checkBoxCur1Visible->setCheckState(Qt::CheckState::Unchecked);
  ui->checkBoxYCur1->setCheckState(Qt::CheckState::Unchecked);
}

void MainWindow::on_pushButtonHideCur2_clicked() {
  ui->checkBoxCur2Visible->setCheckState(Qt::CheckState::Unchecked);
  ui->checkBoxYCur2->setCheckState(Qt::CheckState::Unchecked);
}

void MainWindow::on_pushButtonAvg_toggled(bool checked) {
  emit resetAverager();
  emit setAverager(checked);
}

void MainWindow::on_spinBoxAvg_valueChanged(int arg1) {
  if (ui->radioButtonAverageAll->isChecked()) {
    for (int i = 0; i < ANALOG_COUNT; i++) {
      averagerCounts[i] = arg1;
      emit setAveragerCount(i, arg1);
    }
  } else {
    averagerCounts[ui->comboBoxAvgIndividualCh->currentIndex()] = arg1;
    emit setAveragerCount(ui->comboBoxAvgIndividualCh->currentIndex(), arg1);
  }
}

void MainWindow::on_radioButtonAverageIndividual_toggled(bool checked) {
  if (checked) {
    ui->comboBoxAvgIndividualCh->setCurrentIndex(0);
  } else {
    for (int i = 0; i < ANALOG_COUNT; i++) {
      averagerCounts[i] = ui->spinBoxAvg->value();
      emit setAveragerCount(i, ui->spinBoxAvg->value());
    }
  }
}

void MainWindow::on_comboBoxAvgIndividualCh_currentIndexChanged(int arg1) {
  ui->spinBoxAvg->blockSignals(true);
  ui->spinBoxAvg->setValue(averagerCounts[arg1]);
  ui->spinBoxAvg->blockSignals(false);
}

void MainWindow::checkBoxTriggerLineEn_stateChanged(int arg1) { ui->plot->setTriggerLineVisible(arg1 == Qt::Checked); }

void MainWindow::pushButtonClearGraph_clicked() {
  int chid = developerOptions->getUi()->comboBoxChClear->currentIndex();

  if (IS_LOGIC_INDEX(chid))
    ui->plot->clearLogicGroup(CH_LIST_INDEX_TO_LOGIC_GROUP(chid), 0);
  else
    ui->plot->clearCh(chid);
}

void MainWindow::on_lineEditHUnit_textChanged(const QString &arg1) {
  QString unit = arg1.simplified();

  QString prefixChars = "munkMG";
  timeUseUnits = true;
  if (unit.isEmpty())
    timeUseUnits = false;
  if (unit.length() >= 2) {
    if (prefixChars.contains(unit.at(0))) {
      timeUseUnits = false;
      unit.push_front(' ');
    }
  }

  ui->plot->setXUnit(unit, timeUseUnits);
  ui->plotxy->tUnit = unit;
  ui->doubleSpinBoxRangeHorizontal->setUnit(unit, timeUseUnits);
  ui->doubleSpinBoxXCur1->setUnit(unit, timeUseUnits);
  ui->doubleSpinBoxXCur2->setUnit(unit, timeUseUnits);

  freqUseUnits = (unit == "s");

  ui->plotFFT->setXUnit(freqUseUnits ? "Hz" : "", freqUseUnits);
  freqTimePlotDialog->getUi()->plotPeak->setYUnit(freqUseUnits ? "Hz" : "", freqUseUnits);

  if (unit != "s") {
    if (ui->comboBoxHAxisType->currentIndex() > HAxisType::normal) {
      ui->comboBoxHAxisType->setCurrentIndex(HAxisType::normal);
    }
  }

  updateDivs(); // Aby se aktualizovala jednotka u kroku mřížky
}

void MainWindow::on_pushButtonProtocolGuideCZ_clicked() { openResourceFileCopiedToLocal(":/docs/documentation/Data protocol guide cz.pdf"); }

void MainWindow::openResourceFileCopiedToLocal(QString file) {
  QFile resource(file);
  QString local = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + file.mid(file.lastIndexOf('/'));
  resource.open(QIODevice::ReadOnly);
  QFile::remove(local);
  resource.copy(local);
  if (!QDesktopServices::openUrl(QUrl::fromLocalFile(local))) {
    QMessageBox msgBox(this);
    msgBox.setText(tr("Cant open file."));
    msgBox.setInformativeText(local);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
  }
}

void MainWindow::checkBoxEchoReply_toggled(bool checked) { emit replyEcho(checked); }

void MainWindow::on_comboBoxBaud_currentTextChanged(const QString &arg1) {
  bool isok;
  qint32 baud = arg1.toUInt(&isok);
  if (isok)
    emit changeSerialBaud(baud);
}

void MainWindow::on_pushButtonRecordMeasurements1_clicked() {
  if (recordingOfMeasurements1.isOpen()) {
    recordingOfMeasurements1.close();
    recordingOfMeasurements1.setFileName("");
  } else {
    if (ui->comboBoxMeasure1->currentIndex() == ui->comboBoxMeasure1->count() - 1)
      return;
    QString defaultName = QString(QCoreApplication::applicationDirPath()) + QString("/%1 measurements").arg(getChName(ui->comboBoxMeasure1->currentIndex()));
    QString fileName = QFileDialog::getSaveFileName(this, tr("Record measurements"), defaultName, tr("Comma separated values (*.csv)"));
    if (fileName.isEmpty())
      return;
    recordingOfMeasurements1.setFileName(fileName);
    if (recordingOfMeasurements1.open(QFile::WriteOnly | QFile::Truncate)) {
      char separator = ui->radioButtonCSVDot->isChecked() ? ',' : ';';
      recordingOfMeasurements1.write(QByteArray("timestamp,period,freq,amp,min,max,vrms,dc,fs,rise,fall,"
                                                "samples\n")
                                         .replace(',', separator));
    }
  }

  ui->pushButtonRecordMeasurements1->setText(recordingOfMeasurements1.isOpen() ? tr("Stop") : tr("Record"));
}

void MainWindow::on_pushButtonRecordMeasurements2_clicked() {
  if (recordingOfMeasurements2.isOpen()) {
    recordingOfMeasurements2.close();
    recordingOfMeasurements2.setFileName("");
  } else {
    if (ui->comboBoxMeasure1->currentIndex() == ui->comboBoxMeasure1->count() - 1)
      return;
    QString defaultName = QString(QCoreApplication::applicationDirPath()) + QString("/%1 measurements").arg(getChName(ui->comboBoxMeasure1->currentIndex()));
    QString fileName = QFileDialog::getSaveFileName(this, tr("Record measurements"), defaultName, tr("Comma separated values (*.csv)"));
    if (fileName.isEmpty())
      return;
    recordingOfMeasurements2.setFileName(fileName);
    if (recordingOfMeasurements2.open(QFile::WriteOnly | QFile::Truncate)) {
      char separator = ui->radioButtonCSVDot->isChecked() ? ',' : ';';
      recordingOfMeasurements2.write(QByteArray("timestamp,period,freq,amp,min,max,vrms,dc,fs,rise,fall,"
                                                "samples\n")
                                         .replace(',', separator));
    }
  }

  ui->pushButtonRecordMeasurements1->setText(recordingOfMeasurements2.isOpen() ? tr("Stop") : tr("Record"));
}

QIcon MainWindow::invertIconLightness(const QIcon &icon, QSize size) {
  QPixmap originalPixmap = icon.pixmap(size);
  QImage image = originalPixmap.toImage();

  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      QColor color = image.pixelColor(x, y);
      if (color.hslSaturation() == 0) {
        double invertedLightness = 1.0 - color.lightnessF();
        color.setHsvF(color.hslHueF(), color.hslSaturationF(), invertedLightness, color.alphaF());
      }
      image.setPixelColor(x, y, color);
    }
  }

  QPixmap invertedPixmap = QPixmap::fromImage(image);
  return QIcon(invertedPixmap);
}

void MainWindow::on_radioButtonDark_toggled(bool checked) {
  if (currentThemeDark != checked) {
    currentThemeDark = checked;

    iconHidden = invertIconLightness(iconHidden, ui->pushButtonHideCh->iconSize());
    iconVisible = invertIconLightness(iconVisible, ui->pushButtonHideCh->iconSize());
    iconMaximize = invertIconLightness(iconMaximize, ui->pushButtonFFT_Maximize->iconSize());
    iconUnMaximize = invertIconLightness(iconUnMaximize, ui->pushButtonFFT_Maximize->iconSize());
    iconPause = invertIconLightness(iconPause, ui->pushButtonPause->iconSize());
    iconRun = invertIconLightness(iconRun, ui->pushButtonPause->iconSize());
    iconConnected = invertIconLightness(iconConnected, ui->pushButtonConnect->iconSize());
    iconNotConnected = invertIconLightness(iconNotConnected, ui->pushButtonConnect->iconSize());
    iconCross = invertIconLightness(iconCross, ui->comboBoxMeasure1->iconSize());
    iconAbsoluteCursor = invertIconLightness(iconAbsoluteCursor, ui->comboBoxCursor1Channel->iconSize());

    auto list1 = this->findChildren<QPushButton *>();
    list1.append(simulatedInputDialog->findChildren<QPushButton *>());
    list1.append(freqTimePlotDialog->findChildren<QPushButton *>());
    foreach (auto w, list1)
      w->setIcon(invertIconLightness(w->icon(), w->iconSize()));

    auto list6 = this->findChildren<QRadioButton *>();
    list6.append(simulatedInputDialog->findChildren<QRadioButton *>());
    list6.append(freqTimePlotDialog->findChildren<QRadioButton *>());
    foreach (auto w, list6)
      if (w != ui->radioButtonCz && w != ui->radioButtonEn)
        w->setIcon(invertIconLightness(w->icon(), w->iconSize()));

    auto list2 = this->findChildren<QTabBar *>();
    list2.append(simulatedInputDialog->findChildren<QTabBar *>());
    list2.append(freqTimePlotDialog->findChildren<QTabBar *>());
    foreach (auto w, list2) {
      for (int i = 0; i < w->count(); i++)
        w->setTabIcon(i, invertIconLightness(w->tabIcon(i), w->iconSize()));
    }

    auto list3 = this->findChildren<QLabel *>();
    list3.append(simulatedInputDialog->findChildren<QLabel *>());
    list3.append(freqTimePlotDialog->findChildren<QLabel *>());
    foreach (auto w, list3) {
      if (w == ui->labelLogo)
        continue;
      if (!w->pixmap(Qt::ReturnByValue))
        continue;
      auto icon = w->pixmap(Qt::ReturnByValue).toImage();
      icon.invertPixels(QImage::InvertRgb);
      w->setPixmap(QPixmap::fromImage(icon));
    }

    qApp->setPalette(checked ? darkPalette : lightPalette);
    QColor bck = checked ? QColor::fromRgb(67, 67, 67) : Qt::white;
    QColor fnt = !checked ? QColor::fromRgb(67, 67, 67) : Qt::white;

    auto list4 = this->findChildren<QCustomPlot *>();
    list4.append(freqTimePlotDialog->findChildren<QCustomPlot *>());
    foreach (auto plot, list4) {
      plot->setBackground(bck);
      plot->axisRect()->setBackground(bck);
      plot->xAxis->setBasePen(fnt);
      plot->xAxis->setLabelColor(fnt);
      plot->xAxis->setTickLabelColor(fnt);
      plot->xAxis->setTickPen(fnt);
      plot->xAxis->setSubTickPen(fnt);

      plot->yAxis->setBasePen(fnt);
      plot->yAxis->setLabelColor(fnt);
      plot->yAxis->setTickLabelColor(fnt);
      plot->yAxis->setTickPen(fnt);
      plot->yAxis->setSubTickPen(fnt);

      plot->replot(QCustomPlot::rpQueuedReplot);
    }

    QList<QComboBox *> list5;
    list5.append(ui->comboBoxFFTStyle1);
    list5.append(ui->comboBoxFFTStyle2);
    list5.append(ui->comboBoxXYStyle);
    list5.append(ui->comboBoxGraphStyle);
    list5.append(ui->comboBoxCursor1Channel);
    list5.append(ui->comboBoxCursor2Channel);
    list5.append(ui->comboBoxMeasure1);
    list5.append(ui->comboBoxMeasure2);
    list5.append(ui->comboBoxMath1Op);
    list5.append(ui->comboBoxMath2Op);
    list5.append(ui->comboBoxMath3Op);
    foreach (auto w, list5) {
      for (int i = 0; i < w->count(); i++)
        w->setItemIcon(i, invertIconLightness(w->itemIcon(i), w->iconSize()));
    }
  }

  setStyleSheet(QString(checked ? "QFrame {background-color: rgb(67, 67, 67);}"
                                  "QMessageBox {background-color: rgb(67, 67, 67);}"
                                  "QInputDialog {background-color: rgb(67, 67, 67);}"
                                : "QFrame {background-color: rgb(255, 255, 255);}"
                                  "QMessageBox {background-color: rgb(255, 255, 255);}"
                                  "QInputDialog {background-color: rgb(255, 255, 255);}") +
                "QWidget#widget {background-color: " + qApp->palette().color(QPalette::Window).name() + ";}" + +"QWidget#widget_2 {background-color: " + qApp->palette().color(QPalette::Window).name() + ";}" + "QSplitter#splitter {background-color: " + qApp->palette().color(QPalette::Window).name() +
                ";}");

  qmlTerminalInterface->setDarkThemeIsUsed(checked);
  qmlTerminalInterface->setTabBackground(checked ? "#434343" : "#ffffff");
}

void MainWindow::printSerialMonitor(QByteArray data) { serialMonitor.append(QString::fromStdString(data.toStdString())); }

void MainWindow::mainPlotHRangeChanged(QCPRange range) {
  if (ui->plot->getRollingMode()) {
    ui->doubleSpinBoxRangeHorizontal->blockSignals(true);
    ui->doubleSpinBoxRangeHorizontal->setValue(range.size());
    ui->dialRollingRange->updatePosition(range.size());
    ui->doubleSpinBoxRangeHorizontal->blockSignals(false);
  } else {
    int m = ui->dialZoom->maximum();
    double fullrange = ui->plot->getMaxT() - ui->plot->getMinT();
    double sizeRatio = qBound(0.0, range.size() / fullrange, 1.0);
    int zoomFactor = round(m * sizeRatio);

    ui->dialZoom->blockSignals(true);
    ui->dialZoom->setValue(zoomFactor);
    ui->dialZoom->blockSignals(false);

    double posRatio = (range.center() - ui->plot->getMinT()) / fullrange;
    double posFactor = round(m * qBound(0.0, posRatio, 1.0));
    ui->horizontalScrollBarHorizontal->blockSignals(true);
    ui->horizontalScrollBarHorizontal->setPageStep(zoomFactor);
    ui->horizontalScrollBarHorizontal->setMinimum(zoomFactor / 2);
    ui->horizontalScrollBarHorizontal->setMaximum(m - zoomFactor / 2);
    ui->horizontalScrollBarHorizontal->setValue(posFactor);
    ui->horizontalScrollBarHorizontal->setSliderPosition(posFactor);
    ui->horizontalScrollBarHorizontal->blockSignals(false);
  }
}

void MainWindow::mainPlotHRangeMaxChanged(QCPRange range) {
  range = ui->plot->xAxis->range();
  int m = ui->dialZoom->maximum();
  double fullrange = ui->plot->getMaxT() - ui->plot->getMinT();
  double sizeRatio = qBound(0.0, range.size() / fullrange, 1.0);
  int zoomFactor = round(m * sizeRatio);

  ui->dialZoom->blockSignals(true);
  ui->dialZoom->setValue(zoomFactor);
  ui->dialZoom->blockSignals(false);

  double posRatio = (range.center() - ui->plot->getMinT()) / fullrange;
  double posFactor = round(m * qBound(0.0, posRatio, 1.0));
  ui->horizontalScrollBarHorizontal->blockSignals(true);
  ui->horizontalScrollBarHorizontal->setPageStep(zoomFactor);
  ui->horizontalScrollBarHorizontal->setMinimum(zoomFactor / 2);
  ui->horizontalScrollBarHorizontal->setMaximum(m - zoomFactor / 2);
  ui->horizontalScrollBarHorizontal->setValue(posFactor);
  ui->horizontalScrollBarHorizontal->setSliderPosition(posFactor);
  ui->horizontalScrollBarHorizontal->blockSignals(false);
}

void MainWindow::mainPlotVRangeChanged(QCPRange range) {
  int m = ui->dialZoom->maximum();
  double fullrange = ui->doubleSpinBoxRangeVerticalRange->value();
  double sizeRatio = qBound(0.0, range.size() / fullrange, 1.0);
  int zoomFactor = round(m * sizeRatio);

  double min = ui->doubleSpinBoxViewCenter->value() - ui->doubleSpinBoxRangeVerticalRange->value() / 2;
  double posRatio = (range.center() - min) / fullrange;
  double posFactor = round(m * qBound(0.0, posRatio, 1.0));
  ui->verticalScrollBarVertical->blockSignals(true);
  ui->verticalScrollBarVertical->setPageStep(zoomFactor);
  ui->verticalScrollBarVertical->setMinimum(zoomFactor / 2);
  ui->verticalScrollBarVertical->setMaximum(m - zoomFactor / 2);
  ui->verticalScrollBarVertical->setValue(posFactor);
  ui->verticalScrollBarVertical->setSliderPosition(posFactor);
  ui->verticalScrollBarVertical->blockSignals(false);
}

void MainWindow::mainPlotVRangeMaxChanged(QCPRange range) {
  ui->doubleSpinBoxViewCenter->blockSignals(true);
  ui->doubleSpinBoxRangeVerticalRange->blockSignals(true);
  ui->doubleSpinBoxViewCenter->setValue(range.center());
  ui->doubleSpinBoxRangeVerticalRange->setValue(range.size());
  ui->dialVerticalRange->updatePosition(range.size());
  ui->doubleSpinBoxViewCenter->blockSignals(false);
  ui->doubleSpinBoxRangeVerticalRange->blockSignals(false);
  mainPlotVRangeChanged(ui->plot->yAxis->range());
}

void MainWindow::lastDataTypeWasPointChanged(bool wasPoint) {
  if (autoAutosetPending) {
    if (wasPoint) {
      on_pushButtonResetChannels_clicked();
      ui->pushButtonRollingAutoRange->setChecked(true);
    } else
      on_pushButtonAutoset_clicked();
    autoAutosetPending = false;
  }
}

void MainWindow::checkedVersion(bool isNew, QString message) {
  QMessageBox msgBox(this);
  msgBox.setText(message);
  msgBox.setIcon(QMessageBox::Information);
  msgBox.setStandardButtons(isNew ? (QMessageBox::Yes | QMessageBox::No) : QMessageBox::Ok);
  msgBox.setDefaultButton(isNew ? QMessageBox::Yes : QMessageBox::Ok);
  msgBox.setButtonText(QMessageBox::Yes, tr("Download"));
  msgBox.setButtonText(QMessageBox::No, tr("Close"));
  auto checkBox = new QCheckBox(&msgBox);
  checkBox->setText(tr("Check for updates at startup"));
  checkBox->setChecked(checkForUpdatesAtStartup);
  msgBox.setCheckBox(checkBox);
  int returnValue = msgBox.exec();
  if (returnValue == QMessageBox::Yes)
    QDesktopServices::openUrl(DownloadUrl);
  checkForUpdatesAtStartup = checkBox->isChecked();
}

void MainWindow::plotMaximizeButtonClicked(QString id) {
  setPlotLayout(hasMaximizedPlot ? "all" : id);
  ui->pushButtonFFT_Maximize->setIcon(hasMaximizedPlot ? iconUnMaximize : iconMaximize);
  ui->pushButtonXY_Maximize->setIcon(hasMaximizedPlot ? iconUnMaximize : iconMaximize);
}

void MainWindow::on_pushButtonCheckForUpdates_clicked() { updateChecker.checkForUpdates(false); }
