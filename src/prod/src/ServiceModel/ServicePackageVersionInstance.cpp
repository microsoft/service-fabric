// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

GlobalWString ServicePackageVersionInstance::Delimiter = make_global<wstring>(L":");
GlobalWString ServicePackageVersionInstance::EnvVarName_ServicePackageVersionInstance = make_global<wstring>(L"Fabric_ServicePackageVersionInstance");

const ServicePackageVersionInstance ServicePackageVersionInstance::Invalid(ServicePackageVersion::Invalid, 0);

ServicePackageVersionInstance::ServicePackageVersionInstance()
    : version_(),
    instanceId_()
{
}

ServicePackageVersionInstance::ServicePackageVersionInstance(
    ServicePackageVersion const & version,
    uint64 instanceId)
    : version_(version),
    instanceId_(instanceId)
{
}

ServicePackageVersionInstance::ServicePackageVersionInstance(
    ServicePackageVersionInstance const & other)
    : version_(other.version_),
    instanceId_(other.instanceId_)
{
}

ServicePackageVersionInstance::ServicePackageVersionInstance(
    ServicePackageVersionInstance && other)
    : version_(move(other.version_)),
    instanceId_(other.instanceId_)
{
}

ServicePackageVersionInstance::~ServicePackageVersionInstance() 
{ 
}

ServicePackageVersionInstance const & ServicePackageVersionInstance::operator = (ServicePackageVersionInstance const & other)
{
    if (this != &other)
    {
        this->version_ = other.version_;
        this->instanceId_ = other.instanceId_;
    }

    return *this;
}

ServicePackageVersionInstance const & ServicePackageVersionInstance::operator = (ServicePackageVersionInstance && other)
{
    if (this != &other)
    {
        this->version_ = move(other.version_);
        this->instanceId_ = other.instanceId_;
    }

    return *this;
}

int ServicePackageVersionInstance::compare(ServicePackageVersionInstance const & other) const
{
    int comparision = 0;
    
    comparision = this->version_.compare(other.version_);
    if (comparision != 0) { return comparision; }

    if (this->instanceId_ > other.instanceId_)
    {
        comparision = 1;
    }
    else if (this->instanceId_ < other.instanceId_)
    {
        comparision = -1;
    }
    else 
    {
        comparision = 0;
    }

    return comparision;
}

bool ServicePackageVersionInstance::operator < (ServicePackageVersionInstance const & other) const
{
    return (this->compare(other) < 0);
}

bool ServicePackageVersionInstance::operator == (ServicePackageVersionInstance const & other) const
{
    return ((this->version_ == other.version_) && (this->instanceId_ == other.instanceId_));
}

bool ServicePackageVersionInstance::operator != (ServicePackageVersionInstance const & other) const
{
    return !(*this == other);
}

void ServicePackageVersionInstance::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ServicePackageVersionInstance::ToString() const
{
    wstring result;
    StringWriter writer(result);
    writer.Write(
        "{0}{1}{2}{3}{4}", 
        this->version_.ApplicationVersionValue.ToString(), 
        Delimiter, 
        this->version_.RolloutVersionValue.ToString(), 
        Delimiter, 
        this->instanceId_);
    writer.Flush();

    return result;
}

ErrorCode ServicePackageVersionInstance::FromString(
    wstring const & servicePackageVersionInstanceString, 
    __out ServicePackageVersionInstance & servicePackageVersionInstance)
{
    vector<wstring> tokens;  
    StringUtility::Split<wstring>(servicePackageVersionInstanceString, tokens, Delimiter);

    if (tokens.size() != 3)
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    ApplicationVersion applicationVersion;
    RolloutVersion packageVersion;
    auto error = ApplicationVersion::FromString(tokens[0], applicationVersion);
    if (!error.IsSuccess()) { return error; }

    error = RolloutVersion::FromString(tokens[1], packageVersion);
    if (!error.IsSuccess()) { return error; }

    uint64 instanceId;
    if (!StringUtility::TryFromWString<uint64>(tokens[2], instanceId))
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    servicePackageVersionInstance = ServicePackageVersionInstance(ServicePackageVersion(applicationVersion, packageVersion), instanceId);
    return ErrorCode(ErrorCodeValue::Success);
}

void ServicePackageVersionInstance::ToEnvironmentMap(EnvironmentMap & envMap) const
{
    envMap[ServicePackageVersionInstance::EnvVarName_ServicePackageVersionInstance] = ToString();
}

ErrorCode ServicePackageVersionInstance::FromEnvironmentMap(
    EnvironmentMap const & envMap, 
    __out ServicePackageVersionInstance & servicePackageVersionInstance)
{
    auto servicePackageVersionInstanceIter = envMap.find(ServicePackageVersionInstance::EnvVarName_ServicePackageVersionInstance);

    if(servicePackageVersionInstanceIter == envMap.end())
    {
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    return FromString(servicePackageVersionInstanceIter->second, servicePackageVersionInstance);
}
