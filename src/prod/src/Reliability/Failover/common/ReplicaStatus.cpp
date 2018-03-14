// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability::ReplicaStatus;

void Reliability::ReplicaStatus::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case Invalid: 
            w << L"I"; return;
        case Down: 
            w << L"D"; return;
        case Up: 
            w << L"U"; return;
        default: 
            Common::Assert::CodingError("Unknown Replica Status");
    }
}

::FABRIC_REPLICA_STATUS Reliability::ReplicaStatus::ConvertToPublicReplicaStatus(ReplicaStatus::Enum toConvert)
{
    switch(toConvert)
    {
    case Invalid:
        return ::FABRIC_REPLICA_STATUS_INVALID;
    case Down:
        return ::FABRIC_REPLICA_STATUS_DOWN;
    case Up:
        return ::FABRIC_REPLICA_STATUS_UP;
    default:
        Common::Assert::CodingError("Unknown Replica Status");
    }
}
