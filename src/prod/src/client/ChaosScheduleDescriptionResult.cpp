//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::FaultAnalysisService;
using namespace Client;

ChaosScheduleDescriptionResult::ChaosScheduleDescriptionResult(
    shared_ptr<ChaosScheduleDescription> && chaosScheduleDescription)
    : chaosScheduleDescription_(move(chaosScheduleDescription))
{
}

std::shared_ptr<ChaosScheduleDescription> const & ChaosScheduleDescriptionResult::GetChaosSchedule()
{
    return chaosScheduleDescription_;
}
