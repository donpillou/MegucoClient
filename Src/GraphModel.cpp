
#include "stdafx.h"

void GraphModel::addTrade(quint64 time, double price)
{
  if(totalMin == 0.)
    totalMin = totalMax = price;
  else if(price < totalMin)
    totalMin = price;
  else if(price > totalMax)
    totalMax = price;

  Entry& entry = trades[time];
  entry.time = time;
  if(entry.min == 0.)
    entry.min = entry.max = price;
  else if(price < entry.min)
    entry.min = price;
  else if(price > entry.max)
    entry.max = price;

  while(!trades.isEmpty() && time - trades.begin().key() > 7 * 24 * 60 * 60)
    trades.erase(trades.begin());

  emit dataAdded();
}

void GraphModel::addBookSummary(const BookSummary& bookSummary)
{
  bookSummaries.append(bookSummary);

  quint64 time = bookSummary.time;
  while(!bookSummaries.isEmpty() && time - bookSummaries.front().time > 7 * 24 * 60 * 60)
    bookSummaries.pop_front();

  emit dataAdded();
}
