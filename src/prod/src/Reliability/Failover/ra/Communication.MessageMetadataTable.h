// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Communication
        {
            class MessageMetadataTable
            {
            public:
                MessageMetadata const * LookupMetadata(Transport::Message const & message) const;

                MessageMetadata const * LookupMetadata(Transport::MessageUPtr const & message) const;

                MessageMetadata const * LookupMetadata(std::wstring const & action) const;

            private:
                static Common::Global<MessageMetadata> Deprecated;

                static Common::Global<MessageMetadata> RA_Normal;
                static Common::Global<MessageMetadata> RA_ProcessDuringNodeCloseMessages;

                static Common::Global<MessageMetadata> FT_Failover_Normal;
                static Common::Global<MessageMetadata> FT_Failover_Normal_CreatesEntity;
                static Common::Global<MessageMetadata> FT_Failover_ProcessDuringNodeCloseMessages;

                static Common::Global<MessageMetadata> FT_Proxy_Normal;
                static Common::Global<MessageMetadata> FT_Proxy_ProcessDuringNodeCloseMessages;
            };
        }
    }
}
