
#include "stdafx.h"
#include <cfloat>

GraphService::GraphService() : thread(0) {}

GraphService::~GraphService()
{
  stop();
}

void GraphService::start()
{
  if(thread)
  {
     stop();
     Q_ASSERT(!thread);
  }

  thread = new WorkerThread(*this, eventQueue, jobQueue);
  thread->start();
}

void GraphService::stop()
{
  if(!thread)
    return;

  jobQueue.append(0, 0); // cancel worker thread
  //thread->interrupt();
  thread->wait();
  delete thread;
  thread = 0;

  handleEvents(); // better than qDeleteAll(eventQueue.getAll()); ;)
  qDeleteAll(jobQueue.getAll());
}

void GraphService::registerGraphModel(GraphModel& graphModel)
{
  {
    QMutexLocker lock(&mutex);
    channelData.insert(&graphModel, ChannelData());
  }

  class RegisterGraphModelJob : public Job
  {
  public:
    RegisterGraphModelJob(GraphModel& graphModel) : graphModel(graphModel) {}
  private:
    GraphModel& graphModel;
  public: // Job
    virtual void execute(WorkerThread& workerThread)
    {
      QHash<GraphModel*, GraphRenderer>::Iterator it = workerThread.graphData.find(&graphModel);
      if(it != workerThread.graphData.end())
        workerThread.graphData.erase(it);
      const QString& channelName = graphModel.getChannelName();
      it = workerThread.graphData.insert(&graphModel, GraphRenderer(channelName));
      GraphRenderer& graphRenderer = *it;
      workerThread.graphDataByName.insert(graphModel.getChannelName(), &graphRenderer);
    }
  };
  jobQueue.append(new RegisterGraphModelJob(graphModel), 0);
}

void GraphService::unregisterGraphModel(GraphModel& graphModel)
{
  class UnregisterGraphModelJob : public Job
  {
  public:
    UnregisterGraphModelJob(GraphModel& graphModel) : graphModel(graphModel) {}
  private:
    GraphModel& graphModel;
  public: // Job
    virtual void execute(WorkerThread& workerThread)
    {
      QHash<GraphModel*, GraphRenderer>::Iterator it = workerThread.graphData.find(&graphModel);
      if(it == workerThread.graphData.end())
        return;
      workerThread.graphData.erase(it);
      workerThread.graphDataByName.remove(graphModel.getChannelName());
    }
  };
  jobQueue.append(new UnregisterGraphModelJob(graphModel), 0);

  {
    QMutexLocker lock(&mutex);
    channelData.remove(&graphModel);
  }
}

void GraphService::enable(GraphModel& graphModel, bool enable)
{
  class EnableJob : public Job
  {
  public:
    EnableJob(GraphModel& graphModel, bool enable) : graphModel(graphModel), enable(enable) {}
  private:
    GraphModel& graphModel;
    bool enable;
  public: // Job
    virtual void execute(WorkerThread& workerThread)
    {
      workerThread.graphData[&graphModel].enable(enable);
    }
  };
  jobQueue.append(new EnableJob(graphModel, enable), 0);
}

void GraphService::setSize(GraphModel& graphModel, const QSize& size)
{
  class SetSizeJob : public Job
  {
  public:
    SetSizeJob(GraphModel& graphModel, const QSize& size) : graphModel(graphModel), size(size) {}
  private:
    GraphModel& graphModel;
    QSize size;
  public: // Job
    virtual void execute(WorkerThread& workerThread)
    {
      workerThread.graphData[&graphModel].setSize(size);
    }
  };
  jobQueue.append(new SetSizeJob(graphModel, size), 0);
}

void GraphService::setMaxAge(GraphModel& graphModel, int maxAge)
{
  class SetMaxAgeJob : public Job
  {
  public:
    SetMaxAgeJob(GraphModel& graphModel, int maxAge) : graphModel(graphModel), maxAge(maxAge) {}
  private:
    GraphModel& graphModel;
    int maxAge;
  public: // Job
    virtual void execute(WorkerThread& workerThread)
    {
      workerThread.graphData[&graphModel].setMaxAge(maxAge);
    }
  };
  jobQueue.append(new SetMaxAgeJob(graphModel, maxAge), 0);
}

void GraphService::setEnabledData(GraphModel& graphModel, unsigned int data)
{
  class SetEnabledDataJob : public Job
  {
  public:
    SetEnabledDataJob(GraphModel& graphModel, unsigned int data) : graphModel(graphModel), data(data) {}
  private:
    GraphModel& graphModel;
    unsigned int data;
  public: // Job
    virtual void execute(WorkerThread& workerThread)
    {
      workerThread.graphData[&graphModel].setEnabledData(data);
    }
  };
  jobQueue.append(new SetEnabledDataJob(graphModel, data), 0);
}

