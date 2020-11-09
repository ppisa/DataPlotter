#ifndef SERIALREADER_H
#define SERIALREADER_H

#include "enums_defines_constants.h"
#include <QDebug>
#include <QObject>
#include <QSerialPort>
#include <QThread>

class SerialReader : public QObject {
  Q_OBJECT
public:
  explicit SerialReader(QObject *parent = nullptr);
  ~SerialReader();
  void init();

private:
  QSerialPort *serial;
  bool serialMonitor;

private slots:
  void read();
  void errorOccurred();
signals:
  /// Pošle informaci jestli je port připojen
  void connectionResult(bool, QString);
  /// Oznámý dokončení zápisu dat do portu
  void finishedWriting();
  /// Počle přečtená data
  void sendData(QByteArray data);
  /// Oznámí připojení (očekává odpověď že parser je připravený)
  void started();
  /// Přeposílá data
  void monitor(QByteArray data);
public slots:
  /// Připojí nebo odpojí port
  void toggle(QString portName, int baudRate);
  /// Pokusí se otevřít port
  void begin(QString portName, int baudRate);
  /// Zavře port
  void end();
  /// Zapíše data do portu
  void write(QByteArray data);
  /// Zahájí přeposílání dat
  void parserReady();
  /// Zapne předávání dat
  void enableMonitoring(bool en) { serialMonitor = en; }
};

#endif // SERIALREADER_H
