// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Naming;
using namespace Api;

namespace Client
{
    ServiceAddressTracker::ServiceAddressTracker(
        __in FabricClientImpl & client,
        NamingUri const & name,
        PartitionKey const & partitionKey,
        NamingUri && requestDataName)
        : ComponentRoot()
        , lock_()
        , client_(client)
        , name_(move(name))
        , requestDataName_(requestDataName)
        , partitionKey_(partitionKey)
        , requests_()
        , previousResolves_()
        , previousError_()
        , cancelled_(false)
    {
        client_.Trace.TrackerConstructor(
            client_.TraceContext,
            name_.Name,
            requestDataName_.Name,
            partitionKey_,
            (__int64)(static_cast<void*>(this)));
    }

    ServiceAddressTracker::~ServiceAddressTracker()
    {
        client_.Trace.TrackerDestructor((__int64)(static_cast<void*>(this)));
    }

    ErrorCode ServiceAddressTracker::AddRequest(
        Api::ServicePartitionResolutionChangeCallback const& handler,
        FabricActivityHeader const & activityHeader,
        __out LONGLONG & handlerId)
    {
        handlerId = -1;
        bool raiseCallbacks = false;
        
        { // lock
            AcquireWriteLock acquire(lock_);
            if (cancelled_)
            {
                return ErrorCode(ErrorCodeValue::ObjectClosed);
            }
            
            // If there are updates, try to raise callback
            // This will check the number of pending operations inside the request
            // to see if a new thread must be started,
            // so it must be called before AddAndPostUpdate
            if (requests_.HasUpdate)
            {
                raiseCallbacks = StartRaiseCallbacksCallerHoldsLock(activityHeader);
            }

            // Add the request in the list of handlers
            requests_.AddAndPostUpdate(move(handler), /*out*/handlerId);
            client_.Trace.TrackerAddRequest(
                client_.TraceContext,
                activityHeader.ActivityId,
                handlerId, 
                *this);
        } // endlock
        
        if (raiseCallbacks)
        {
            FinishRaiseCallbacks(activityHeader);
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    bool ServiceAddressTracker::TryRemoveRequest(LONGLONG handlerId)
    {
        AcquireWriteLock acquire(lock_);
        if (requests_.TryRemove(handlerId))
        {
            client_.Trace.TrackerRemoveRequest(
                client_.TraceContext,
                handlerId, 
                *this);
            return true;
        }

        return false;
    }

    ServiceLocationNotificationRequestData ServiceAddressTracker::CreateRequestData()
    {
        AcquireReadLock acquire(lock_);
        return ServiceLocationNotificationRequestData(
            requestDataName_, 
            previousResolves_,
            previousError_,
            partitionKey_);
    }
        
    void ServiceAddressTracker::Cancel()
    {
        AcquireWriteLock acquire(lock_);
        cancelled_ = true;
        requests_.Cancel();
    }

    void ServiceAddressTracker::PostUpdate(
        ResolvedServicePartitionSPtr const & partition,
        FabricActivityHeader const & activityHeader)
    {
        ResolvedServicePartitionSPtr updatePartition;
        
        if (partition->IsServiceGroup)
        {
            if (name_.Fragment.empty())
            {
                // Raise AccessDenied
                PostInnerFailureUpdate(ErrorCodeValue::AccessDenied, partition, activityHeader);
                return;
            }
            else if (!ServiceGroup::Utility::ExtractMemberServiceLocation(partition, name_.Name, /*out*/updatePartition))
            {
                // Not relevant since service group info doesn't match
                PostInnerFailureUpdate(ErrorCodeValue::UserServiceNotFound, partition, activityHeader);
                return;    
            }
        }
        else
        {
            updatePartition = partition;
        }

        PostProcessedUpdate(updatePartition, activityHeader);
    }

    void ServiceAddressTracker::PostProcessedUpdate(
        ResolvedServicePartitionSPtr const & update,
        FabricActivityHeader const & activityHeader)
    {
        bool raiseCallbacks = false;

        { //lock
            AcquireWriteLock acquire(lock_);
            client_.Trace.TrackerUpdateRSP(
                client_.TraceContext,
                activityHeader.ActivityId, 
                requests_,
                *update);
        
            if (!cancelled_ && TryUpdate(update, activityHeader))
            {
                // A valid RSP is received, so clear the previous error
                previousError_.reset();
                raiseCallbacks = StartRaiseCallbacksCallerHoldsLock(activityHeader);
                requests_.PostUpdate(update, raiseCallbacks);
            }
        } // endlock

        if (raiseCallbacks)
        {
            FinishRaiseCallbacks(activityHeader);
        }
    }
    
    bool ServiceAddressTracker::TryUpdate(
        ResolvedServicePartitionSPtr const & update,
        FabricActivityHeader const & activityHeader)
    {
        Reliability::ConsistencyUnitId const & cuid = update->Locations.ConsistencyUnitId;
        if (IsMoreRecent(update))
        {
            previousResolves_[cuid] =
                ServiceLocationVersion(update->FMVersion, update->Generation, update->StoreVersion);
            return true;
        }
        else
        {
            client_.Trace.TrackerDuplicateUpdate(
                client_.TraceContext,
                activityHeader.ActivityId,
                name_.Name);
        }

        return false;
    }

    bool ServiceAddressTracker::IsMoreRecent(ResolvedServicePartitionSPtr const & update)
    {
        auto iter = previousResolves_.find(update->Locations.ConsistencyUnitId);

        if (iter == previousResolves_.end())
        {
            return true;
        }
        else
        {
            return update->IsMoreRecent(iter->second);
        }
    }
    
    void ServiceAddressTracker::PostInnerFailureUpdate(
        ErrorCodeValue::Enum error,
        ResolvedServicePartitionSPtr const & partition,
        FabricActivityHeader const & activityHeader)
    {
        bool raiseCallbacks = false;
        AddressDetectionFailureSPtr update = make_shared<AddressDetectionFailure>(
            requestDataName_, 
            partitionKey_, 
            error, 
            partition->StoreVersion);

        { //lock
            AcquireWriteLock acquire(lock_);
            if (cancelled_)
            {
                return;
            }
            
            client_.Trace.TrackerUpdateADF(
                client_.TraceContext,
                activityHeader.ActivityId, 
                requests_,
                *update);
        
            // Remember the last received update, 
            // to not pick up the same value from the cache
            // and to get newer information from the gateway
            TryUpdate(partition, activityHeader);
            if (TryUpdate(update, activityHeader))
            {
                raiseCallbacks = StartRaiseCallbacksCallerHoldsLock(activityHeader);
                requests_.PostUpdate(update, raiseCallbacks);
            }
        } // endlock

        if (raiseCallbacks)
        {
            FinishRaiseCallbacks(activityHeader);
        }
    }

    void ServiceAddressTracker::PostUpdate(
        AddressDetectionFailureSPtr const & update,
        FabricActivityHeader const & activityHeader)
    {
        bool raiseCallbacks = false;

        { //lock
            AcquireWriteLock acquire(lock_);
            client_.Trace.TrackerUpdateADF(
                client_.TraceContext,
                activityHeader.ActivityId, 
                requests_,
                *update);
        
            if (!cancelled_ && TryUpdate(update, activityHeader))
            {
                previousResolves_.clear();
                raiseCallbacks = StartRaiseCallbacksCallerHoldsLock(activityHeader);
                requests_.PostUpdate(update, raiseCallbacks);
            }
        } // endlock

        if (raiseCallbacks)
        {
            FinishRaiseCallbacks(activityHeader);
        }
    }

    void ServiceAddressTracker::PostError(
        ErrorCode const error,
        FabricActivityHeader const & activityHeader)
    {
        bool raiseCallbacks = false;
        
        { //lock
            AcquireWriteLock acquire(lock_);
            // Create a failure with the version of the previous error;
            // this way, the user will not be notified of the same previous error.
            AddressDetectionFailureSPtr update = make_shared<AddressDetectionFailure>(
                requestDataName_, 
                partitionKey_, 
                error.ReadValue(), 
                previousError_ ? previousError_->StoreVersion : ServiceModel::Constants::UninitializedVersion);

            client_.Trace.TrackerUpdateADF(
                client_.TraceContext,
                activityHeader.ActivityId, 
                requests_,
                *update);
        
            if (!cancelled_ && TryUpdate(update, activityHeader))
            {
                // For general failures, do not clear the previous results,
                // so callbacks will not be triggered with same values
                raiseCallbacks = StartRaiseCallbacksCallerHoldsLock(activityHeader);
                requests_.PostUpdate(update, raiseCallbacks);
            }
        } // endlock

        if (raiseCallbacks)
        {
            FinishRaiseCallbacks(activityHeader);
        }
    }

    bool ServiceAddressTracker::TryUpdate(
        AddressDetectionFailureSPtr const & update,
        FabricActivityHeader const & /*activityHeader*/)
    {
        if (!previousError_ || previousError_->IsObsolete(*(update.get())))
        {
            previousError_ = update;
            return true;
        }
         
        return false;
    }

    // Called from under the lock
    bool ServiceAddressTracker::StartRaiseCallbacksCallerHoldsLock(
        FabricActivityHeader const & activityHeader)
    {
        bool startThread = false;
        if (requests_.PendingNotificationCount == 0u)
        {
            startThread = true;
        }

        client_.Trace.TrackerRaiseCallbacks(
            client_.TraceContext,
            activityHeader.ActivityId,
            *this, 
            startThread);
            
        return startThread;
    }
    
    void ServiceAddressTracker::FinishRaiseCallbacks(FabricActivityHeader const & activityHeader)
    {
        // The callbacks shouldn't be executed on the same thread, 
        // as this is called from under ServiceAddressTrackerManager lock

        auto thisSPtr = CreateComponentRoot();
        Threadpool::Post([thisSPtr, this, activityHeader]() { this->RaiseCallbacks(activityHeader); });
    }
        
    void ServiceAddressTracker::RaiseCallbacks(FabricActivityHeader const & activityHeader)
    {
        for (;;)
        {
            ServiceAddressTrackerCallbacks::HandlerNotification notif;
            { // lock
                AcquireWriteLock acquire(lock_);
                if (!requests_.TryGetNotification(notif))
                {
                    // No more entries in neither of the registered notifications
                    break;
                }

            } // endlock

            client_.Trace.TrackerRaiseCallback(
                client_.TraceContext,
                activityHeader.ActivityId,
                notif.HandlerId);
            IResolvedServicePartitionResultPtr result;
            if (notif.resultSPtr_)
            {
                ResolvedServicePartitionResultSPtr resultSPtr = 
                    make_shared<ResolvedServicePartitionResult>(move(notif.resultSPtr_));
                result = RootedObjectPointer<IResolvedServicePartitionResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
                notif.Callback(notif.HandlerId, result, ErrorCode::Success());
            }
            else
            {
                notif.Callback(notif.HandlerId, result, notif.failureSPtr_->Error);
            }
        }
    }

    void ServiceAddressTracker::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << "Tracker(" << name_;
        w << ", pk='" << partitionKey_ << "'";
        w << ", " << requests_ << ")";
    }

    void ServiceAddressTracker::WriteToEtw(uint16 contextSequenceId) const
    {
        client_.Trace.TrackerTrace(
            contextSequenceId,
            name_,
            partitionKey_,
            requests_);
    }

    wstring ServiceAddressTracker::ToString() const
    {
        std::wstring text;
        Common::StringWriter writer(text);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return text;
    }    
}
