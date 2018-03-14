// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        interface IFlushCallbackProcessor
        {
            K_SHARED_INTERFACE(IFlushCallbackProcessor)
            K_WEAKREF_INTERFACE(IFlushCallbackProcessor)

            public:
                virtual void ProcessFlushedRecordsCallback(__in LoggedRecords const & loggedRecords) noexcept = 0;
        };
    }
}
