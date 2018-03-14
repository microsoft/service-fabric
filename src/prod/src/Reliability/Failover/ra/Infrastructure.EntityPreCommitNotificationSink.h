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
            template <typename T>
            class EntityPreCommitNotificationSink
            {
                DENY_COPY(EntityPreCommitNotificationSink);

            public:
                EntityPreCommitNotificationSink()
                {
                }

                virtual ~EntityPreCommitNotificationSink()
                {
                }

                virtual void OnPreCommit(EntityEntryBaseSPtr entry, CommitDescription<T> const & description)
                {
                    UNREFERENCED_PARAMETER(entry);
                    UNREFERENCED_PARAMETER(description);
                }
            };
        }
    }
}


