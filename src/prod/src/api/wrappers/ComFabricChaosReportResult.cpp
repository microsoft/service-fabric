// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using Common::ComUtility;

using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

ComFabricChaosReportResult::ComFabricChaosReportResult(
    IChaosReportResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_CHAOS_REPORT * STDMETHODCALLTYPE ComFabricChaosReportResult::get_ChaosReportResult()
{
    auto report = impl_->GetChaosReport();
    auto chaosReportPtr = heap_.AddItem<FABRIC_CHAOS_REPORT>();
    report->ToPublicApi(heap_, *chaosReportPtr);

    return chaosReportPtr.GetRawPointer();
}
