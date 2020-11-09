#include "serialreader.h"

SerialReader::SerialReader(QObject *parent) : QObject(parent) { qDebug() << "SerialReader created from " << QThread::currentThreadId(); }

SerialReader::~SerialReader() {
  end();
  delete serial;
  qDebug() << "SerialParser deleted from " << QThread::currentThreadId();
}

void SerialReader::init() {
  serial = new QSerialPort;
  connect(serial, &QSerialPort::bytesWritten, this, &SerialReader::finishedWriting);
  // V starším Qt (Win XP) není signál pro error
#if QT_VERSION >= 0x050800
  connect(serial, &QSerialPort::errorOccurred, this, &SerialReader::errorOccurred);
#endif
  qDebug() << "SerialReader initialised from " << QThread::currentThreadId();
}

void SerialReader::begin(QString portName, int baudRate) {
  if (serial->isOpen())
    end();
  serial->setPortName(portName);
  serial->setBaudRate(baudRate);
  serial->setDataBits(QSerialPort::Data8);
  serial->setParity(QSerialPort::NoParity);
  serial->setStopBits(QSerialPort::OneStop);
  serial->setFlowControl(QSerialPort::NoFlowControl);
  if (serial->open(QIODevice::ReadWrite)) {
    emit connectionResult(true, tr("Connected (") + serial->portName() + tr(", ") + QString::number(serial->baudRate()) + ")");
  } else if (serial->open(QIODevice::ReadOnly)) {
    emit connectionResult(true, tr("Connected (") + serial->portName() + tr(", ") + QString::number(serial->baudRate()) + ")");
  } else
    emit connectionResult(false, tr("Error: ") + serial->errorString());
  if (serial->isOpen()) {
    serial->clear();
    emit started();
  }
}

void SerialReader::write(QByteArray data) {
  if (serial->isOpen())
    serial->write(data);
}

void SerialReader::parserReady() { connect(serial, &QSerialPort::readyRead, this, &SerialReader::read); }

void SerialReader::end() {
  disconnect(serial, &QSerialPort::readyRead, this, &SerialReader::read);
  emit connectionResult(false, tr("Not connected"));
  if (!serial->isOpen())
    return;
  serial->close();
}

void SerialReader::errorOccurred() {
  if (serial->error() == QSerialPort::NoError)
    return;
  QString error = serial->errorString();
  if (serial->error() == QSerialPort::PermissionError)
    error = tr("Access denied.");
  disconnect(serial, &QSerialPort::readyRead, this, &SerialReader::read);
  if (serial->isOpen()) {
    serial->close();
  }
  emit connectionResult(false, tr("Error: ") + error);
}

void SerialReader::toggle(QString portName, int baudRate) {
  if (!serial->isOpen())
    begin(portName, baudRate);
  else
    end();
}

void SerialReader::read() {
  if (serialMonitor) {
    QByteArray newdata = serial->readAll();
    emit sendData(newdata);
    emit monitor(newdata);
  } else
    emit sendData(serial->readAll());
}
