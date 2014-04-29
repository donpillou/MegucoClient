
#pragma once

class BotProtocol
{
public:
  enum MessageType
  {
    pingRequest,
    pingResponse, // a.k.a. pong
    loginRequest,
    loginResponse,
    authRequest,
    authResponse,
    registerBotRequest,
    registerBotResponse,
    registerMarketRequest,
    registerMarketResponse,
    updateEntity,
    removeEntity,
    controlEntity,
    createEntity,
    requestEntities,
  };
  
  enum EntityType
  {
    error,
    session,
    engine,
    marketAdapter,
    sessionTransaction,
    sessionOrder,
    market,
    marketTransaction,
    marketOrder,
    marketBalance,
  };

#pragma pack(push, 1)
  struct Header
  {
    quint32 size; // including header
    quint16 messageType; // MessageType
  };

  struct Entity
  {
    quint16 entityType; // EntityType
    quint32 entityId;
  };

  struct LoginRequest
  {
    char userName[33];
  };

  struct LoginResponse
  {
    unsigned char userKey[32];
    unsigned char loginKey[32];
  };

  struct AuthRequest
  {
    unsigned char signature[32];
  };

  struct Error : public Entity
  {
    char errorMessage[129];
  };

  struct Session : public Entity
  {
    enum State
    {
      stopped,
      starting,
      running,
      simulating,
    };

    char name[33];
    quint32 botEngineId;
    quint32 marketId;
    quint8 state;
  };

  struct BotEngine : public Entity
  {
    char name[33];
  };

  struct MarketAdapter : public Entity
  {
    char name[33];
    char currencyBase[33];
    char currencyComm[33];
  };

  struct Transaction : public Entity
  {
    enum Type
    {
      buy,
      sell
    };

    quint8 type;
    qint64 date;
    double price;
    double amount;
    double fee;
  };

  struct Order : public Entity
  {
    enum Type
    {
      buy,
      sell,
    };

    quint8 type;
    qint64 date;
    double price;
    double amount;
    double fee;
  };

  struct Market : public Entity
  {
    enum State
    {
      stopped,
      starting,
      running,
    };

    quint32 marketAdapterId;
    quint8 state;
  };

  struct CreateSessionArgs : public Entity
  {
    char name[33];
    quint32 engineId;
    quint32 marketId;
    double balanceBase;
    double balanceComm;
  };

  struct ControlSessionArgs : public Entity
  {
    enum Command
    {
      startSimulation,
      stop,
      select,
    };

    quint8 cmd;
  };

  struct CreateMarketArgs : public Entity
  {
    quint32 marketAdapterId;
    char userName[33];
    char key[65];
    char secret[65];
  };

  struct ControlMarketArgs : public Entity
  {
    enum Command
    {
      select,
      refreshTransactions,
      refreshOrders,
    };

    quint8 cmd;
  };

#pragma pack(pop)

};
