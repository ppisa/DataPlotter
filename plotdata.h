#ifndef PLOTTING_H
#define PLOTTING_H

#include <QDebug>
#include <QObject>
#include <QThread>
#include <QVector>
#include <QWidget>
#include <QtMath>

#include "enums_defines_constants.h"
#include "qcustomplot.h"

using namespace ValueType;
class PlotData : public QObject {
  Q_OBJECT
public:
  explicit PlotData(QObject *parent = nullptr);
  ~PlotData();
  void init();

private:
  int digitalChannel[ANALOG_COUNT];
  int logicBits[LOGIC_COUNT];
  double getValue(QByteArray number, ValueType::enumerator type);
  ValueType::enumerator getType(QByteArray array);
  OutputLevel::enumerator debugLevel = OutputLevel::info;
  double lastTimes[ANALOG_COUNT];
  double defaultTimestep = 1;
  void sendMessageIfAllowed(QString header, QByteArray message, MessageLevel::enumerator type);
  double arrayToDouble(QByteArray &array, bool &isok);
  uint32_t getBits(QByteArray data, ValueType::enumerator type);
public slots:
  void addPoint(QByteArrayList data);
  void addChannel(QByteArray data, unsigned int ch, QByteArray timeRaw, int bits, QByteArray min, QByteArray max);
  void reset();
  /// Nastavý úroveň výpisu
  void setDebugLevel(OutputLevel::enumerator debugLevel) { this->debugLevel = debugLevel; }

  void setDigitalChannel(int chid, int target);

  void setLogicBits(int target, int bits) { logicBits[target - 1] = bits; }

signals:
  /// Pošle zprávu do výpisu
  void sendMessage(QString header, QByteArray message, MessageLevel::enumerator type, MessageTarget::enumerator target = MessageTarget::serial1);

  /// Předá data do grafu
  void addVectorToPlot(int ch, QSharedPointer<QCPGraphDataContainer>, bool isMath = false);

  /// Předá data do grafu
  void addPointToPlot(int ch, double time, double value, bool append);

  void clearLogic(int group, int fromBit);
};

#endif // PLOTTING_H
