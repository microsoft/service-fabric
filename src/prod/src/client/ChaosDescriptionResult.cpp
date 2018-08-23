//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FaultAnalysisService;
using namespace Client;

ChaosDescriptionResult::ChaosDescriptionResult(
    shared_ptr<ChaosDescription> && chaosDescription)
    : chaosDescription_(move(chaosDescription))
{
}

std::shared_ptr<ChaosDescription> const & ChaosDescriptionResult::GetChaos()
{
    return chaosDescription_;
}
