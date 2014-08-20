
#include "stdafx.h"

bool DataConnection::connect(const QString& server, quint16 port)
{
  connection.close();
  recvBuffer2.clear();

  if(!connection.connect(server, port == 0 ? 40123 : port))
  {
    error = connection.getLastError();
    return false;
  }

  // request server time
  {
    DataProtocol::Header header;
    header.size = sizeof(header);
    header.destination = header.source = 0;
    header.messageType = DataProtocol::timeRequest;
    if(!connection.send((char*)&header, sizeof(header)))
    {
      error = connection.getLastError();
      return false;
    }
    qint64 localRequestTime = QDateTime::currentDateTime().toTime_t() * 1000ULL;
    char* data;
    size_t size;
    if(!receiveMessage(header, data, size))
      return false;
    qint64 localResponseTime = QDateTime::currentDateTime().toTime_t() * 1000ULL;
    if(header.messageType == DataProtocol::errorResponse && size >= sizeof(DataProtocol::ErrorResponse))
    {
      DataProtocol::ErrorResponse* errorResponse = (DataProtocol::ErrorResponse*)data;
      error = DataProtocol::getString(errorResponse->errorMessage);
      return false;
    }
    if(header.messageType != DataProtocol::timeResponse || size < sizeof(DataProtocol::TimeResponse))
    {
      error = "Could not request server time.";
      return false;
    }
    DataProtocol::TimeResponse* timeResponse = (DataProtocol::TimeResponse*)data;
    serverTimeToLocalTime = (localResponseTime - localRequestTime) / 2 + localRequestTime - timeResponse->time;
  }

  recvBuffer2.clear();
  return true;
}

void DataConnection::interrupt()
{
  connection.interrupt();
}

bool DataConnection::process(Callback& callback)
{
  this->callback = &callback;

  if(!connection.recv(recvBuffer2))
  {
    error = connection.getLastError();
    return false;
  }

  char* buffer = recvBuffer2.data();
  unsigned int bufferSize = recvBuffer2.size();

  for(;;)
  {
    if(bufferSize >= sizeof(DataProtocol::Header))
    {
      DataProtocol::Header* header = (DataProtocol::Header*)buffer;
      if(header->size < sizeof(DataProtocol::Header))
      {
        error = "Received invalid data.";
        return false;
      }
      if(bufferSize >= header->size)
      {
        handleMessage((DataProtocol::MessageType)header->messageType, (char*)(header + 1), header->size - sizeof(DataProtocol::Header));
        buffer += header->size;
        bufferSize -= header->size;
        continue;
      }
    }
    break;
  }
  if(buffer > recvBuffer2.data())
    recvBuffer2.remove(0, buffer - recvBuffer2.data());
  if(recvBuffer2.size() > 4000)
  {
    error = "Received invalid data.";
    return false;
  }
  return true;
}

bool DataConnection::receiveMessage(DataProtocol::Header& header, char*& data, size_t& size)
{
  if(connection.recv((char*)&header, sizeof(header), sizeof(header)) != sizeof(header))
  {
    error = connection.getLastError();
    return false;
  }
  if(header.size < sizeof(DataProtocol::Header))
  {
    error = "Received invalid data.";
    return false;
  }
  size = header.size - sizeof(header);
  if(size > 0)
  {
    recvBuffer2.resize(size);
    data = recvBuffer2.data();
    if(connection.recv(data, size, size) != size)
    {
      error = connection.getLastError();
      return false;
    }
  }
  return true;
}

void DataConnection::handleMessage(DataProtocol::MessageType messageType, char* data, unsigned int size)
{
  switch(messageType)
  {
  case DataProtocol::MessageType::channelResponse:
    {
      int count = size / sizeof(DataProtocol::Channel);
      DataProtocol::Channel* channel = (DataProtocol::Channel*)data;
      for(int i = 0; i < count; ++i, ++channel)
      {
        channel->channel[sizeof(channel->channel) - 1] = '\0';
        QString channelName(channel->channel);
        callback->receivedChannelInfo(channelName);
      }
    }
    break;
  case DataProtocol::MessageType::subscribeResponse:
    if(size >= sizeof(DataProtocol::SubscribeResponse))
    {
      DataProtocol::SubscribeResponse* subscribeResponse = (DataProtocol::SubscribeResponse*)data;
      subscribeResponse->channel[sizeof(subscribeResponse->channel) - 1] = '\0';
      QString channelName(subscribeResponse->channel);
      callback->receivedSubscribeResponse(channelName, subscribeResponse->channelId, subscribeResponse->flags);
    }
    break;
  case DataProtocol::MessageType::unsubscribeResponse:
    if(size >= sizeof(DataProtocol::UnsubscribeResponse))
    {
      DataProtocol::UnsubscribeResponse* unsubscribeResponse = (DataProtocol::UnsubscribeResponse*)data;
      unsubscribeResponse->channel[sizeof(unsubscribeResponse->channel) - 1] = '\0';
      QString channelName(unsubscribeResponse->channel);
      callback->receivedUnsubscribeResponse(channelName, unsubscribeResponse->channelId);
    }
    break;
  case DataProtocol::MessageType::tradeMessage:
    if(size >= sizeof(DataProtocol::TradeMessage))
    {
      DataProtocol::TradeMessage* tradeMessage = (DataProtocol::TradeMessage*)data;
      tradeMessage->trade.time += serverTimeToLocalTime;
      callback->receivedTrade(tradeMessage->channelId, tradeMessage->trade);
    }
    break;
  case DataProtocol::MessageType::tickerMessage:
    {
      DataProtocol::TickerMessage* tickerMessage = (DataProtocol::TickerMessage*)data;
      tickerMessage->ticker.time += serverTimeToLocalTime;
      callback->receivedTicker(tickerMessage->channelId, tickerMessage->ticker);
    }
    break;
  case DataProtocol::MessageType::errorResponse:
    if(size >= sizeof(DataProtocol::ErrorResponse))
    {
      DataProtocol::ErrorResponse* errorResponse = (DataProtocol::ErrorResponse*)data;
      errorResponse->errorMessage[sizeof(errorResponse->errorMessage) - 1] = '\0';
      QString errorMessage(errorResponse->errorMessage);
      callback->receivedErrorResponse(errorMessage);
    }
    break;
  default:
    break;
  }
}

