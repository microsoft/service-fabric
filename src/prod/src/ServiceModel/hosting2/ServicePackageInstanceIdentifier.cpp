// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

GlobalWString ServicePackageInstanceIdentifier::Delimiter = make_global<wstring>(L"@");
GlobalWString ServicePackageInstanceIdentifier::EnvVarName_ServicePackageActivationId = make_global<wstring>(L"Fabric_ServicePackageActivationId");

INITIALIZE_SIZE_ESTIMATION(ServicePackageInstanceIdentifier)

ServicePackageInstanceIdentifier::ServicePackageInstanceIdentifier()
    : servicePackageId_()
    , activationContext_()
	, servicePackageActivationId_()
{
}

ServicePackageInstanceIdentifier::ServicePackageInstanceIdentifier(
    ServicePackageIdentifier const & servicePackageId,
    ServicePackageActivationContext const & activationContext,
	wstring const & servicePackageActivationId)
    : servicePackageId_(servicePackageId)
    , activationContext_(activationContext)
	, servicePackageActivationId_(servicePackageActivationId)
{
	Validate(activationContext_, servicePackageActivationId_);
}

ServicePackageInstanceIdentifier::ServicePackageInstanceIdentifier(ServicePackageInstanceIdentifier const & other)
    : servicePackageId_(other.servicePackageId_)
    , activationContext_(other.activationContext_)
	, servicePackageActivationId_(other.servicePackageActivationId_)
{
}

ServicePackageInstanceIdentifier::ServicePackageInstanceIdentifier(ServicePackageInstanceIdentifier && other)
    : servicePackageId_(move(other.servicePackageId_))
    , activationContext_(move(other.activationContext_))
	, servicePackageActivationId_(move(other.servicePackageActivationId_))
{
}

ServicePackageInstanceIdentifier const & ServicePackageInstanceIdentifier::operator = (ServicePackageInstanceIdentifier const & other)
{
    if (this != &other)
    {
        this->servicePackageId_ = other.servicePackageId_;
        this->activationContext_ = other.activationContext_;
		this->servicePackageActivationId_ = other.servicePackageActivationId_;
    }

    return *this;
}

ServicePackageInstanceIdentifier const & ServicePackageInstanceIdentifier::operator = (ServicePackageInstanceIdentifier && other)
{
    if (this != &other)
    {
        this->servicePackageId_ = move(other.servicePackageId_);
        this->activationContext_ = move(other.activationContext_);
		this->servicePackageActivationId_ = move(other.servicePackageActivationId_);
    }

    return *this;
}

bool ServicePackageInstanceIdentifier::operator == (ServicePackageInstanceIdentifier const & other) const
{
    bool equals = true;

    equals = (this->servicePackageId_ == other.servicePackageId_);
    if (!equals) { return equals; }

	equals = (this->activationContext_ == other.activationContext_);
	if (!equals) { return equals; }

	ASSERT_IF(
		this->servicePackageActivationId_ != other.servicePackageActivationId_,
		"ServicePackageActivationId must match when ServicePackageId and ActivationContext match.")

    return equals;
}

bool ServicePackageInstanceIdentifier::operator != (ServicePackageInstanceIdentifier const & other) const
{
    return !(*this == other);
}

int ServicePackageInstanceIdentifier::compare(ServicePackageInstanceIdentifier const & other) const
{
    int comparision = this->servicePackageId_.compare(other.servicePackageId_);
    if (comparision != 0) { return comparision; }

    comparision = this->activationContext_.compare(other.activationContext_);
    if (comparision != 0) { return comparision; }

    return comparision;
}

bool ServicePackageInstanceIdentifier::operator < (ServicePackageInstanceIdentifier const & other) const
{
    return (this->compare(other) < 0);
}

void ServicePackageInstanceIdentifier::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(this->ToString());
}

wstring ServicePackageInstanceIdentifier::ToString() const
{
    if (activationContext_.IsExclusive)
    {
        return wformatString(
			"{0}{1}{2}{3}{4}", 
			servicePackageId_.ToString(), 
			Delimiter, 
			activationContext_.ToString(),
			Delimiter,
			servicePackageActivationId_);
    }

    return servicePackageId_.ToString();
}

ErrorCode ServicePackageInstanceIdentifier::FromString(
    wstring const & packageInstanceIdString, 
    __out ServicePackageInstanceIdentifier & servicePackageInstanceId)
{
    if (packageInstanceIdString.empty())
    {
        servicePackageInstanceId = ServicePackageInstanceIdentifier();
        return ErrorCode(ErrorCodeValue::Success);
    }

    vector<wstring> tokens;
    StringUtility::Split(packageInstanceIdString, tokens, *Delimiter);

    if (tokens.size() > 3 || tokens.size() == 2)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    ServicePackageIdentifier servicePackageId;
    auto error = ServicePackageIdentifier::FromString(tokens[0], servicePackageId);
    if (!error.IsSuccess())
    {
        return error;
    }
    
    ServicePackageActivationContext activationContext;
	wstring servicePackageActivationId;

    if (tokens.size() == 3)
    {
        error = ServicePackageActivationContext::FromString(tokens[1], activationContext);
        if (!error.IsSuccess())
        {
            return error;
        }

		servicePackageActivationId = tokens[2];
    }
    
    servicePackageInstanceId = ServicePackageInstanceIdentifier(servicePackageId, activationContext, servicePackageActivationId);

    return error;
}

bool ServicePackageInstanceIdentifier::IsActivationOf(ServicePackageIdentifier servicePackageId) const
{
    return (servicePackageId_ == servicePackageId);
}

void ServicePackageInstanceIdentifier::ToEnvironmentMap(EnvironmentMap & envMap) const
{
    servicePackageId_.ToEnvironmentMap(envMap);
    activationContext_.ToEnvironmentMap(envMap);

	envMap[ServicePackageInstanceIdentifier::EnvVarName_ServicePackageActivationId] = servicePackageActivationId_;
}

ErrorCode ServicePackageInstanceIdentifier::FromEnvironmentMap(
    EnvironmentMap const & envMap,
    __out ServicePackageInstanceIdentifier & servicePackageInstanceId)
{
    ServicePackageIdentifier servicePackageId;
    auto error = ServicePackageIdentifier::FromEnvironmentMap(envMap, servicePackageId);
    if (!error.IsSuccess()) { return error; }

    ServicePackageActivationContext activationContext;
    error = ServicePackageActivationContext::FromEnvironmentMap(envMap, activationContext);
    if (!error.IsSuccess()) { return error; }

	wstring servicePackageActivationId;
	auto activationIdIterator = envMap.find(ServicePackageInstanceIdentifier::EnvVarName_ServicePackageActivationId);
	if (activationIdIterator != envMap.end())
	{
		servicePackageActivationId = activationIdIterator->second;
	}

	servicePackageInstanceId = ServicePackageInstanceIdentifier(servicePackageId, activationContext, servicePackageActivationId);
    return ErrorCode(ErrorCodeValue::Success);
}

void ServicePackageInstanceIdentifier::Validate(
	ServicePackageActivationContext const & activationContext,
	wstring const & servicePackageActivationId)
{
	ASSERT_IF(
		activationContext.IsExclusive && servicePackageActivationId.empty(),
		"ServicePackageActivationId is empty for exclusive ServicePackageActivationContext.")

	Guid dontCare;
	ASSERT_IF(
		!servicePackageActivationId.empty() && !Guid::TryParse(servicePackageActivationId, dontCare),
		"Non empty ServicePackageActivationId must be a guid string. Actual=[{0}]",
		servicePackageActivationId)
}
