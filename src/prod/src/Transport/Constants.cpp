// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    using namespace Common;
    using namespace std;

    StringLiteral const Constants::TcpTrace("Tcp");
    StringLiteral const Constants::MemoryTrace("Memory");
    StringLiteral const Constants::DemuxerTrace("Demuxer");
    StringLiteral const Constants::ConfigTrace("Config");
    Global<wstring> const Constants::ClaimsTokenError = make_global<wstring>(L"ClaimsTokenError");
    Global<wstring> const Constants::ConnectionAuth = make_global<wstring>(L"ConnectionAuth");
    StringLiteral const Constants::PartitionIdString("PartitionId");
    Global<wstring> const Constants::PartitionIdWString = make_global<wstring>(L"PartitionId");
    Global<wstring> const Constants::ClaimsMessageAction = make_global<wstring>(L"Claims");
    Global<wstring> const Constants::ReconnectAction = make_global<wstring>(L"Reconnect");
}
