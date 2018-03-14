// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        namespace HealthEntityState 
        {
            enum Enum 
            { 
                // New entity created by a child, not yet received any report
                // Not persisted to store.
                PendingFirstReport = 0, 

                // Created by previous primary, needs to load events from store
                // before accepting requests
                PendingLoadFromStore = 1, 

                // New entity created by a received report, which is not yet persisted to store.
                PendingWriteToStore = 2, 

                // All events are loaded in memory and the entity is ready to accept requests
                Ready = 3,

                // Closed because the replica is closing down.
                // No new requests are accepted and pending ones can be cleared.
                Closed = 4,

                LAST_STATE = Closed,
            };
            
            void WriteToTextWriter(__in Common::TextWriter & w, Enum e);

            DECLARE_ENUM_STRUCTURED_TRACE( HealthEntityState )
        }

        class EntityState
        {
        public:
            EntityState(HealthEntityState::Enum entityState);
            ~EntityState();

            __declspec (property(get=get_State)) HealthEntityState::Enum State;
            HealthEntityState::Enum get_State() const { return entityState_; }

            __declspec (property(get=get_IsPendingWriteToStore)) bool IsPendingWriteToStore;
            bool get_IsPendingWriteToStore() const { return entityState_ == HealthEntityState::PendingWriteToStore; }

            __declspec (property(get=get_IsPendingFirstReport)) bool IsPendingFirstReport;
            bool get_IsPendingFirstReport() const { return entityState_ == HealthEntityState::PendingFirstReport; }

            __declspec (property(get=get_IsPendingLoadFromStore)) bool IsPendingLoadFromStore;
            bool get_IsPendingLoadFromStore() const { return entityState_ == HealthEntityState::PendingLoadFromStore; }

            __declspec (property(get=get_IsReady)) bool IsReady;
            bool get_IsReady() const { return entityState_ == HealthEntityState::Ready; }

            __declspec (property(get=get_IsClosed)) bool IsClosed;
            bool get_IsClosed() const { return entityState_ == HealthEntityState::Closed; }

            __declspec (property(get=get_IsInStore)) bool IsInStore;
            bool get_IsInStore() const { return entityState_ == HealthEntityState::PendingLoadFromStore || entityState_ == HealthEntityState::Ready; }

            bool CanAcceptLoadFromStore() const;

            bool CanAcceptReport() const;

            bool CanAcceptQuery() const;

            void TransitionReady();

            void TransitionPendingFirstReport();

            void TransitionPendingLoadFromStore();

            void TransitionClosed();

        private:
            HealthEntityState::Enum entityState_;
        };
    }
}
