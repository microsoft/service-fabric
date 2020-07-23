// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace Api;
using namespace ServiceModel;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("PartitionContext");

class PartitionContext::RefreshStagingLocationAsyncOperation : public TimedAsyncOperation
{
public:
    RefreshStagingLocationAsyncOperation(
        PartitionContext & owner,
        uint64 const currentSequenceNumber,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , currentSequenceNumber_(currentSequenceNumber)
        , retryCount_(0)
    {
    }

    ~RefreshStagingLocationAsyncOperation()
    {
    }

    EventT<RefreshedStagingLocationEventArgs>::HHandler Add(EventT<RefreshedStagingLocationEventArgs>::EventHandler const & handler)
    {
        ASSERT_IFNOT(refreshedEvent_, "refreshedEvent_ should be set");
        return refreshedEvent_->Add(handler);
    }

    void Remove(EventT<RefreshedStagingLocationEventArgs>::HHandler const & handleOfHandler)
    {
        ASSERT_IFNOT(refreshedEvent_, "refreshedEvent_ should be set");
        refreshedEvent_->Remove(handleOfHandler);
    }
   
protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        bool alreadyExists = false;
        bool alreadyRefreshed = false;
        {
            AcquireWriteLock lock(owner_.lock_);
            
            if(owner_.refreshAsyncOperation_)
            {
                // Another thread already started the refresh 
                alreadyExists = true;
            }
            else if(currentSequenceNumber_ < owner_.currentShareLocationInfo_.SequenceNumber)
            {
                // Another thread already refreshed the StagingLocationInfo
                alreadyRefreshed = true;
            }
            else
            {
                refreshedEvent_ = make_shared<EventT<RefreshedStagingLocationEventArgs>>();
                owner_.refreshAsyncOperation_ = this->shared_from_this();
            }
        }

        if(alreadyExists)
        {
            TryComplete(thisSPtr, ErrorCodeValue::AlreadyExists);
            return;
        }

        if(alreadyRefreshed)
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }
        
        this->StartRefreshStagingLocation(thisSPtr);
    }

    void StartRefreshStagingLocation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.Root.TraceId,
            "Begin(GetStagingLocation): PartitionId:{0}",
            owner_.PartitionId);

        auto operation = owner_.fileStoreServiceClient_->BeginGetStagingLocation(
            owner_.PartitionId,
            FileStoreServiceConfig::GetConfig().GetStagingLocationTimeout,
            [this] (AsyncOperationSPtr const & operation) { this->OnGetStagingLocationComplete(operation, false); },
            thisSPtr);
        this->OnGetStagingLocationComplete(operation, true);
    }

    void OnGetStagingLocationComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        wstring newStagingLocation;
        auto error = owner_.fileStoreServiceClient_->EndGetStagingLocation(operation, newStagingLocation);
        WriteTrace(        
            error.ToLogLevel(),
            TraceComponent,
            owner_.Root.TraceId,
            "End(GetStagingLocation): NewStagingLocation:{0}, Error:{1} retryCount:{2}",
            newStagingLocation,
            error,
            retryCount_);

        if(!error.IsSuccess() && 
            IsRetryable(error) && 
            retryCount_ < FileStoreServiceConfig::GetConfig().MaxGetStagingLocationRetryAttempt &&
            this->RemainingTime >= FileStoreServiceConfig::GetConfig().GetStagingLocationTimeout)
        {
            ++retryCount_;
            StartRefreshStagingLocation(operation->Parent);
            return;
        }

        this->UpdateStagingLocationAndComplete(operation->Parent, error, newStagingLocation);
    }
    
    void UpdateStagingLocationAndComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error, wstring const & newStagingLocation)
    {
        shared_ptr<EventT<RefreshedStagingLocationEventArgs>> refreshedEvent;
        StagingLocationInfo refreshedInfo;
        {
            AcquireWriteLock lock(owner_.lock_);
            if(error.IsSuccess())
            {
                refreshedInfo = StagingLocationInfo(
                    newStagingLocation,
                    owner_.currentShareLocationInfo_.SequenceNumber + 1);
                owner_.currentShareLocationInfo_ = refreshedInfo;
            }

            owner_.refreshAsyncOperation_.reset();
            refreshedEvent = move(refreshedEvent_);
        }

        RefreshedStagingLocationEventArgs eventArg(refreshedInfo, error);
        refreshedEvent->Fire(eventArg, true);

        TryComplete(thisSPtr, move(error));
    }

    bool IsRetryable(ErrorCode const & error)
    {
        switch(error.ReadValue())
        {
        case ErrorCodeValue::NotPrimary:
        case ErrorCodeValue::Timeout:
            return true;

        default:
            return false;
        }
    }

private:
    uint64 const currentSequenceNumber_;
    PartitionContext & owner_;
    shared_ptr<EventT<RefreshedStagingLocationEventArgs>> refreshedEvent_;
    uint32 retryCount_;
};

