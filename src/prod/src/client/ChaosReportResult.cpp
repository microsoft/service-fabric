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

ChaosReportResult::ChaosReportResult(
    shared_ptr<ChaosReport> && report)
    : chaosReport_(move(report))
{
}

shared_ptr<ChaosReport> const & ChaosReportResult::GetChaosReport()
{
    return chaosReport_;
}