bool DataConnection::loadChannelList()
{
  DataProtocol::Header header;
  header.size = sizeof(header);
  header.destination = header.source = 0;
  header.messageType = DataProtocol::channelRequest;
  if(!connection.send((char*)&header, sizeof(header)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool DataConnection::subscribe(const QString& channel, quint64 lastReceivedTradeId)
{
  char message[sizeof(DataProtocol::Header) + sizeof(DataProtocol::SubscribeRequest)];
  DataProtocol::Header* header = (DataProtocol::Header*)message;
  DataProtocol::SubscribeRequest* subscribeRequest = (DataProtocol::SubscribeRequest*)(header + 1);
  header->size = sizeof(message);
  header->destination = header->source = 0;
  header->messageType = DataProtocol::subscribeRequest;
  QByteArray channelNameData = channel.toUtf8();
  memcpy(subscribeRequest->channel, channelNameData.constData(), qMin(channelNameData.size() + 1, (int)sizeof(subscribeRequest->channel) - 1));
  subscribeRequest->channel[sizeof(subscribeRequest->channel) - 1] = '\0';
  if(lastReceivedTradeId)
  {
    subscribeRequest->maxAge = 0;
    subscribeRequest->sinceId =  lastReceivedTradeId;
  }
  else
  {
    subscribeRequest->maxAge = 24ULL * 60ULL * 60ULL * 1000ULL * 7ULL;
    subscribeRequest->sinceId =  0;
  }
  if(!connection.send(message, sizeof(message)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool DataConnection::unsubscribe(const QString& channel)
{
  char message[sizeof(DataProtocol::Header) + sizeof(DataProtocol::UnsubscribeRequest)];
  DataProtocol::Header* header = (DataProtocol::Header*)message;
  DataProtocol::UnsubscribeRequest* unsubscribeRequest = (DataProtocol::UnsubscribeRequest*)(header + 1);
  header->size = sizeof(message);
  header->destination = header->source = 0;
  header->messageType = DataProtocol::unsubscribeRequest;
  QByteArray channelNameData = channel.toUtf8();
  memcpy(unsubscribeRequest->channel, channelNameData.constData(), qMin(channelNameData.size() + 1, (int)sizeof(unsubscribeRequest->channel) - 1));
  unsubscribeRequest->channel[sizeof(unsubscribeRequest->channel) - 1] = '\0';
  if(!connection.send(message, sizeof(message)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool DataConnection::readTrade(quint64& channelId, DataProtocol::Trade& trade)
{
  struct ReadTradeCallback : public Callback
  {
    quint64& channelId;
    DataProtocol::Trade& trade;
    bool finished;

    ReadTradeCallback(quint64& channelId, DataProtocol::Trade& trade) : channelId(channelId), trade(trade), finished(false) {}

    virtual void receivedChannelInfo(const QString& channelName) {}
    virtual void receivedSubscribeResponse(const QString& channelName, quint64 channelId, quint32 flags) {}
    virtual void receivedUnsubscribeResponse(const QString& channelName, quint64 channelId) {}
    virtual void receivedTicker(quint64 channelId, const DataProtocol::Ticker& ticker) {}
    virtual void receivedErrorResponse(const QString& message) {}

    virtual void receivedTrade(quint64 channelId, const DataProtocol::Trade& trade)
    {
      this->channelId = channelId;
      this->trade = trade;
      finished = true;
    }
  } callback(channelId, trade);
  do
  {
    if(!process(callback))
      return false;
  } while(!callback.finished);
  return true;
}
