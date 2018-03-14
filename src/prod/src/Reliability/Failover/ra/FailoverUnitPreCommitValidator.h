// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class FailoverUnitPreCommitValidator : public Infrastructure::EntityPreCommitNotificationSink<FailoverUnit>
        {
            DENY_COPY(FailoverUnitPreCommitValidator);
        public:
            FailoverUnitPreCommitValidator() {}
            void OnPreCommit(Infrastructure::EntityEntryBaseSPtr entry, Infrastructure::CommitDescription<FailoverUnit> const & description) override;
        };
    }
}



