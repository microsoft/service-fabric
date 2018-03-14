// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class SaveLookupVersionAction : public StateMachineAction
        {
            DENY_COPY(SaveLookupVersionAction);

        public:
            SaveLookupVersionAction(int64 lookupVersion_);

            int OnPerformAction(FailoverManager& failoverManager);

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            int64 lookupVersion_;

        };
    }
}
