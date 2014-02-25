
#include "stdafx.h"
#include <cfloat>

GraphView::GraphView(QWidget* parent, const PublicDataModel* publicDataModel, const QMap<QString, PublicDataModel*>& publicDataModels) :
  QWidget(parent), publicDataModel(publicDataModel), 
  publicDataModels(publicDataModels), graphModel(0),
  enabledData((unsigned int)Data::all), time(0), maxAge(60 * 60),
  totalMin(DBL_MAX), totalMax(0.), volumeMax(0.)
{
  if(publicDataModel)
  {
    graphModel = &publicDataModel->graphModel;
    connect(graphModel, SIGNAL(dataAdded()), this, SLOT(update()));
  }
}

QSize GraphView::sizeHint() const
{
  return QSize(400, 300);
}

void GraphView::setMaxAge(int maxAge)
{
  this->maxAge = maxAge;
}

void GraphView::setEnabledData(unsigned int data)
{
  this->enabledData = data;
}

void GraphView::paintEvent(QPaintEvent* event)
{
  if(!graphModel || !graphModel->synced)
    return;

  if(!graphModel->tradeSamples.isEmpty())
  {
    const GraphModel::TradeSample& tradeSample = graphModel->tradeSamples.back();
    addToMinMax(tradeSample.max);
    addToMinMax(tradeSample.min);
    if(tradeSample.time > time)
      time = tradeSample.time;
  }

  if(!graphModel->bookSamples.isEmpty())
  {
    const GraphModel::BookSample& bookSample = graphModel->bookSamples.back();
    addToMinMax(bookSample.ask);
    addToMinMax(bookSample.bid);
    for(int i = 0; i < (int)GraphModel::BookSample::ComPrice::numOfComPrice; ++i)
      addToMinMax(bookSample.comPrice[i]);
    if(bookSample.time > time)
      time = bookSample.time;
  }

  if(enabledData & (int)Data::otherMarkets)
  {
    foreach(const PublicDataModel* publicDataModel, publicDataModels)
    {
      if(!publicDataModel || publicDataModel->graphModel.tradeSamples.isEmpty())
        continue;
      const GraphModel::TradeSample& tradeSample = publicDataModel->graphModel.tradeSamples.back();
      if(tradeSample.time > time)
        time = tradeSample.time;
    }
  }

  for(;;)
  {
    QRect rect = this->rect();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::black);

    double vmin = totalMin;
    double vmax = qMax(totalMax, vmin + 1.);
    const QSize priceSize = painter.fontMetrics().size(Qt::TextSingleLine, publicDataModel->formatPrice(totalMax == 0. ? 0. : vmax));

    QRect plotRect(10, 10, rect.width() - (10 + priceSize.width() + 8), rect.height() - 20 - priceSize.height() + 5);
    if(plotRect.width() <= 0 || plotRect.height() <= 0)
      return; // too small

    double lastTotalMin = totalMin;
    double lastTotalMax = totalMax;
    double lastVolumeMax = volumeMax;

    totalMin = DBL_MAX;
    totalMax = 0.;
    volumeMax = 0.;

    if(enabledData & (int)Data::otherMarkets)
    {
      static unsigned int colors[] = { 0x0000FF, 0x8A2BE2, 0xA52A2A, 0x5F9EA0, 0x7FFF00, 0xD2691E, 0xFF7F50, 0x6495ED, 0xDC143C, 0x00FFFF, 0x00008B, 0x008B8B, 0xB8860B, 0xA9A9A9, 0x006400, 0xBDB76B, 0x8B008B, 0x556B2F, 0xFF8C00, 0x9932CC, 0x8B0000, 0xE9967A, 0x8FBC8F, 0x483D8B, 0x2F4F4F, 0x00CED1, 0x9400D3, 0xFF1493, 0x00BFFF, 0x696969, 0x1E90FF, 0xB22222, 0x228B22, 0xFF00FF, 0xFFD700, 0xDAA520, 0x808080, 0x008000, 0xADFF2F, 0xFF69B4, 0xCD5C5C, 0x4B0082 };
      int nextColorIndex = 0;
      double averagePrice = graphModel->getVwap24();
      if(averagePrice > 0.)
        foreach(const PublicDataModel* publicDataModel, publicDataModels)
        {
          if(publicDataModel)
          {
            if(publicDataModel != this->publicDataModel || !(enabledData & ((int)Data::trades)))
            {
              int colorIndex = nextColorIndex % (sizeof(colors) / sizeof(*colors));
              double otherAveragePrice = publicDataModel->graphModel.getVwap24();
              if(otherAveragePrice > 0.)
              {
                QColor color(colors[colorIndex]);
                color.setAlpha(0x70);
                prepareTradePolyline(plotRect, vmin, vmax, lastVolumeMax, publicDataModel->graphModel, (int)Data::trades, averagePrice / otherAveragePrice, color);
              }
            }
            nextColorIndex++;
          }
        }
    }
    if(enabledData & ((int)Data::trades | (int)Data::tradeVolume) && !graphModel->tradeSamples.isEmpty())
      prepareTradePolyline(plotRect, vmin, vmax, lastVolumeMax, *graphModel, enabledData, 1., QColor(0, 0, 0));

    if(totalMax == 0.)
      return; // no data to draw
    totalMin = floor(totalMin);
    totalMax = ceil(totalMax);
    if(!(totalMin == lastTotalMin && totalMax == lastTotalMax && (!(enabledData & (int)Data::tradeVolume) || qAbs(lastVolumeMax -  volumeMax) <= volumeMax * 0.5)))
      continue;

    drawAxesLables(painter, plotRect, vmin, vmax, priceSize);
    if(enabledData & (int)Data::orderBook && !graphModel->bookSamples.isEmpty())
      drawBookPolyline(painter, plotRect, vmin, vmax);

    drawTradePolylines(painter);

    if(enabledData & (int)Data::regressionLines)
      drawRegressionLines(painter, plotRect, vmin, vmax);
    if(enabledData & (int)Data::expRegressionLines)
      drawExpRegressionLines(painter, plotRect, vmin, vmax);
    //if(enabledData  & (int)Data::estimates && !graphModel->tradeSamples.isEmpty())
    //  drawEstimates(painter, plotRect, vmin, vmax);
    break;
  }
}

