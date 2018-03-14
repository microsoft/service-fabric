// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        interface ILogManager
            : public LogRecordLib::ILogManagerReadOnly
        {
            K_SHARED_INTERFACE(ILogManager);

        public:

            virtual ktl::Awaitable<void> FlushAsync(
                __in KStringView const & initiator) = 0;

        };
    }
}
