// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ServiceFromTemplateDescription");

ServiceFromTemplateDescription::ServiceFromTemplateDescription()
    : applicationName_()
    , serviceName_()
	, serviceDnsName_()
    , serviceTypeName_()
    , servicePackageActivationMode_(ServicePackageActivationMode::SharedProcess)
    , initializationData_()
{
}

ServiceFromTemplateDescription::ServiceFromTemplateDescription(
    NamingUri const & applicationName,
    NamingUri const & serviceName,
	wstring const & serviceDnsName,
    wstring const & serviceTypeName,
    ServicePackageActivationMode::Enum servicePackageActivationMode,
    vector<byte> const & initializationData)
    : applicationName_(applicationName)
    , serviceName_(serviceName)
	, serviceDnsName_(serviceDnsName)
    , serviceTypeName_(serviceTypeName)
    , servicePackageActivationMode_(servicePackageActivationMode)
    , initializationData_(initializationData)
{
    StringUtility::ToLower(serviceDnsName_);
}

ServiceFromTemplateDescription::ServiceFromTemplateDescription(
    ServiceGroupFromTemplateDescription const & svcGroupFromTemplateDesc)
    : applicationName_(svcGroupFromTemplateDesc.ApplicationName)
    , serviceName_(svcGroupFromTemplateDesc.ServiceName)
	, serviceDnsName_()
    , serviceTypeName_(svcGroupFromTemplateDesc.ServiceTypeName)
    , servicePackageActivationMode_(svcGroupFromTemplateDesc.ServicePackageActivationMode)
    , initializationData_(svcGroupFromTemplateDesc.InitializationData)
{
}

ServiceFromTemplateDescription::ServiceFromTemplateDescription(ServiceFromTemplateDescription const & other)
    : applicationName_(other.applicationName_)
    , serviceName_(other.serviceName_)
	, serviceDnsName_(other.serviceDnsName_)
    , serviceTypeName_(other.serviceTypeName_)
    , servicePackageActivationMode_(other.servicePackageActivationMode_)
    , initializationData_(other.initializationData_)
{

}

ServiceFromTemplateDescription ServiceFromTemplateDescription::operator=(ServiceFromTemplateDescription const & other)
{
    if (this != &other)
    {
        applicationName_ = other.applicationName_;
        serviceName_ = other.serviceName_;
		serviceDnsName_ = other.serviceDnsName_;
        serviceTypeName_ = other.serviceTypeName_;
        servicePackageActivationMode_ = other.servicePackageActivationMode_;
        initializationData_ = other.initializationData_;
    }

    return *this;
}

ErrorCode ServiceFromTemplateDescription::FromPublicApi(FABRIC_SERVICE_FROM_TEMPLATE_DESCRIPTION const & svcFromTemplateDesc)
{
    return this->FromPublicApiInternal(
        svcFromTemplateDesc.ApplicationName,
        svcFromTemplateDesc.ServiceName,
		svcFromTemplateDesc.ServiceDnsName,
        svcFromTemplateDesc.ServiceTypeName,
        svcFromTemplateDesc.ServicePackageActivationMode,
        svcFromTemplateDesc.InitializationDataSize,
        svcFromTemplateDesc.InitializationData);
}

Common::ErrorCode ServiceFromTemplateDescription::FromPublicApi(
    FABRIC_URI applicationName,
    FABRIC_URI serviceName,
    LPCWSTR serviceTypeName,
    ULONG initializationDataSize,
    BYTE *initializationData)
{
    return this->FromPublicApiInternal(
        applicationName,
        serviceName,
		L"",
        serviceTypeName,
        FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_SHARED_PROCESS,
        initializationDataSize,
        initializationData);
}

ErrorCode ServiceFromTemplateDescription::FromPublicApiInternal(
    FABRIC_URI applicationName, 
    FABRIC_URI serviceName,
	LPCWSTR serviceDnsName,
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

    auto hr = StringUtility::LpcwstrToWstring(serviceDnsName, true, serviceDnsName_);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceSource, "Error parsing ServiceDnsName in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    StringUtility::ToLower(serviceDnsName_);

    hr = StringUtility::LpcwstrToWstring(serviceTypeName, false, serviceTypeName_);
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

void ServiceFromTemplateDescription::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write(
        "ServiceFromTemplateDescription(ApplicationName={0}, ServiceName={1}, ServiceDnsName={2}, ServiceTypeName={3}, ServicePackageActivationMode={4})",
        applicationName_,
        serviceName_,
        serviceDnsName_,
        serviceTypeName_,
        servicePackageActivationMode_);
}