void GraphView::drawAxesLables(QPainter& painter, const QRect& rect, double vmin, double vmax, const QSize& priceSize)
{
  QPen linePen(Qt::gray);
  linePen.setStyle(Qt::DotLine);
  QPen textPen(Qt::darkGray);

  {
    double vrange = vmax - vmin;
    double height = rect.height();
    double vstep = pow(10., ceil(log10(vrange * 20. / height)));
    if(vstep * height / vrange >= 40.f)
      vstep *= 0.5;
    if(vstep * height / vrange >= 40.f)
      vstep *= 0.5;

    double vstart = ceil(vmin / vstep) * vstep - vmin;

    for(int i = 0;; ++i)
    {
      QPoint right(rect.right() + 4, rect.bottom() - (vstart + vstep * i) * height / vrange);

      if(right.y() < rect.top())
        break;

      painter.setPen(linePen);
      painter.drawLine(QPoint(0, right.y()), right);
      painter.setPen(textPen);
      QPoint textPos(right.x() + 2, right.y() + priceSize.height() * 0.5 - 3);
      if(textPos.y() > 2)
        painter.drawText(textPos, publicDataModel->formatPrice(vmin + vstart + i * vstep));
    }
  }

  {
    quint64 hmax = time;
    quint64 hmin = hmax - maxAge;

    double hrange = hmax - hmin;
    double width = rect.width();
    double hstep = pow(10., ceil(log10(hrange * 35. / width)));
    if(hstep * width / hrange >= 70.f)
      hstep *= 0.5;
    if(hstep * width / hrange >= 70.f)
      hstep *= 0.5;
    if(hstep < 60.)
      hstep = 60.;
    else if(hstep > 60. && hstep < (5. * 60.))
      hstep = ceil(hstep / (5. * 60.)) * (5. * 60.);
    else if(hstep > (5. * 60.) && hstep < (15. * 60.))
      hstep = ceil(hstep / (15. * 60.)) * (15. * 60.);
    else if(hstep > (15. * 60.) && hstep < (30. * 60.))
      hstep = ceil(hstep / (30. * 60.)) * (30. * 60.);
    else if(hstep > (30. * 60.) && hstep < (60. * 60.))
      hstep = ceil(hstep / (60. * 60.)) * (60. * 60.);
    else if(hstep > (60. * 60.) && hstep < 2. * 60. * 60.)
      hstep = ceil(hstep / (2. * 60. * 60.)) * (2. * 60. * 60.);
    else if(hstep > (2. * 60. * 60.) && hstep < 3. * 60. * 60.)
      hstep = ceil(hstep / (3. * 60. * 60.)) * (3. * 60. * 60.);
    else if(hstep > (3. * 60. * 60.) && hstep < 6. * 60. * 60.)
      hstep = ceil(hstep / (6. * 60. * 60.)) * (6. * 60. * 60.);
    else if(hstep > (6 * 60. * 60.) && hstep < 12. * 60. * 60.)
      hstep = ceil(hstep / (12. * 60. * 60.)) * (12. * 60. * 60.);
    else if(hstep > (12. * 60. * 60.) && hstep < 24. * 60. * 60.)
      hstep = ceil(hstep / (24. * 60. * 60.)) * (24. * 60. * 60.);

    double hstart = hmax - floor(hmax / hstep) * hstep;

    QString timeFormat = QLocale::system().timeFormat(QLocale::ShortFormat).replace(":ss", "");
    QString formatedTime;
    QDateTime date;
    for(int i = 0; ; ++i)
    {
      QPoint bottom(rect.right() - (hstart + hstep * i) * width / hrange, rect.bottom() + 4);

      if(bottom.x() < rect.left())
        break;

      painter.setPen(linePen);
      painter.drawLine(QPoint(bottom.x(), 0), bottom);
      painter.setPen(textPen);
      date = QDateTime::fromTime_t(hmax - (hstart + hstep * i));
      //date.setTimeSpec(Qt::UTC);
      formatedTime = date.toLocalTime().time().toString(timeFormat);
      const QSize timeSize = painter.fontMetrics().size(Qt::TextSingleLine, formatedTime);
      QPoint textPos(bottom.x() - timeSize.width() * 0.5, rect.bottom() + 2 + timeSize.height());
      if(textPos.x() > 2)
        painter.drawText(textPos, formatedTime);
    }
  }
}

