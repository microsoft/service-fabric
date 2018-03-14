// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        template <class T>
        class FailoverUnitMessageTask : DynamicStateMachineTask
        {
            DENY_COPY(FailoverUnitMessageTask);

        public:
            FailoverUnitMessageTask(
                std::wstring const & action,
                std::unique_ptr<T> && body,
                FailoverManager & failoverManager,
                Federation::NodeInstance const & from);

            FailoverUnitId const & GetFailoverUnitId() const;

            void CheckFailoverUnit(
                LockedFailoverUnitPtr & lockedFailoverUnit,
                std::vector<StateMachineActionUPtr> & actions);

            static void ProcessRequest(
                Transport::Message & request,
                FailoverManager & failoverManager,
                Federation::NodeInstance const & from);

            static void ProcessRequest(
                std::wstring const& action,
                std::unique_ptr<T> && body,
                FailoverManager & failoverManager,
                Federation::NodeInstance const & from);

        private:
            bool IsStaleRequest(LockedFailoverUnitPtr const& failoverUnit);

            FailoverManager & failoverManager_;
            std::wstring action_;
            std::unique_ptr<T> body_;
            Federation::NodeInstance from_;
        };
    }
}
