// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ApplicationEntity");

INITIALIZE_SIZE_ESTIMATION(ApplicationEntityHealthInformation)

ApplicationEntityHealthInformation::ApplicationEntityHealthInformation()
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_APPLICATION)
    , applicationName_()
    , applicationInstanceId_(FABRIC_INVALID_INSTANCE_ID)
{
}

ApplicationEntityHealthInformation::ApplicationEntityHealthInformation(
    std::wstring const & applicationName,
    FABRIC_INSTANCE_ID applicationInstanceId)
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_APPLICATION)
    , applicationName_(applicationName)
    , applicationInstanceId_(applicationInstanceId)
{
}

ErrorCode ApplicationEntityHealthInformation::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    auto applicationHealthReport = heap.AddItem<FABRIC_APPLICATION_HEALTH_REPORT>();

    applicationHealthReport->ApplicationName = heap.AddString(applicationName_);
    applicationHealthReport->HealthInformation = commonHealthInformation;

    healthReport.Kind = kind_;
    healthReport.Value = applicationHealthReport.GetRawPointer();
    return ErrorCode::Success();
}

ErrorCode ApplicationEntityHealthInformation::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    __inout HealthInformation & commonHealthInformation,
    __out AttributeList & attributes)
{
    UNREFERENCED_PARAMETER(attributes);

    kind_ = healthReport.Kind;

    auto applicationHealthReport = reinterpret_cast<FABRIC_APPLICATION_HEALTH_REPORT *>(healthReport.Value);

    auto hr = StringUtility::LpcwstrToWstring(applicationHealthReport->ApplicationName, false /* acceptNull */, applicationName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    return commonHealthInformation.FromCommonPublicApi(*applicationHealthReport->HealthInformation);
}
