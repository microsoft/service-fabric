// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FMNodeRecord.h"

using namespace std;
using namespace ModelChecker;
using namespace Reliability::FailoverManagerComponent;

NodeRecord::NodeRecord(
    NodeInfo const& nodeInfo,
    bool hasAnInstance)
    : nodeInfo_(nodeInfo),
      hasAnInstance_(hasAnInstance)
{
}
