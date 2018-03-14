// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace HealthManager
    {
        using namespace Common;

        namespace HealthEntityState 
        {
            void WriteToTextWriter(__in Common::TextWriter & w, Enum e)
            {
                switch(e)
                {
                case PendingFirstReport: w << "PendingFirstReport"; return;
                case PendingLoadFromStore: w << "PendingLoadFromStore"; return;
                case PendingWriteToStore: w << "PendingWriteToStore"; return;
                case Ready: w << "Ready"; return;
                case Closed: w << "Closed"; return;
                default: w << "HealthEntityState(" << static_cast<uint>(e) << ')'; return;
                }
            }
          
            BEGIN_ENUM_STRUCTURED_TRACE( HealthEntityState )

            ADD_CASTED_ENUM_MAP_VALUE_RANGE( HealthEntityState, PendingFirstReport, LAST_STATE)

            END_ENUM_STRUCTURED_TRACE( HealthEntityState )
        }

        EntityState::EntityState(HealthEntityState::Enum entityState)
            : entityState_(entityState)
        {
        }

        EntityState::~EntityState()
        {
        }

        bool EntityState::CanAcceptLoadFromStore() const
        {
            return entityState_ == HealthEntityState::PendingLoadFromStore;
        }
        
        bool EntityState::CanAcceptReport() const
        {
            return entityState_ == HealthEntityState::PendingWriteToStore ||
                   entityState_ == HealthEntityState::PendingFirstReport ||
                   entityState_ == HealthEntityState::Ready;
        }

        bool EntityState::CanAcceptQuery() const
        {
            return entityState_ == HealthEntityState::PendingFirstReport ||
                   entityState_ == HealthEntityState::Ready;
        }

        void EntityState::TransitionReady()
        {
            if (entityState_ == HealthEntityState::Closed)
            {
                return;
            }

            ASSERT_IFNOT(
                entityState_ == HealthEntityState::PendingWriteToStore ||
                entityState_ == HealthEntityState::PendingFirstReport ||
                entityState_ == HealthEntityState::PendingLoadFromStore,
                "Invalid transition to ready from state {0}", 
                entityState_);

            entityState_ = HealthEntityState::Ready;
        }

        void EntityState::TransitionPendingLoadFromStore()
        {
            if (entityState_ == HealthEntityState::Closed)
            {
                return;
            }

            ASSERT_IFNOT(
                entityState_ == HealthEntityState::PendingFirstReport,
                "Invalid transition to PendingLoadFromStore from state {0}", 
                entityState_);

            entityState_ = HealthEntityState::PendingLoadFromStore;
        }

        void EntityState::TransitionPendingFirstReport()
        {
            if (entityState_ == HealthEntityState::Closed)
            {
                return;
            }

            entityState_ = HealthEntityState::PendingFirstReport;
        }

        void EntityState::TransitionClosed()
        {
            entityState_ = HealthEntityState::Closed;
        }
    }
}
