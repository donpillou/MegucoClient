
#pragma once

class GraphView : public QWidget
{
public:
  GraphView(QWidget* parent, const PublicDataModel* publicDataModel, const QMap<QString, PublicDataModel*>& publicDataModels);

  void setFocusPublicDataModel(const PublicDataModel* publicDataModel);

  void setMaxAge(int maxAge);
  int getMaxAge() const {return maxAge;}

  enum class Data
  {
    trades = 0x01,
    tradeVolume = 0x02,
    orderBook = 0x04,
    regressionLines = 0x08,
    otherMarkets = 0x10,
    //estimates = 0x20,
    expRegressionLines = 0x20,
    all = 0xffff,
  };

  void setEnabledData(unsigned int data);
  unsigned int getEnabledData() const {return enabledData;}

  virtual QSize sizeHint() const;

private:
  const PublicDataModel* publicDataModel;
  const QMap<QString, PublicDataModel*>& publicDataModels;
  const GraphModel* graphModel;

  unsigned int enabledData;

  quint64 time;
  int maxAge;
  double totalMin;
  double totalMax;
  double volumeMax;

  class GraphModelData
  {
  public:
    QPointF* polyData;
    unsigned int polyDataSize;
    unsigned int polyDataCount;
    QPoint* volumeData;
    unsigned int volumeDataSize;
    unsigned int volumeDataCount;
    QColor color;
    bool drawn;

    GraphModelData() : polyData(0), polyDataSize(0), polyDataCount(0), volumeData(0), volumeDataSize(0), volumeDataCount(0), drawn(false) {}

    ~GraphModelData()
    {
      delete polyData;
      delete volumeData;
    }
  };
  QHash<const GraphModel*, GraphModelData> graphModelData;

  virtual void paintEvent(QPaintEvent* );

  void drawAxesLables(QPainter& painter, const QRect& rect, double hmin, double hmax, const QSize& priceSize);
  void prepareTradePolyline(const QRect& rect, double hmin, double hmax, double lastVolumeMax, const GraphModel& graphModel, int enabledData, double scale, const QColor& color);
  void drawTradePolylines(QPainter& painer);
  void drawBookPolyline(QPainter& painter, const QRect& rect, double hmin, double hmax);
  void drawRegressionLines(QPainter& painter, const QRect& rect, double hmin, double hmax);
  void drawExpRegressionLines(QPainter& painter, const QRect& rect, double hmin, double hmax);
  //void drawEstimates(QPainter& painter, const QRect& rect, double vmin, double vmax);

  inline void addToMinMax(double price)
  {
    if(price > totalMax)
      totalMax = price;
    if(price < totalMin)
      totalMin = price;
  }
};
