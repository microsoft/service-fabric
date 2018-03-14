// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceType_ServicePackageIdentifier("ServicePackageIdentifier");

GlobalWString ServicePackageIdentifier::Delimiter = make_global<wstring>(L":");
GlobalWString ServicePackageIdentifier::EnvVarName_ServicePackageName = make_global<wstring>(L"Fabric_ServicePackageName");

INITIALIZE_SIZE_ESTIMATION(ServicePackageIdentifier)

ServicePackageIdentifier::ServicePackageIdentifier()
    : applicationIdentifier_(),
    servicePackageName_()
{
}

ServicePackageIdentifier::ServicePackageIdentifier(
    wstring const & applicationIdString,
    wstring const & servicePackageName)
    : applicationIdentifier_(),
    servicePackageName_(servicePackageName)
{
    auto error = ApplicationIdentifier::FromString(applicationIdString, applicationIdentifier_);
    ASSERT_IF(
        !error.IsSuccess(), 
        "Failed to converted ApplicationIdString {0} to ApplicationIdentifier. ErrorCode={1}", 
        applicationIdString, 
        error);

    ASSERT_IF((!applicationIdentifier_.IsEmpty() && servicePackageName_.empty()),
        "ServicePackageName must be non-empty for non-empty ApplicationId {0}", applicationIdentifier_);
}

ServicePackageIdentifier::ServicePackageIdentifier(
    ApplicationIdentifier const & applicationIdentifier, 
    wstring const & servicePackageName)
    : applicationIdentifier_(applicationIdentifier),
    servicePackageName_(servicePackageName)
{
    ASSERT_IF((!applicationIdentifier_.IsEmpty() && servicePackageName_.empty()),
        "ServicePackageName must be non-empty for non-empty ApplicationId {0}", applicationIdentifier_);
}

ServicePackageIdentifier::ServicePackageIdentifier(ServicePackageIdentifier const & other)
    : applicationIdentifier_(other.applicationIdentifier_),
    servicePackageName_(other.servicePackageName_)
{
}

ServicePackageIdentifier::ServicePackageIdentifier(ServicePackageIdentifier && other)
    : applicationIdentifier_(move(other.applicationIdentifier_)),
    servicePackageName_(move(other.servicePackageName_))
{
}

bool ServicePackageIdentifier::IsEmpty() const 
{
    return (applicationIdentifier_.IsEmpty() && servicePackageName_.empty());
}

ServicePackageIdentifier const & ServicePackageIdentifier::operator = (ServicePackageIdentifier const & other)
{
    if (this != &other)
    {
        this->applicationIdentifier_ = other.applicationIdentifier_;
        this->servicePackageName_ = other.servicePackageName_;
    }

    return *this;
}

ServicePackageIdentifier const & ServicePackageIdentifier::operator = (ServicePackageIdentifier && other)
{
    if (this != &other)
    {
        this->applicationIdentifier_ = move(other.applicationIdentifier_);
        this->servicePackageName_ = move(other.servicePackageName_);
    }

    return *this;
}

bool ServicePackageIdentifier::operator == (ServicePackageIdentifier const & other) const
{
     bool equals = true;

    equals = (this->applicationIdentifier_ == other.applicationIdentifier_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->servicePackageName_, other.servicePackageName_);
    if (!equals) { return equals; }

    return equals;
}

bool ServicePackageIdentifier::operator != (ServicePackageIdentifier const & other) const
{
    return !(*this == other);
}

int ServicePackageIdentifier::compare(ServicePackageIdentifier const & other) const
{
    int comparision = this->applicationIdentifier_.compare(other.applicationIdentifier_);
    if (comparision != 0) { return comparision; }

    comparision = StringUtility::CompareCaseInsensitive(this->servicePackageName_, other.servicePackageName_);
    if (comparision != 0) { return comparision; }

    return comparision;
}

bool ServicePackageIdentifier::operator < (ServicePackageIdentifier const & other) const
{
    return (this->compare(other) < 0);
}

void ServicePackageIdentifier::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ServicePackageIdentifier::ToString() const
{
    return ServicePackageIdentifier::GetServicePackageIdentifierString(applicationIdentifier_, servicePackageName_);
}

ErrorCode ServicePackageIdentifier::FromString(
    wstring const & servicePackageIdString,
    __out ServicePackageIdentifier & servicePackageId)
{
    if (servicePackageIdString.empty())
    {
        // both ApplicationId and ServicePackageName is empty
        servicePackageId = ServicePackageIdentifier();
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        auto pos = servicePackageIdString.find_last_of(*ServicePackageIdentifier::Delimiter);
        if (pos == wstring::npos)
        {
            // ApplicationId is empty
            servicePackageId = ServicePackageIdentifier(ApplicationIdentifier(), servicePackageIdString);
            return ErrorCode(ErrorCodeValue::Success);
        }
        else
        {
            // both ApplicationId and ServicePackageName is non-empty
            wstring applicationIdString = servicePackageIdString.substr(0, pos);
            wstring servicePackageName = servicePackageIdString.substr(pos+1);
        
            ApplicationIdentifier applicationId;
            auto error = ApplicationIdentifier::FromString(applicationIdString, applicationId);
            if (!error.IsSuccess()) { return error; }

            servicePackageId = ServicePackageIdentifier(applicationId, servicePackageName);
            return ErrorCode(ErrorCodeValue::Success);
        }
    }
}

void ServicePackageIdentifier::ToEnvironmentMap(EnvironmentMap & envMap) const
{
    applicationIdentifier_.ToEnvironmentMap(envMap);
    envMap[ServicePackageIdentifier::EnvVarName_ServicePackageName] = servicePackageName_;
}

ErrorCode ServicePackageIdentifier::FromEnvironmentMap(
    EnvironmentMap const & envMap, 
    __out ServicePackageIdentifier & servicePackageId)
{
    ApplicationIdentifier applicationId;
    auto error = ApplicationIdentifier::FromEnvironmentMap(envMap, applicationId);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound)) { return error; }

    auto servicePackageNameIterator = envMap.find(ServicePackageIdentifier::EnvVarName_ServicePackageName);
    if (servicePackageNameIterator == envMap.end())
    {
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    servicePackageId = ServicePackageIdentifier(applicationId, servicePackageNameIterator->second);
    return ErrorCode(ErrorCodeValue::Success);
}

wstring ServicePackageIdentifier::GetServicePackageIdentifierString(wstring const & applicationId, wstring const & servicePackageName)
{
    if (applicationId.empty() && servicePackageName.empty())
    {
        return L"";
    }
    else
    {
        ASSERT_IF(
            !applicationId.empty() && servicePackageName.empty(), 
            "ServicePackageName must be non-empty for non-empty ApplicationId {0}", applicationId);

        if (applicationId.empty())
        {
            return servicePackageName;
        }
        else
        {
            return applicationId + *Delimiter + servicePackageName;
        }
    }
}

wstring ServicePackageIdentifier::GetServicePackageIdentifierString(ApplicationIdentifier const & applicationId, wstring const & servicePackageName)
{
    return GetServicePackageIdentifierString(applicationId.ToString(), servicePackageName);
}