void GraphView::prepareTradePolyline(const QRect& rect, double ymin, double ymax, double lastVolumeMax, const GraphModel& graphModel, int enabledData, double scale, const QColor& color)
{
  double yrange = ymax - ymin;
  quint64 vmax = time;
  quint64 vmin = vmax - maxAge;
  quint64 vrange = vmax - vmin;
  int left = rect.left();
  double bottom = rect.bottom();
  int bottomInt = rect.bottom();
  double height = rect.height();
  quint64 width = rect.width();

  GraphModelData& graphModelData = this->graphModelData[&graphModel];
  unsigned int polyDataSize = rect.width() * 4;
  unsigned int volumeDataSize = rect.width() * 2;
  if(graphModelData.polyDataSize < polyDataSize)
  {
    delete graphModelData.polyData;
    graphModelData.polyData = new QPointF[polyDataSize];
    graphModelData.polyDataSize = polyDataSize;
  }
  if(graphModelData.volumeDataSize < volumeDataSize)
  {
    delete graphModelData.volumeData;
    graphModelData.volumeData = new QPoint[volumeDataSize];
    graphModelData.volumeDataSize = volumeDataSize;
  }
  QPointF* polyData = graphModelData.polyData;
  QPoint* volumeData = graphModelData.volumeData;

  if(&graphModel != this->graphModel)
    enabledData &= ~(int)Data::tradeVolume;

  {
    QPointF* currentPoint = polyData;
    QPoint* currentVolumePoint = volumeData;

    const QList<GraphModel::TradeSample>& tradeSamples = graphModel.tradeSamples;
    int i = 0, count = tradeSamples.size();
    for(int step = qMax(tradeSamples.size() / 2, 1);;)
    {
      if(i + step < count && tradeSamples.at(i + step).time < vmin)
        i += step;
      else if(step == 1)
        break;
      if(step > 1)
        step /= 2;
    }
    if(i < count)
    {
      const GraphModel::TradeSample* sample = &tradeSamples.at(i);

      int pixelX = 0;
      quint64 currentTimeMax = vmin + vrange / rect.width();
      double currentMin = sample->min;
      double currentMax = sample->max;
      double currentVolume = 0;
      int currentEntryCount = 0;
      double currentLast = 0.;
      double currentFirst = sample->first;
      double lastLast = 0.;
      int lastLeftPixelX = 0;

      for(;;)
      {
        if(sample->time > currentTimeMax)
          goto addLine;

        if(sample->min < currentMin)
          currentMin = sample->min;
        if(sample->max > currentMax)
          currentMax = sample->max;
        currentVolume += sample->amount;
        currentLast = sample->last;
        ++currentEntryCount;

        if(++i >= count)
          goto addLine;
        sample = &tradeSamples.at(i);
        continue;

      addLine:
        if(currentEntryCount > 0)
        {
          if(enabledData & (int)Data::tradeVolume)
          {
            currentVolumePoint->setX(left + pixelX);
            currentVolumePoint->setY(bottomInt);
            ++currentVolumePoint;
            currentVolumePoint->setX(left + pixelX);
            currentVolumePoint->setY(bottomInt - currentVolume * (height * (1. / 3.)) / lastVolumeMax);
            ++currentVolumePoint;

            if(currentVolume > volumeMax)
              volumeMax = currentVolume;
          }

          if(currentMin * scale < totalMin)
            totalMin = currentMin * scale;
          if(currentMax * scale > totalMax)
            totalMax = currentMax * scale;

          if(lastLeftPixelX != 0 && lastLeftPixelX != left + pixelX - 1)
          {
            currentPoint->setX(left + pixelX - 1);
            currentPoint->setY(bottom - (lastLast * scale - ymin) * height / yrange);
            ++currentPoint;
          }
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentFirst * scale - ymin) * height / yrange);
          ++currentPoint;
          if(currentFirst - currentMin < currentFirst - currentMax)
          {
            if(currentMin != currentFirst)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMin * scale - ymin) * height / yrange);
              ++currentPoint;
            }
            if(currentMax != currentMin)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMax * scale - ymin) * height / yrange);
              ++currentPoint;
            }
            if(currentLast != currentMax)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentLast * scale - ymin) * height / yrange);
              ++currentPoint;
            }
          }
          else
          {
            if(currentMax != currentFirst)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMax * scale - ymin) * height / yrange);
              ++currentPoint;
            }
            if(currentMin != currentMax)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentMin * scale - ymin) * height / yrange);
              ++currentPoint;
            }
            if(currentLast != currentMin)
            {
              currentPoint->setX(left + pixelX);
              currentPoint->setY(bottom - (currentLast * scale - ymin) * height / yrange);
              ++currentPoint;
            }
          }
          lastLast = currentLast;
          lastLeftPixelX = left + pixelX;
        }

        if(i >= count)
          break;
        ++pixelX;
        currentTimeMax = vmin + vrange * (pixelX + 1) / width;
        currentMin = sample->min;
        currentMax = sample->max;
        currentEntryCount = 0;
        currentVolume = 0;
        currentFirst = sample->first;
      }
    }

    Q_ASSERT(currentPoint - polyData <= rect.width() * 4);
    Q_ASSERT(currentVolumePoint - volumeData <= rect.width() * 2);

    graphModelData.drawn = true;
    graphModelData.color = color;
    graphModelData.polyDataCount = currentPoint - polyData;
    graphModelData.volumeDataCount = (currentVolumePoint - volumeData) / 2;
  }
}

