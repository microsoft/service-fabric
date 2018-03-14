// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Store
{
    typedef StoreTransactionTemplate<ReplicatedStore> StoreTransactionInternal;

    StringLiteral const TraceComponent("FabricTimeController");

    FabricTimeController::FabricTimeController(__in ReplicatedStore & replicatedStore)
        :replicatedStore_(replicatedStore)
        ,root_(replicatedStore.Root.shared_from_this())
        ,PartitionedReplicaTraceComponent(replicatedStore.PartitionedReplicaId)
        ,ready_(false)
        ,cancelled_(false)
    {
       refreshActive_.store(false);
    }

    FabricTimeController::~FabricTimeController()
    {

    }
    
    void FabricTimeController::Cancel()
    {
		cancelled_.store(true);

        if (!refreshActive_.load())
        {
            refreshTimer_->Cancel();
        }
    }

    void FabricTimeController::Start(bool)
    {
        auto componentRoot=this->CreateComponentRoot();
        refreshTimer_=Timer::Create(
            TimerTagDefault,
            [componentRoot,this](TimerSPtr const &)
            {
                this->RefreshAndPersist();
            },
            true);

        fabricTimeData_.Start();    
        //Start the timer immediately for the first run      
        TryScheduleTimer(TimeSpan::Zero);
    }

    void FabricTimeController::RefreshAndPersist()
    {
        shared_ptr<ScopedActiveFlag> scopedActiveFlag;
        if (!ScopedActiveFlag::TryAcquire(refreshActive_, scopedActiveFlag))
        {
            WriteInfo(
                TraceComponent, 
                "{0} FabricTimeController refresh already active",
                this->TraceId);
            TryScheduleTimer(StoreConfig::GetConfig().FabricTimePersistInterval);
            return;
        }
        
		if (cancelled_.load())
		{
			WriteInfo(
				TraceComponent,
				"{0} FabricTimeController refresh already cencelled",
				this->TraceId);
            refreshTimer_->Cancel();
			return;
		}

		ActivityId activityId;
        StoreTransactionInternal tx=StoreTransactionInternal::Create(&replicatedStore_,replicatedStore_.PartitionedReplicaId,activityId);

        //if the controller is not ready, then initialize the controller
        if(!ready_)
        {
            //first time call            
            ErrorCode error=tx.Exists(fabricTimeData_);
            if(error.IsError(ErrorCodeValue::NotFound))
            {
                fabricTimeData_.RefreshAndGetCurrentStoreTime();
                error=tx.Insert(fabricTimeData_);
                if(!error.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} Fail to insert logical time in replicated store. Error:{1}, PartitionReplicaId:{2}, Activity:{3}",
                        this->TraceId,
                        error,
                        PartitionedReplicaId,
                        activityId);       
                    TryScheduleTimer(StoreConfig::GetConfig().FabricTimePersistInterval);
                    return;
                }
            }
            //load logical time from store
            else
            {
                error=tx.ReadExact(fabricTimeData_);
                if(!error.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} Fail to read logical time in replicated store. Error:{1}, PartitionReplicaId:{2}, Activity:{3}",
                        this->TraceId,
                        error,
                        PartitionedReplicaId,
                        activityId);       
                    TryScheduleTimer(StoreConfig::GetConfig().FabricTimePersistInterval);
                    return;
                }
            }
            auto operation=StoreTransactionInternal::BeginCommit(
                std::move(tx),
                StoreConfig::GetConfig().FabricTimeRefreshTimeoutValue,
                [this,activityId](AsyncOperationSPtr const & operation) 
                    { 
                        RefreshAndPersistCompletedCallback(operation,true,false,activityId);          
                    },
                this->CreateAsyncOperationRoot());
            //Complete Synchronously
            RefreshAndPersistCompletedCallback(operation,true,true,activityId);   
        }
        else
        {
            //refresh the value
            fabricTimeData_.RefreshAndGetCurrentStoreTime();
            tx.Update(fabricTimeData_,false);
            auto componentRoot=this->CreateComponentRoot();
            auto operation=StoreTransactionInternal::BeginCommit(
                std::move(tx),
                StoreConfig::GetConfig().FabricTimeRefreshTimeoutValue,
                [componentRoot,this,activityId](AsyncOperationSPtr const & operation) 
                    { 
                        RefreshAndPersistCompletedCallback(operation,false,false,activityId);          
                    },
                 this->CreateAsyncOperationRoot());
            //Complete Synchronously
            RefreshAndPersistCompletedCallback(operation,false,true,activityId);    
        }
    }

    void FabricTimeController::RefreshAndPersistCompletedCallback(AsyncOperationSPtr const & operation, bool setReady, bool completedSynchronously, ActivityId activityId)
    {
        if (operation->CompletedSynchronously == completedSynchronously)
        {
            ErrorCode error=StoreTransactionInternal::EndCommit(operation);

            if(error.IsSuccess())
            {
                if(setReady)
                {
                    ready_=true;
                }
            }
            else
            {
                WriteWarning(TraceComponent,"{0} Fail to read logical time in replicated store. Error:{1}, Activity:{2}",this->TraceId,error,activityId);                
            }

            if(error.IsError(ErrorCodeValue::Timeout))
            {
                TryScheduleTimer(StoreConfig::GetConfig().FabricTimeRefreshTimeoutValue);
            }
            else
            {
                TryScheduleTimer(StoreConfig::GetConfig().FabricTimePersistInterval);
            }
            
        }
    }

    Common::ErrorCode FabricTimeController::GetCurrentStoreTime(__out int64 & currentStoreTime)
    {
        ErrorCode error(ErrorCodeValue::Success);

        if(!ready_)
        {
            error=ErrorCodeValue::NotReady;
        }
        else
        {
            currentStoreTime=fabricTimeData_.RefreshAndGetCurrentStoreTime();
        }

        return error;
    }

    void FabricTimeController::TryScheduleTimer(TimeSpan delay)
    {
        if (!cancelled_.load())
        {
            refreshTimer_->Change(delay);
        }
        else
        {
            refreshTimer_->Cancel();
        }
    }

}
