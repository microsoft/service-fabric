// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Management::FaultAnalysisService;
using namespace Client;

RestartPartitionProgressResult::RestartPartitionProgressResult(
    shared_ptr<RestartPartitionProgress> && progress)
    : restartPartitionProgress_(move(progress))
{
}

shared_ptr<RestartPartitionProgress> const & RestartPartitionProgressResult::GetProgress()
{
    return restartPartitionProgress_;
}
