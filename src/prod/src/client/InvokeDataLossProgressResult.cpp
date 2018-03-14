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

InvokeDataLossProgressResult::InvokeDataLossProgressResult(
    shared_ptr<InvokeDataLossProgress> && progress)
    : invokeDataLossProgress_(move(progress))
{
}

shared_ptr<InvokeDataLossProgress> const & InvokeDataLossProgressResult::GetProgress()
{
    return invokeDataLossProgress_;
}