void GraphView::drawTradePolylines(QPainter& painter)
{
  bool cleanup = false;
  GraphModelData* focusGraphModelData = 0;
  for(QHash<const GraphModel*, GraphModelData>::Iterator i = graphModelData.begin(), end = graphModelData.end(); i != end; ++i)
  {
    GraphModelData& graphModelData = *i;
    if(!graphModelData.drawn)
    {
      cleanup = true;
      continue;
    }
    const GraphModel* graphMode = i.key();
    if(graphMode == this->graphModel)
      focusGraphModelData = &graphModelData;
    else
    {
      if(graphModelData.polyDataCount > 0)
      {
        QPen rangePen(graphModelData.color);
        rangePen.setWidth(2);
        painter.setPen(rangePen);
        painter.drawPolyline(graphModelData.polyData, graphModelData.polyDataCount);
      }
    }
    graphModelData.drawn = false;
  }
  if(focusGraphModelData)
  {
    if(focusGraphModelData->volumeDataCount > 0)
    {
      QPen volumePen(Qt::darkBlue);
      painter.setPen(volumePen);
      painter.drawLines(focusGraphModelData->volumeData, focusGraphModelData->volumeDataCount);
    }
    if(focusGraphModelData->polyDataCount > 0)
    {
      QPen rangePen(focusGraphModelData->color);
      rangePen.setWidth(2);
      painter.setPen(rangePen);
      painter.drawPolyline(focusGraphModelData->polyData, focusGraphModelData->polyDataCount);
    }
  }
  if(cleanup)
  {
    for(QHash<const GraphModel*, GraphModelData>::Iterator i = graphModelData.begin(), end = graphModelData.end(); i != end; ++i)
    {
      const GraphModelData& graphModelData = *i;
      if(!graphModelData.drawn)
      {
        this->graphModelData.erase(i);
        break;
      }
    }
  }
}

