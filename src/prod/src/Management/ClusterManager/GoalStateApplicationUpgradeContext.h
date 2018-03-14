// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        // Any time during application rollback, a target "goal state" upgrade can be submitted 
        // using the normal application upgrade API. Upon rollback completion, the goal state 
        // upgrade (if any) will be loaded and a new upgrade will be automatically started.
        //
        // Attempting to start a new application upgrade against a pending upgrade will trigger 
        // a rollback and submit the interrupting upgrade as the new goal state.
        //
        class GoalStateApplicationUpgradeContext : public ApplicationUpgradeContext
        {
        public:
            GoalStateApplicationUpgradeContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                ApplicationUpgradeDescription const &,
                ServiceModelVersion const &,
                std::wstring const &,
                uint64 upgradeInstance);

            // For delete
            //
            GoalStateApplicationUpgradeContext(
                Common::NamingUri const &);

            virtual std::wstring const & get_Type() const override;

        protected:
            virtual StringLiteral const & GetTraceComponent() const override;
        };
    }
}
