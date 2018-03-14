// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

void Reliability::NodeUpgradePhase::WriteToTextWriter(TextWriter & w, Enum const& value)
{
    switch (value)
    {
        case Invalid:
            w << L"Invalid";
            return;
        case PreUpgradeSafetyCheck:
            w << L"NotStarted";
            return;
        case Upgrading:
            w << L"InProgress";
            return;
        case PostUpgradeSafetyCheck:
            w << L"Waiting";
            return;
        default:
            w << "NodeUpgradePhase(" << static_cast<uint>(value) << ')';
            return;
    }
}
