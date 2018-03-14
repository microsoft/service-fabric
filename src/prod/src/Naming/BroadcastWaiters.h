// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    typedef std::map<Common::NamingUri, std::vector<Reliability::ConsistencyUnitId>> UserServiceCuidMap;

    // Callback invoked when broadcast is received
    typedef std::function<void(
        UserServiceCuidMap const &, 
        Common::AsyncOperationSPtr const &)> OnBroadcastReceivedCallback;

    class BroadcastWaiters
    {
        DENY_COPY(BroadcastWaiters)

    public:
        BroadcastWaiters(
            GatewayEventSource const & trace,
            std::wstring const & traceId);

        Common::ErrorCode Add(
            Common::AsyncOperationSPtr const & owner,
            OnBroadcastReceivedCallback const & callback,
            BroadcastRequestVector && requests);

        void Remove(
            Common::AsyncOperationSPtr const & owner,
            std::set<Common::NamingUri> && names);
                
        void Notify(UserServiceCuidMap const & updates);

        void Close();

        // Test hooks
        bool Test_Contains(
            Common::NamingUri const & name, 
            Reliability::ConsistencyUnitId const & cuids) const;
        
    private:
         
        class WaiterEntry
        {
        public:
            WaiterEntry(
                Common::AsyncOperationSPtr const & owner,
                OnBroadcastReceivedCallback const & callback,
                Reliability::ConsistencyUnitId cuid);

            __declspec(property(get=get_Owner)) Common::AsyncOperationSPtr const & Owner;
            Common::AsyncOperationSPtr const & get_Owner() const { return owner_; }

            __declspec(property(get=get_Cuid)) Reliability::ConsistencyUnitId const & Cuid;
            Reliability::ConsistencyUnitId const & get_Cuid() const { return cuid_; }

            void Cancel();
            bool ShouldNotify(
                std::vector<Reliability::ConsistencyUnitId> const & cuids,
                __out std::vector<Reliability::ConsistencyUnitId> & relevantCuids) const;
            void Notify(UserServiceCuidMap const & changedCuids);

        private:
            Common::AsyncOperationSPtr owner_;
            OnBroadcastReceivedCallback callback_;
            Reliability::ConsistencyUnitId cuid_;
        };

        typedef std::shared_ptr<WaiterEntry> WaiterEntrySPtr;
           
        void RemoveCallerHoldsLock(
            Common::AsyncOperationSPtr const & owner,
            Common::NamingUri const & name);

        GatewayEventSource const & trace_;
        std::wstring traceId_;

        // Map that keeps track of all registered waiters per service name
        typedef std::vector<std::shared_ptr<WaiterEntry>> BroadcastWaiterVector;
        typedef std::pair<Common::NamingUri, BroadcastWaiterVector> BroadcastWaiterEntry;
        std::map<Common::NamingUri, BroadcastWaiterVector> waiters_;

        bool isClosed_;
        mutable Common::ExclusiveLock lock_;
    };
}
