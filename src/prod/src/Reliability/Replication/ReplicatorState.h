// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Class the follows the replicator states,
        // based on whether the replicator is created/opened/closed/aborted
        // and its role.
        // It is not thread safe, and it should be protected by the replicator state lock.
        // The accepted state transitions are:
        // Created                                                     Closing/Aborting -> Closed/Aborted
        //    |                                                                |
        //    ----> Opened  -------------------------------------------------->|
        //             |                                                       |
        //             -----> ChangingRole <-----------------------|           |
        //                          |    |                         |           |
        //                          |    |                         |           |
        //                          |-> Primary ------------------>|---------->|
        //                          |       |---> CheckingDataLoss |           |
        //                          |       |<----------|          |           |
        //                          |-> SecondaryActive  --------->|---------->|  
        //                          |                              |           |
        //                          |                              |           |
        //                          |-> SecondaryIdle ------------>|---------->|
        //                          |                                          |
        //                          |-> Faulted ------------------------------>|
        struct ReplicatorState
        {
            DENY_COPY(ReplicatorState)

        public:
            ReplicatorState();
            ~ReplicatorState();

            __declspec(property(get=get_Value)) int Value;
            inline int get_Value() const { return state_; }

            __declspec(property(get=get_Role)) FABRIC_REPLICA_ROLE Role;
            inline FABRIC_REPLICA_ROLE get_Role() const { return role_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;
            std::wstring ToString() const;
    
            static const int Created = 1;
            static const int Opened = 2;
            static const int ChangingRole = 4;
            static const int Primary = 8;
            static const int SecondaryActive = 16;
            static const int SecondaryIdle = 32;
            static const int CheckingDataLoss = 64;
            static const int Closing = 128;
            static const int Closed = 256;
            static const int Aborting = 512;
            static const int Aborted = 1024;
            static const int Faulted = 2048;
            static const int RoleNone = 4096;
            
        private:

            enum Action 
            { 
                None = 0,
                CreateInitialPrimary, 
                CreateInitialSecondary,  
                PromoteSecondaryToPrimary,
                DemotePrimaryToSecondary,
                PromoteIdleToActive,
                ClosePrimary,
                CloseSecondary
            };

            Common::ErrorCode TransitionToOpened();
            Common::ErrorCode TransitionToChangingRole(
                ::FABRIC_REPLICA_ROLE newRole,
                __out Action & nextAction);
            Common::ErrorCode TransitionToPrimary();
            Common::ErrorCode TransitionToSecondaryIdle();
            Common::ErrorCode TransitionToSecondaryActive();
            Common::ErrorCode TransitionToCheckingDataLoss();
            Common::ErrorCode TransitionToClosing();
            Common::ErrorCode TransitionToClosed();
            Common::ErrorCode TransitionToAborting();
            Common::ErrorCode TransitionToAborted();
            Common::ErrorCode TransitionToRoleNone();
            void TransitionToFaulted();
            
            static const int DestructEntryMask = Created | Closed | Aborted | Faulted | None;
            static const int SecondaryOperationsMask = SecondaryIdle | SecondaryActive | ChangingRole | Closing | Aborting;
                        
            int state_;
            ::FABRIC_REPLICA_ROLE role_;

            friend Replicator;
        };
    }
}
