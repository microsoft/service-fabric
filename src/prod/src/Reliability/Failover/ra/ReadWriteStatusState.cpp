// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

ReadWriteStatusState::ReadWriteStatusState() :
readStatus_(AccessStatus::NotPrimary),
writeStatus_(AccessStatus::NotPrimary)
{
}

ReadWriteStatusValue ReadWriteStatusState::get_Current() const
{
    return ReadWriteStatusValue(readStatus_, writeStatus_);
}

bool ReadWriteStatusState::TryUpdate(
    AccessStatus::Enum readStatus,
    AccessStatus::Enum writeStatus)
{
    if (readStatus_ == readStatus && writeStatus == writeStatus_)
    {
        return false;
    }

    readStatus_ = readStatus;
    writeStatus_ = writeStatus;
    return true;
}

