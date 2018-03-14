// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    struct Constants
    {
    public:
        static Common::StringLiteral const TcpTrace;
        static Common::StringLiteral const MemoryTrace;
        static Common::StringLiteral const DemuxerTrace;
        static Common::StringLiteral const ConfigTrace;
        static Common::Global<std::wstring> const ClaimsTokenError;
        static Common::Global<std::wstring> const ConnectionAuth;
        static Common::StringLiteral const PartitionIdString;
        static Common::Global<std::wstring> const PartitionIdWString;
        static Common::Global<std::wstring> const ClaimsMessageAction;
        static Common::Global<std::wstring> const ReconnectAction;
    };
}
