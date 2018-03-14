// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Client
{
    using namespace Common;
    using namespace std;
    using namespace Naming;

    ServiceAddressTrackerCallbacks::ServiceAddressTrackerCallbacks()
        : handlers_()
        , handlerList_()
        , pendingNotifications_()
        , lastUpdate_()
        , lastError_()
        , pendingNotificationIndex_(0)
    {
    }
        
    ServiceAddressTrackerCallbacks::~ServiceAddressTrackerCallbacks()
    {
    }
     
    void ServiceAddressTrackerCallbacks::AddAndPostUpdate(
        Api::ServicePartitionResolutionChangeCallback const & handler,
        __out LONGLONG & handlerId)
    {
        handlerId = InterlockedIncrement64(&Constants::CurrentNotificationHandlerId);
           
        // If a previous request received notifications, raise them to the newcomer;
        ASSERT_IF(lastUpdate_ && lastError_, "Only one of error/update should be set");
        if (lastUpdate_)
        {
            pendingNotifications_.push_back(HandlerNotification(handler,  lastUpdate_, handlerId));
        }
        else if (lastError_)
        {
            pendingNotifications_.push_back(HandlerNotification(handler,  lastError_, handlerId));
        }

        handlers_.insert(NotificationRequest(handlerId, move(handler)));
        handlerList_.push_back(HandlerIdWrapper(handlerId));
    }
        
    bool ServiceAddressTrackerCallbacks::TryRemove(
        LONGLONG handlerId)
    {
        auto it = handlers_.find(handlerId);
        if (it != handlers_.end())
        {
            // If there are pending notifications for this handler, remove them
            // Leave the entry in the vector of pending notifications, 
            // because the ServiceAddressTracker.RaiseCallbacks logic
            // expects that the vector items are not moved.
            // RaiseCallbacks will check whether the entry is valid before calling the callback.
            for (auto itPending = pendingNotifications_.begin(); itPending != pendingNotifications_.end(); ++itPending)
            {
                if (handlerId == itPending->HandlerId)
                {
                    itPending->Reset();
                }
            }

            handlers_.erase(it);
            handlerList_.erase(remove_if(handlerList_.begin(), handlerList_.end(), [handlerId](HandlerIdWrapper const & wrapper) -> bool{ return wrapper.Id == handlerId; }));
            return true;
        }
        
        return false;
    }
        
    void ServiceAddressTrackerCallbacks::PostUpdate(
        ResolvedServicePartitionSPtr const & update, 
        bool cacheUpdate)
    {
        for (auto it = handlers_.begin(); it != handlers_.end(); ++it)
        {
            pendingNotifications_.push_back(HandlerNotification(it->second, update, it->first));
        }

        if (cacheUpdate)
        {
            lastUpdate_ = update;
            lastError_.reset();
        }
    }

    void ServiceAddressTrackerCallbacks::PostUpdate(
        AddressDetectionFailureSPtr const & update, 
        bool cacheUpdate)
    {
        for (auto it = handlers_.begin(); it != handlers_.end(); ++it)
        {
            pendingNotifications_.push_back(HandlerNotification(it->second, update, it->first));
        }

        if (cacheUpdate)
        {
            lastError_ = update;
            lastUpdate_.reset();
        }
    }
    
    bool ServiceAddressTrackerCallbacks::TryGetNotification(
        __out HandlerNotification & notification)
    {
        if (pendingNotificationIndex_ > pendingNotifications_.size())
        {
            Common::Assert::CodingError(
                "{0}: TryGetNotification: index {1} is invalid", 
                *this, 
                pendingNotificationIndex_);
        }

        for (;;)
        {
            if (pendingNotificationIndex_ == pendingNotifications_.size())
            {
                pendingNotifications_.clear();
                pendingNotificationIndex_ = 0;
                return false;
            }

            // If the callback is empty, it means it was unregistered, 
            // so ignore it and search for next valid one, if exists
            if (!pendingNotifications_[pendingNotificationIndex_].IsEmpty())
            {
                // Found a valid handler, return it.
                break;
            }
                
            ++pendingNotificationIndex_;
        }

        // Replace the notification with the empty notification passed in,
        // so this notification will not be raised again
        ASSERT_IFNOT(notification.IsEmpty(), "TryGetNotification: notif should not be set");
        notification = std::move(pendingNotifications_[pendingNotificationIndex_]);
        // Move to next index to be processed
        ++pendingNotificationIndex_;
        return true;
    }
    
    void ServiceAddressTrackerCallbacks::Cancel()
    {
        handlers_.clear();
        handlerList_.clear();
        pendingNotifications_.clear();
        pendingNotificationIndex_ = 0;
    }
    
    void ServiceAddressTrackerCallbacks::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << "handlers=(" << handlerList_;        
        if (lastUpdate_)
        {
            w << " result=" << lastUpdate_;
        }

        if (lastError_)
        {
            w << " error=" << lastError_;
        }

        w << ")";
    }

    void ServiceAddressTrackerCallbacks::WriteToEtw(uint16 contextSequenceId) const
    {
        if (lastUpdate_)
        {
            FabricClientImpl::Trace.TrackerCallbacksWithRSPTrace(
                contextSequenceId,
                handlerList_,
                *lastUpdate_);
        }
        else if (lastError_)
        {
            FabricClientImpl::Trace.TrackerCallbacksWithADFTrace(
                contextSequenceId,
                handlerList_,
                *lastError_);
        }
        else
        {
            FabricClientImpl::Trace.TrackerCallbacksTrace(
                contextSequenceId,
                handlerList_);
        }
    }
    
    std::wstring ServiceAddressTrackerCallbacks::ToString() const
    {
        std::wstring text;
        Common::StringWriter writer(text);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return text;
    }

    void ServiceAddressTrackerCallbacks::HandlerIdWrapper::WriteToEtw(uint16 contextSequenceId) const
    {
        FabricClientImpl::Trace.HandlerIdTrace(contextSequenceId, Id);
    }

    void ServiceAddressTrackerCallbacks::HandlerIdWrapper::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << Id << ";";
    }
}