void GraphView::drawBookPolyline(QPainter& painter, const QRect& rect, double ymin, double ymax)
{
  double yrange = ymax - ymin;
  quint64 vmax = time;
  quint64 vmin = vmax - maxAge;
  quint64 vrange = vmax - vmin;
  int left = rect.left();
  double bottom = rect.bottom();
  double height = rect.height();
  quint64 width = rect.width();

  QPointF* polyData = (QPointF*)alloca(rect.width() * sizeof(QPointF));

  for (int type = 0; type < (int)GraphModel::BookSample::ComPrice::numOfComPrice; ++type)
  {
    QPointF* currentPoint = polyData;

    const QList<GraphModel::BookSample>& bookSummaries = graphModel->bookSamples;
    int i = 0, count = bookSummaries.size();
    for(; i < count; ++i) // todo: optimize this
      if(bookSummaries.at(i).time >= vmin) 
        break;
    if(i < count)
    {
      const GraphModel::BookSample* sample = &bookSummaries.at(i);

      int pixelX = 0;
      quint64 currentTimeMax = vmin + vrange / rect.width();
      double currentVal = sample->comPrice[type];
      int currentEntryCount = 0;

      for(;;)
      {
        if(sample->time > currentTimeMax)
          goto addLine;

        currentVal = sample->comPrice[type];
        ++currentEntryCount;
        if(currentVal > totalMax)
          totalMax = currentVal;
        if(currentVal < totalMin)
          totalMin = currentVal;

        if(++i >= count)
          goto addLine;
        sample = &bookSummaries.at(i);
        continue;

      addLine:
        if(currentEntryCount > 0)
        {
          currentPoint->setX(left + pixelX);
          currentPoint->setY(bottom - (currentVal - ymin) * height / yrange);
          ++currentPoint;
        }
        if(i >= count)
          break;
        ++pixelX;
        currentTimeMax = vmin + vrange * (pixelX + 1) / width;
        currentVal = sample->comPrice[type];
        currentEntryCount = 0;
      }
    }

    int color = type * 0xff / (int)GraphModel::BookSample::ComPrice::numOfComPrice;
    painter.setPen(QColor(0xff - color, 0, color));
    painter.drawPolyline(polyData, currentPoint - polyData);
  }
}

