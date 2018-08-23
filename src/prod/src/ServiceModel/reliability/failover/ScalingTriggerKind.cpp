// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ScalingTriggerKind.h"

using namespace std;
using namespace Common;
using namespace Reliability;

void ScalingTriggerKind::WriteToTextWriter(__in TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case ScalingTriggerKind::Invalid:
        w << L"Invalid";
        return;
    case ScalingTriggerKind::AveragePartitionLoad:
        w << L"AveragePartitionLoad";
        return;
    case ScalingTriggerKind::AverageServiceLoad:
        w << L"AverageServiceLoad";
        return;
    }
}