// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

void Reliability::SafetyCheckKind::WriteToTextWriter(TextWriter & w, Enum const& value)
{
    switch (value)
    {
        case Invalid:
            w << L"Invalid";
            return;
        case EnsureSeedNodeQuorum:
            w << L"EnsureSeedNodeQuorum";
            return;
        case EnsurePartitionQuorum:
            w << L"EnsurePartitionQuorum";
            return;
        case WaitForPrimaryPlacement:
            w << L"WaitForPrimaryPlacement";
            return;
        case WaitForPrimarySwap:
            w << L"WaitForPrimarySwap";
            return;
        case WaitForReconfiguration:
            w << L"WaitForReconfiguration";
            return;
        case WaitForInBuildReplica:
            w << L"WaitForInBuildReplica";
            return;
        case EnsureAvailability:
            w << L"EnsureAvailability";
            return;
        default:
            w << "SafetyCheckKind(" << static_cast<uint>(value) << ')';
            return;
    }
}