void GraphView::drawRegressionLines(QPainter& painter, const QRect& rect, double vmin, double vmax)
{
  double vrange = vmax - vmin;
  quint64 hmax = time;
  quint64 hmin = hmax - maxAge;
  quint64 hrange = hmax - hmin;
  quint64 width = rect.width();
  double height = rect.height();

  for(int i = 0; i < (int)GraphModel::RegressionDepth::numOfRegressionDepths; ++i)
  {
    const GraphModel::RegressionLine& rl = graphModel->regressionLines[i];
    if(rl.endTime == 0)
      break;

    quint64 startTime = qMax(rl.startTime, hmin);
    if((qint64)hmin - (qint64)rl.startTime > maxAge / 2)
      break;
    quint64 endTime = rl.endTime;
    double val = rl.a - rl.b * (endTime - startTime);
    QPointF a(rect.left() + (startTime - hmin) * width / hrange, rect.bottom() - (val -  vmin) * height / vrange);
    QPointF b(rect.right() - (time - endTime) * width / hrange, rect.bottom() - (rl.a -  vmin) * height / vrange);

    int color = qMin((int)((0xdd - 0x64) * qMin(qMax(fabs(rl.b / 0.005), 0.), 1.)), 0xdd - 0x64);
    QPen pen(rl.b >= 0 ? QColor(0, color + 0x64, 0) : QColor(color + 0x64, 0, 0));
    pen.setWidth(2);
    painter.setPen(pen);

    painter.drawLine(a, b);
  }
}

void GraphView::drawExpRegressionLines(QPainter& painter, const QRect& rect, double vmin, double vmax)
{
  double vrange = vmax - vmin;
  quint64 hmax = time;
  quint64 hmin = hmax - maxAge;
  quint64 hrange = hmax - hmin;
  quint64 width = rect.width();
  double height = rect.height();

  for(int i = 0; i < (int)GraphModel::RegressionDepth::numOfExpRegessionDepths; ++i)
  {
    const GraphModel::RegressionLine& rl = graphModel->expRegressionLines[i];
    if(rl.endTime == 0)
      break;

    quint64 startTime = qMax(rl.startTime, hmin);
    if((qint64)hmin - (qint64)rl.startTime > maxAge / 2)
      break;
    quint64 endTime = rl.endTime;
    double val = rl.a - rl.b * (endTime - startTime);
    QPointF a(rect.left() + (startTime - hmin) * width / hrange, rect.bottom() - (val -  vmin) * height / vrange);
    QPointF b(rect.right() - (time - endTime) * width / hrange, rect.bottom() - (rl.a -  vmin) * height / vrange);

    int color = qMin((int)((0xdd - 0x64) * qMin(qMax(fabs(rl.b / 0.005), 0.), 1.)), 0xdd - 0x64);
    QPen pen(rl.b >= 0 ? QColor(0, color + 0x64, 0) : QColor(color + 0x64, 0, 0));
    pen.setWidth(2);
    painter.setPen(pen);

    painter.drawLine(a, b);
  }
}
/*
void GraphView::drawEstimates(QPainter& painter, const QRect& rect, double vmin, double vmax)
{
  double vrange = vmax - vmin;
  quint64 hmax = time;
  quint64 hmin = hmax - maxAge;
  quint64 hrange = hmax - hmin;
  quint64 width = rect.width();
  double height = rect.height();

  for(int i = 0; i < (int)sizeof(graphModel->estimations) / sizeof(*graphModel->estimations); ++i)
  {
    const GraphModel::Estimator& estimator = graphModel->estimations[i];
    quint64 startTime = hmin;
    quint64 endTime = time;
    double bb = estimator.incline;
    QPointF a(rect.left(), rect.bottom() - (estimator.estimate + (-bb) * (endTime - startTime) -  vmin) * height / vrange);
    QPointF b(rect.right(), rect.bottom() - (estimator.estimate -  vmin) * height / vrange);

    int color = qMin((int)((0xdd - 0x64) * qMin(qMax(fabs(bb / 0.005), 0.), 1.)), 0xdd - 0x64);
    QPen pen(estimator.incline >= 0 ? QColor(0, color + 0x64, 0) : QColor(color + 0x64, 0, 0));
    pen.setWidth(2);
    painter.setPen(pen);

    painter.drawLine(a, b);
  }
}
*/

void GraphView::setFocusPublicDataModel(const PublicDataModel* publicDataModel)
{
  if(graphModel)
  {
    disconnect(graphModel, SIGNAL(dataAdded()), this, SLOT(update()));
    graphModel = 0;
    this->publicDataModel = 0;
  }
  if(publicDataModel)
  {
    this->publicDataModel = publicDataModel;
    graphModel = &publicDataModel->graphModel;
    connect(graphModel, SIGNAL(dataAdded()), this, SLOT(update()));
  }
  update();
}
