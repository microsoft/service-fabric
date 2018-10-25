// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ScalingMechanismKind.h"

using namespace std;
using namespace Common;
using namespace Reliability;

void ScalingMechanismKind::WriteToTextWriter(__in TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case ScalingMechanismKind::Invalid:
        w << L"Invalid";
        return;
    case ScalingMechanismKind::PartitionInstanceCount:
        w << L"PartitionInstanceCount";
        return;
    case ScalingMechanismKind::AddRemoveIncrementalNamedPartition:
        w << L"AddRemoveIncrementalNamedPartition";
        return;
    }
}
