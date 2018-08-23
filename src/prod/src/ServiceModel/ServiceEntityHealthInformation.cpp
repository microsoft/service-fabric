// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ServiceEntity");

INITIALIZE_SIZE_ESTIMATION(ServiceEntityHealthInformation)

ServiceEntityHealthInformation::ServiceEntityHealthInformation()
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_SERVICE)
    , serviceName_()
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
{
}

ServiceEntityHealthInformation::ServiceEntityHealthInformation(
    std::wstring const & serviceName,
    FABRIC_INSTANCE_ID instanceId)
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_SERVICE)
    , serviceName_(serviceName)
    , instanceId_(instanceId)
{
}

ErrorCode ServiceEntityHealthInformation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    auto serviceHealthReport = heap.AddItem<FABRIC_SERVICE_HEALTH_REPORT>();

    serviceHealthReport->ServiceName = heap.AddString(serviceName_);
    serviceHealthReport->HealthInformation = commonHealthInformation;

    healthReport.Kind = kind_;
    healthReport.Value = serviceHealthReport.GetRawPointer();
    return ErrorCode::Success();
}

ErrorCode ServiceEntityHealthInformation::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    __inout HealthInformation & commonHealthInformation,
    __out AttributeList & attributes)
{
    UNREFERENCED_PARAMETER(attributes);

    kind_ = healthReport.Kind;

    auto serviceHealthReport = reinterpret_cast<FABRIC_SERVICE_HEALTH_REPORT *>(healthReport.Value);

    auto hr = StringUtility::LpcwstrToWstring(serviceHealthReport->ServiceName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, serviceName_);
    if (!SUCCEEDED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }

    instanceId_ = FABRIC_INVALID_INSTANCE_ID;
    return commonHealthInformation.FromCommonPublicApi(*serviceHealthReport->HealthInformation);
}
