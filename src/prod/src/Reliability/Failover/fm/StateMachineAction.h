// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class StateMachineAction
        {
            DENY_COPY(StateMachineAction);

        public:
            virtual ~StateMachineAction() {}

            int PerformAction(FailoverManager & failoverManager);
            virtual int OnPerformAction(FailoverManager & failoverManager) = 0;

            virtual void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const = 0;
            virtual void WriteToEtw(uint16 contextSequenceId) const = 0;

        protected:
            StateMachineAction();
        };
    }
}
