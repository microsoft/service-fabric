// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class DeleteServiceAction : public StateMachineAction
        {
            DENY_COPY(DeleteServiceAction);

        public:

            DeleteServiceAction(ServiceInfoSPtr const& serviceInfo, FailoverUnitId const& failoverUnitId);

            int OnPerformAction(FailoverManager & fm);

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:

            ServiceInfoSPtr serviceInfo_;
            FailoverUnitId failoverUnitId_;
        };
    }
}
