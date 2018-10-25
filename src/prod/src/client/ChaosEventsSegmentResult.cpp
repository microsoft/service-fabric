//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::FaultAnalysisService;
using namespace Client;

ChaosEventsSegmentResult::ChaosEventsSegmentResult(
    shared_ptr<ChaosEventsSegment> && chaosEventsSegment)
    : chaosEventsSegment_(move(chaosEventsSegment))
{
}

std::shared_ptr<ChaosEventsSegment> const & ChaosEventsSegmentResult::GetChaosEvents()
{
    return chaosEventsSegment_;
}
