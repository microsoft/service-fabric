// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

GlobalWString ServicePackageVersion::Delimiter = make_global<wstring>(L":");
GlobalWString ServicePackageVersion::EnvVarName_ServicePackageVersion = make_global<wstring>(L"Fabric_ServicePackageVersion");

const ServicePackageVersion ServicePackageVersion::Zero(ApplicationVersion(RolloutVersion(0,0)), RolloutVersion(0,0));

ServicePackageVersion const ServicePackageVersion::Invalid(ApplicationVersion::Invalid, RolloutVersion::Invalid);

ServicePackageVersion::ServicePackageVersion()
    : appVersion_(),
    value_()
{
}

ServicePackageVersion::ServicePackageVersion(
    ApplicationVersion const & appVersion,
    RolloutVersion const & value)
    : appVersion_(appVersion),
    value_(value)
{
}

ServicePackageVersion::ServicePackageVersion(
    ServicePackageVersion const & other)
    : appVersion_(other.appVersion_),
    value_(other.value_)
{
}

ServicePackageVersion::ServicePackageVersion(
    ServicePackageVersion && other)
    : appVersion_(move(other.appVersion_)),
    value_(move(other.value_))
{
}

ServicePackageVersion::~ServicePackageVersion() 
{ 
}

ServicePackageVersion const & ServicePackageVersion::operator = (ServicePackageVersion const & other)
{
    if (this != &other)
    {
        this->appVersion_ = other.appVersion_;
        this->value_ = other.value_;
    }

    return *this;
}

ServicePackageVersion const & ServicePackageVersion::operator = (ServicePackageVersion && other)
{
    if (this != &other)
    {
        this->appVersion_ = move(other.appVersion_);
        this->value_ = move(other.value_);
    }

    return *this;
}

bool ServicePackageVersion::operator == (ServicePackageVersion const & other) const
{
    return ((this->appVersion_ == other.appVersion_) && (this->value_ == other.value_));
}

bool ServicePackageVersion::operator != (ServicePackageVersion const & other) const
{
    return !(*this == other);
}

int ServicePackageVersion::compare(ServicePackageVersion const & other) const
{
    int comparision = this->appVersion_.compare(other.appVersion_);
    if (comparision != 0) { return comparision; }

    comparision = this->value_.compare(other.value_);
    return comparision;
}

bool ServicePackageVersion::operator < (ServicePackageVersion const & other) const
{
    return (this->compare(other) < 0);
}

void ServicePackageVersion::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ServicePackageVersion::ToString() const
{
    wstring result(appVersion_.ToString());
    result.append(Delimiter);
    result.append(value_.ToString());

    return result;
}

ErrorCode ServicePackageVersion::FromString(wstring const & servicePackageVersionString, __out ServicePackageVersion & servicePackageVersion)
{
    vector<wstring> tokens;  
    StringUtility::Split<wstring>(servicePackageVersionString, tokens, Delimiter);

    if (tokens.size() != 2)
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    ApplicationVersion applicationVersion;
    RolloutVersion value;
    auto error = ApplicationVersion::FromString(tokens[0], applicationVersion);
    if (!error.IsSuccess()) { return error; }

    error = RolloutVersion::FromString(tokens[1], value);
    if (!error.IsSuccess()) { return error; }

    servicePackageVersion = ServicePackageVersion(applicationVersion, value);
    return ErrorCode(ErrorCodeValue::Success);
}

void ServicePackageVersion::ToEnvironmentMap(EnvironmentMap & envMap) const
{
    envMap[ServicePackageVersion::EnvVarName_ServicePackageVersion] = ToString();
}

ErrorCode ServicePackageVersion::FromEnvironmentMap(EnvironmentMap const & envMap, __out ServicePackageVersion & servicePackageVersion)
{
    auto servicePackageVersionIter = envMap.find(ServicePackageVersion::EnvVarName_ServicePackageVersion);

    if(servicePackageVersionIter == envMap.end())
    {
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    return FromString(servicePackageVersionIter->second, servicePackageVersion);
}

string ServicePackageVersion::AddField(Common::TraceEvent & traceEvent, string const & name)
{            
    string format = "{0}:{1}";
    size_t index = 0;

    traceEvent.AddEventField<ServiceModel::ApplicationVersion>(format, name + ".appVersion", index);
    traceEvent.AddEventField<ServiceModel::RolloutVersion>(format, name + ".rolloutVersion", index);
    return format;
}

void ServicePackageVersion::FillEventData(TraceEventContext & context) const
{
    context.Write<ServiceModel::ApplicationVersion>(appVersion_);
    context.Write<ServiceModel::RolloutVersion>(value_);
}