void GraphService::addTradeData(GraphModel& graphModel, const QList<EMarketTradeData::Trade>& data)
{
  class AddTradeDataJob : public Job
  {
  public:
    AddTradeDataJob(GraphModel& graphModel, const QList<EMarketTradeData::Trade>& data) : graphModel(graphModel), data(data) {}
  private:
    GraphModel& graphModel;
    QList<EMarketTradeData::Trade> data;
  public: // Job
    virtual void execute(WorkerThread& workerThread)
    {
      workerThread.graphData[&graphModel].addTradeData(data);
      for(QHash<GraphModel*, GraphRenderer>::Iterator i = workerThread.graphData.begin(), end = workerThread.graphData.end(); i != end; ++i)
      {
        GraphRenderer& graphRenderer = *i;
        if(graphRenderer.getEnabledData() & GraphRenderer::otherMarkets)
          graphRenderer.setUpToDate(false);
      }
    }
  };
  jobQueue.append(new AddTradeDataJob(graphModel, data), 0);
}

void GraphService::addSessionMarker(GraphModel& graphModel, const EUserSessionMarker& marker)
{
  class AddSessionMarkerJob : public Job
  {
  public:
    AddSessionMarkerJob(GraphModel& graphModel, const EUserSessionMarker& marker) : graphModel(graphModel), marker(marker) {}
  private:
    GraphModel& graphModel;
    EUserSessionMarker marker;
  public: // Job
    virtual void execute(WorkerThread& workerThread)
    {
      workerThread.graphData[&graphModel].addSessionMarker(marker);
    }
  };
  jobQueue.append(new AddSessionMarkerJob(graphModel, marker), 0);
}

void GraphService::clearSessionMarker(GraphModel& graphModel)
{
  class ClearSessionMarkerJob : public Job
  {
  public:
    ClearSessionMarkerJob(GraphModel& graphModel) : graphModel(graphModel) {}
  private:
    GraphModel& graphModel;
  public: // Job
    virtual void execute(WorkerThread& workerThread)
    {
      workerThread.graphData[&graphModel].clearSessionMarker();
    }
  };
  jobQueue.append(new ClearSessionMarkerJob(graphModel), 0);
}

void GraphService::handleEvents()
{
  for(;;)
  {
    Event* event = 0;
    if(!eventQueue.get(event, 0) || !event)
      break;
    event->handle(*this);
    delete event;
  }
}

void GraphService::WorkerThread::run()
{
  Job* job;
  for(;;)
  {
    // wait for new job
    if(!jobQueue.get(job))
      return;

    // process job
  processJobs:
    do
    {
      if(!job)
      {
        canceled = true;
        return;
      }
      job->execute(*this);
      delete job;
    } while(jobQueue.get(job, 0));

    // redraw dirty graphs
    for(QHash<GraphModel*, GraphRenderer>::Iterator i = graphData.begin(), end = graphData.end(); i != end; ++i)
    {
      GraphRenderer& renderer = *i;
      if(renderer.isEnabled() && !renderer.isUpToDate())
      {
        GraphModel* graphModel = i.key();
        QImage& image = renderer.render(graphDataByName);

        // send data changed event
        {
          {
            QMutexLocker lock(&graphService.mutex);
            QHash<GraphModel*, ChannelData>::Iterator it =  graphService.channelData.find(graphModel);
            if(it == graphService.channelData.end())
              return;
            ChannelData& channelData = it.value();
            channelData.image.swap(image);
            channelData.updated = true;
          }
          class DataChangedEvent : public Event
          {
          public:
            DataChangedEvent(GraphModel& graphModel) : graphModel(graphModel) {}
          private:
            GraphModel& graphModel;
          public: // Event
            virtual void handle(GraphService& graphService)
            {
              {
                QMutexLocker lock(&graphService.mutex);
                QHash<GraphModel*, ChannelData>::Iterator it =  graphService.channelData.find(&graphModel);
                if(it == graphService.channelData.end())
                  return;
                ChannelData& channelData = it.value();
                if(!channelData.updated)
                  return;
                graphModel.swapImage(channelData.image);
                channelData.updated = false;
              }
              graphModel.redraw();
            }
          };
          bool wasEmpty;
          graphService.eventQueue.append(new DataChangedEvent(*graphModel), &wasEmpty);
          if(wasEmpty)
            QTimer::singleShot(0, &graphService, SLOT(handleEvents()));
        }

        if(jobQueue.get(job, 0))
          goto processJobs;
      }
    }
  }
}
