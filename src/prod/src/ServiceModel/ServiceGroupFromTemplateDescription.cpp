// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ServiceGroupFromTemplateDescription");

ServiceGroupFromTemplateDescription::ServiceGroupFromTemplateDescription()
: applicationName_()
, serviceName_()
, serviceTypeName_()
, servicePackageActivationMode_(ServicePackageActivationMode::SharedProcess)
, initializationData_()
{
}

ServiceGroupFromTemplateDescription::ServiceGroupFromTemplateDescription(
    NamingUri const & applicationName,
    NamingUri const & serviceName,
    wstring const & serviceTypeName,
    ServicePackageActivationMode::Enum servicePackageActivationMode,
    vector<byte> const & initializationData)
    : applicationName_(applicationName)
    , serviceName_(serviceName)
    , serviceTypeName_(serviceTypeName)
    , servicePackageActivationMode_(servicePackageActivationMode)
    , initializationData_(initializationData)
{
}

ServiceGroupFromTemplateDescription::ServiceGroupFromTemplateDescription(ServiceGroupFromTemplateDescription const & other)
: applicationName_(other.applicationName_)
, serviceName_(other.serviceName_)
, serviceTypeName_(other.serviceTypeName_)
, servicePackageActivationMode_(other.servicePackageActivationMode_)
, initializationData_(other.initializationData_)
{

}

ServiceGroupFromTemplateDescription ServiceGroupFromTemplateDescription::operator=(ServiceGroupFromTemplateDescription const & other)
{
    if (this != &other)
    {
        applicationName_ = other.applicationName_;
        serviceName_ = other.serviceName_;
        serviceTypeName_ = other.serviceTypeName_;
        servicePackageActivationMode_ = other.servicePackageActivationMode_;
        initializationData_ = other.initializationData_;
    }

    return *this;
}

ErrorCode ServiceGroupFromTemplateDescription::FromPublicApi(FABRIC_SERVICE_GROUP_FROM_TEMPLATE_DESCRIPTION const & svcGrpFromTemplateDesc)
{
    return this->FromPublicApiInternal(
        svcGrpFromTemplateDesc.ApplicationName,
        svcGrpFromTemplateDesc.ServiceName,
        svcGrpFromTemplateDesc.ServiceTypeName,
        svcGrpFromTemplateDesc.ServicePackageActivationMode,
        svcGrpFromTemplateDesc.InitializationDataSize,
        svcGrpFromTemplateDesc.InitializationData);
}

ErrorCode ServiceGroupFromTemplateDescription::FromPublicApi(
    FABRIC_URI applicationName,
    FABRIC_URI serviceName,
    LPCWSTR serviceTypeName,
    ULONG initializationDataSize,
    BYTE *initializationData)
{
    return this->FromPublicApiInternal(
        applicationName,
        serviceName,
        serviceTypeName,
        FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_SHARED_PROCESS,
        initializationDataSize,
        initializationData);
}

ErrorCode ServiceGroupFromTemplateDescription::FromPublicApiInternal(
    FABRIC_URI applicationName,
    FABRIC_URI serviceName,
    LPCWSTR serviceTypeName,
    FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE servicePackageActivationMode,
    ULONG initializationDataSize,
    BYTE *initializationData)
{
    auto error = NamingUri::TryParse(applicationName, "ApplicationName", applicationName_);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceSource, "Failed to parse ApplicationName in FromPublicAPI");
        return error;
    }

    error = NamingUri::TryParse(serviceName, "ServiceName", serviceName_);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceSource, "Failed to parse serviceName in FromPublicAPI");
        return error;
    }

    auto hr = StringUtility::LpcwstrToWstring(serviceTypeName, false, serviceTypeName_);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceSource, "Error parsing ServiceTypeName in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    error = ServicePackageActivationMode::FromPublicApi(servicePackageActivationMode, servicePackageActivationMode_);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceSource, "Error parsing ServicePackageActivationMode in FromPublicAPI: {0}", error);
        return error;
    }

    initializationData_ = vector<byte>(initializationData, initializationData + initializationDataSize);

    return error;
}

void ServiceGroupFromTemplateDescription::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write(
        "ServiceGroupFromTemplateDescription(ApplicationName={0}, ServiceName={1}, ServiceTypeName={2}, ServicePackageActivationMode={3})",
        applicationName_,
        serviceName_,
        serviceTypeName_,
        servicePackageActivationMode_);
}