class PartitionContext::GetStagingLocationAsyncOperation : public TimedAsyncOperation
{
public:
    GetStagingLocationAsyncOperation(
        PartitionContext & owner,
        StagingLocationInfo previousInfo,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , previousInfo_(previousInfo)
        , newInfo_()
        , refreshCount_(0)
        , timer_()
        , asyncOperationLock_()
    {
    }

    ~GetStagingLocationAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out StagingLocationInfo & stagingLocationInfo)
    {
        auto thisSPtr = AsyncOperation::End<GetStagingLocationAsyncOperation>(operation);
        stagingLocationInfo = thisSPtr->newInfo_;
        return thisSPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        bool startRefresh = false;
        bool waitForPendingRefresh = false;
        uint64 currentSequenceNumber = 0;
        {
            AcquireReadLock readLock(owner_.lock_);
            AcquireExclusiveLock exclusiveLock(this->asyncOperationLock_);

            if(owner_.refreshAsyncOperation_)
            {
                ASSERT_IF(timer_, "Timer should be set only once");
                timer_ = Timer::Create(
                    TimerTagDefault,
                    [this, thisSPtr] (TimerSPtr const& timer) 
                { 
                    timer->Cancel();
                    this->OnTimerExpired(thisSPtr); 
                },
                    false);

                auto casted = AsyncOperation::Get<RefreshStagingLocationAsyncOperation>(owner_.refreshAsyncOperation_);
                eventHandle_ = casted->Add(
                    [this, thisSPtr] (RefreshedStagingLocationEventArgs const & eventArg) { this->OnStagingLocationRefreshed(thisSPtr, eventArg); });                

                waitForPendingRefresh = true;
            }
            else if(previousInfo_.SequenceNumber < owner_.currentShareLocationInfo_.SequenceNumber)
            {
                newInfo_ = owner_.currentShareLocationInfo_;
            }
            else
            {                
                // If there are two competing threads to refresh the staging location,
                // the currentSequenceNumber check in RefreshStagingLocationAsyncOperation 
                // guarantees that only one will go through
                currentSequenceNumber = owner_.currentShareLocationInfo_.SequenceNumber;

                // Try to refresh three times before giving up
                if(refreshCount_++ < 3)
                {
                    startRefresh = true;
                }
                else
                {
                    error = ErrorCodeValue::OperationFailed;
                }
            }
        }               
        
        if(startRefresh)
        {
            this->StartRefresh(thisSPtr, currentSequenceNumber);
        }
        else if(waitForPendingRefresh)
        {
            timer_->Change(this->RemainingTime);
        }
        else
        {
            TryComplete(thisSPtr, error);            
        }
    }

    void StartRefresh(AsyncOperationSPtr const & thisSPtr, uint64 const currentSequenceNumber)
    {        
        owner_.StartRefresh(currentSequenceNumber);

        // Post the call on a different thread to avoid callstack from increasing
        Threadpool::Post(
            [this, thisSPtr] () { this->OnStart(thisSPtr); });
    }    

    void OnStagingLocationRefreshed(AsyncOperationSPtr const & thisSPtr, RefreshedStagingLocationEventArgs const & eventArg)
    {
        if(TryStartComplete())
        {
            AcquireExclusiveLock exclusiveLock(this->asyncOperationLock_);
            newInfo_ = eventArg.StagingLocation;
            timer_->Cancel();            

            FinishComplete(thisSPtr, eventArg.Error);
        }                
    }

    void OnTimerExpired(AsyncOperationSPtr const & thisSPtr)
    {
        if(TryStartComplete())
        {
            AcquireReadLock readLock(owner_.lock_);
            AcquireExclusiveLock exclusiveLock(this->asyncOperationLock_);
            if(owner_.refreshAsyncOperation_)
            {
                auto casted = AsyncOperation::Get<RefreshStagingLocationAsyncOperation>(owner_.refreshAsyncOperation_);
                casted->Remove(eventHandle_);
            }

            FinishComplete(thisSPtr, ErrorCodeValue::Timeout);
        }           
    }

private:
    StagingLocationInfo previousInfo_;
    StagingLocationInfo newInfo_;
    PartitionContext & owner_;    
    ExclusiveLock asyncOperationLock_;

    // access under lock
    uint refreshCount_;
    EventT<RefreshedStagingLocationEventArgs>::HHandler eventHandle_;
    TimerSPtr timer_;
};

PartitionContext::PartitionContext(
    ComponentRoot const & root, 
    Guid const & partitionId,    
    IInternalFileStoreServiceClientPtr const & fileStoreServiceClient)
    : RootedObject(root)
    , partitionId_(partitionId)
    , fileStoreServiceClient_(fileStoreServiceClient)
    , lock_()
    , currentShareLocationInfo_()
    , refreshAsyncOperation_()
{
}

PartitionContext::~PartitionContext()
{
}

Common::AsyncOperationSPtr PartitionContext::BeginGetStagingLocation(
    StagingLocationInfo previousInfo,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const &callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetStagingLocationAsyncOperation>(
        *this,
        previousInfo,
        timeout,
        callback,
        parent);
}

Common::ErrorCode PartitionContext::EndGetStagingLocation(
    Common::AsyncOperationSPtr const & operation,
    __out StagingLocationInfo & stagingLocationInfo)
{
    return GetStagingLocationAsyncOperation::End(operation, stagingLocationInfo);
}

bool PartitionContext::DoesMatchPartition(uint64 const partitionKey) const
{
    // Partitioned service not supported yet
    UNREFERENCED_PARAMETER(partitionKey);
    return true;
}

void PartitionContext::StartRefresh(uint64 const currentSequenceNumber)
{
    WriteNoise(        
        TraceComponent,
        Root.TraceId,
        "Begin(RefreshStagingLocation): CurrentSequenceNumber:{0}",
        currentSequenceNumber);

    auto operation = AsyncOperation::CreateAndStart<RefreshStagingLocationAsyncOperation>(
        *this,
        currentSequenceNumber,
        FileStoreServiceConfig::GetConfig().ShortRequestTimeout,
        [this] (AsyncOperationSPtr const & operation) { this->OnRefreshCompleted(operation, false); },
        Root.CreateAsyncOperationRoot());
    this->OnRefreshCompleted(operation, true);
}

void PartitionContext::OnRefreshCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = RefreshStagingLocationAsyncOperation::End(operation);
    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        Root.TraceId,
        "End(RefreshStagingLocation): Error:{0}",
        error);

}
