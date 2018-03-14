// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            class LocalFailoverUnitMap : public EntityMap<FailoverUnit>
            {
                DENY_COPY(LocalFailoverUnitMap);

            public:
                LocalFailoverUnitMap(
                    ReconfigurationAgent & ra,
                    Infrastructure::IClock & clock,
                    Diagnostics::RAPerformanceCounters & perfCounters,
                    EntityPreCommitNotificationSinkSPtrType const & preCommitNotificationSink) :
                    EntityMap(ra, clock, perfCounters, preCommitNotificationSink)
                {
                }

                Infrastructure::EntityEntryBaseList GetAllFailoverUnitEntries(bool excludeFM) const;

                // Returns all the FM entries (in future there may be multiple)
                Infrastructure::EntityEntryBaseList GetFMFailoverUnitEntries() const;

                // Returns all entity entries owned by the specified fm
                Infrastructure::EntityEntryBaseList GetFailoverUnitEntries(
                    FailoverManagerId const & owner) const;

            };
        }
    }
}
