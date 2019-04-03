// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // Class the holds callbacks registered for notifications.
    // It is not thread safe, must be protected by the caller.
    
    class ServiceAddressTrackerCallbacks
    {
        DENY_COPY(ServiceAddressTrackerCallbacks);

    public:
        ServiceAddressTrackerCallbacks();

        ~ServiceAddressTrackerCallbacks();

        __declspec(property(get=get_HasUpdate)) bool HasUpdate;
        inline bool get_HasUpdate() const { return lastUpdate_ != nullptr || lastError_ != nullptr; }

        __declspec(property(get=get_RequestCount)) size_t RequestCount;
        inline size_t get_RequestCount() const { return handlers_.size(); }

        __declspec(property(get=get_PendingNotificationCount)) size_t PendingNotificationCount;
        inline size_t get_PendingNotificationCount() const { return pendingNotifications_.size(); }

        void AddAndPostUpdate(
            Api::ServicePartitionResolutionChangeCallback const& handler,
            __out LONGLONG & handlerId);

        bool TryRemove(LONGLONG handlerId);

        void PostUpdate(Naming::ResolvedServicePartitionSPtr const & update, bool cacheUpdate);
        void PostUpdate(Naming::AddressDetectionFailureSPtr const & update, bool cacheUpdate);

        struct HandlerIdWrapper
        {
        public:
            explicit HandlerIdWrapper(LONGLONG id) : Id(id) {}
            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;
            LONGLONG Id;
        };

        struct HandlerNotification
        {
        public:
            // empty notification
            HandlerNotification() 
                : Callback()
                , resultSPtr_()
                , failureSPtr_()
                , handlerId_(-1)
            {}

            HandlerNotification(
                Api::ServicePartitionResolutionChangeCallback const & callback,
                Naming::ResolvedServicePartitionSPtr const & result, 
                LONGLONG handlerId)
                : Callback(callback)
                , resultSPtr_(result)
                , handlerId_(handlerId)
            {
                ASSERT_IF(handlerId < 0, "HandlerNotification: handlerId is invalid {0}", handlerId);
            }

            HandlerNotification(
                Api::ServicePartitionResolutionChangeCallback const & callback,
                Naming::AddressDetectionFailureSPtr const & error, 
                LONGLONG handlerId)
                : Callback(callback)
                , failureSPtr_(error)
                , handlerId_(handlerId)
            {
                ASSERT_IF(handlerId < 0, "HandlerNotification: handlerId is invalid {0}", handlerId);
            }
            
            HandlerNotification(HandlerNotification && other)
                : Callback(std::move(other.Callback))
                , resultSPtr_(std::move(other.resultSPtr_))
                , failureSPtr_(std::move(other.failureSPtr_))
                , handlerId_(other.handlerId_)
            {
            }
            
            HandlerNotification & operator = (HandlerNotification && other)
            {
                if (this != &other)
                {
                    Callback = std::move(other.Callback);
                    resultSPtr_ = std::move(other.resultSPtr_);
                    failureSPtr_ = std::move(other.failureSPtr_);
                    handlerId_ = other.handlerId_;
                }
                return *this;
            }
                                   
            bool IsEmpty() const { return handlerId_ == -1; }

            void Reset()
            {
                Callback = nullptr;
                resultSPtr_.reset();
                failureSPtr_.reset();
                handlerId_ = -1;
            }

            __declspec(property(get=get_HandlerId)) LONGLONG HandlerId;
            LONGLONG get_HandlerId() { return handlerId_; }

            Api::ServicePartitionResolutionChangeCallback Callback;
            Naming::ResolvedServicePartitionSPtr resultSPtr_;
            Naming::AddressDetectionFailureSPtr failureSPtr_;
            LONGLONG handlerId_;
        };

        bool TryGetNotification(__out HandlerNotification & notification);

        void Cancel();

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        void WriteToEtw(uint16 contextSequenceId) const;

    private:
        
        // Map that keeps track of the address change handler for each request, 
        // by the handlerId
        typedef std::pair<LONGLONG, Api::ServicePartitionResolutionChangeCallback> NotificationRequest;
        typedef std::map<LONGLONG, Api::ServicePartitionResolutionChangeCallback> NotificationRequestMap;
        NotificationRequestMap handlers_;
        // Keep the handlers in a duplicate list for easy tracing
        std::vector<HandlerIdWrapper> handlerList_;

        // Notifications that must be raised to the registered handlers
        std::vector<HandlerNotification> pendingNotifications_;
            
        // Latest update, which will be immediately raised when adding new trackers 
        // TODO: 847196, support notifications for all partitions:
        // keep a vector when registering for *all* partition updates
        Naming::ResolvedServicePartitionSPtr lastUpdate_;
        Naming::AddressDetectionFailureSPtr lastError_;

        // The index of current pending notification that should be raised
        size_t pendingNotificationIndex_;
    };
}
