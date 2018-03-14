// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // LocalFailoverUnitProxyMap; used by RA in each service host process to map fuid->FailoverUnit
        class LocalFailoverUnitProxyMap: 
            public Common::RootedObject
        {
            DENY_COPY(LocalFailoverUnitProxyMap);

        public:
            LocalFailoverUnitProxyMap(
                Common::ComponentRoot const & root) 
                :failoverUnitProxies_(),
                 removedFailoverUnitProxies_(),
                 registeredFailoverUnitProxiesForCleanupEvent_(),
                 lock_(),
                 closed_(false),     
                 Common::RootedObject(root)
            {
                // Empty
            }

            FailoverUnitProxySPtr GetOrCreateFailoverUnitProxy(
                ReconfigurationAgentProxy & reconfigurationAgentProxy,
                Reliability::FailoverUnitDescription const & ftDesc,
                std::wstring const & runtimeId,
                bool createFlag,
                bool & wasCreated);

            void Open(ReconfigurationAgentProxyId const & id);

            FailoverUnitProxySPtr GetFailoverUnitProxy(FailoverUnitId const & failoverUnitId) const;

            // Gives the LFUPM contents to the caller to manage, the LFUPM is then empty
            // It is guaranteed no other caller can acquire a lock on an entry after this
            std::map<FailoverUnitId, FailoverUnitProxySPtr> && PrivatizeLFUPM();

            void AddFailoverUnitProxy(FailoverUnitProxySPtr &fuProxy);

            void RemoveFailoverUnitProxy(FailoverUnitId const & failoverUnitId);

            void RegisterFailoverUnitProxyForCleanupEvent(FailoverUnitId const & failoverUnitId);
            void UnregisterFailoverUnitProxyForCleanupEvent(FailoverUnitId const & failoverUnitId);

            void SetCleanupTimerCallerHoldsLock();
            void UpdateCleanupTimer();

            void CleanupCallback();
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

            std::vector<Reliability::FailoverUnitId> GetAllIds() const;

        private:
            std::map<FailoverUnitId, FailoverUnitProxySPtr> failoverUnitProxies_;
            std::set<FailoverUnitId> removedFailoverUnitProxies_;
            std::vector<FailoverUnitId> registeredFailoverUnitProxiesForCleanupEvent_;
            
            mutable Common::RwLock lock_;
            bool closed_;
            Common::TimerSPtr cleanupTimerSPtr_;

            ReconfigurationAgentProxyId id_;

            void AddFailoverUnitProxyCallerHoldsLock(FailoverUnitProxySPtr &&fuProxy);
        };
    }
}
