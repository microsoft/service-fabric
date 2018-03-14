// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

GlobalWString ApplicationHostContext::EnvVarName_HostId = make_global<wstring>(L"Fabric_ApplicationHostId");
GlobalWString ApplicationHostContext::EnvVarName_HostType = make_global<wstring>(L"Fabric_ApplicationHostType");

ApplicationHostContext::ApplicationHostContext()
    : hostId_(Guid::NewGuid().ToString()),
    hostType_(ApplicationHostType::NonActivated),
    processId_(0),
	isContainerHost_(false)
{
}

ApplicationHostContext::ApplicationHostContext(
    wstring const & hostId, 
    ApplicationHostType::Enum const hostType,
	bool isContainerHost)
    : hostId_(hostId),
    hostType_(hostType),
    processId_(0),
	isContainerHost_(isContainerHost)
{
}

ApplicationHostContext::ApplicationHostContext(ApplicationHostContext const & other)
    : hostId_(other.hostId_),
    hostType_(other.hostType_),
    processId_(other.processId_),
	isContainerHost_(other.isContainerHost_)
{
}

ApplicationHostContext::ApplicationHostContext(ApplicationHostContext && other)
    : hostId_(move(other.hostId_)),
    hostType_(other.hostType_),
    processId_(other.processId_),
	isContainerHost_(other.isContainerHost_)
{
}

ApplicationHostContext const & ApplicationHostContext::operator = (ApplicationHostContext const & other)
{
    if (this != &other)
    {
        this->hostId_ = other.hostId_;
        this->hostType_ = other.hostType_;
        this->processId_ = other.processId_;
		this->isContainerHost_ = other.isContainerHost_;
    }

    return *this;
}

ApplicationHostContext const & ApplicationHostContext::operator = (ApplicationHostContext && other)
{
    if (this != &other)
    {
        this->hostId_ = move(other.hostId_);
        this->hostType_ = other.hostType_;
        this->processId_ = other.processId_;
		this->isContainerHost_ = other.isContainerHost_;
    }

    return *this;
}

bool ApplicationHostContext::operator == (ApplicationHostContext const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->hostId_, other.hostId_);
    if (!equals) { return equals; }

    equals = (this->hostType_ == other.hostType_);
    if (!equals) { return equals; }

	equals = (this->isContainerHost_ == other.isContainerHost_);
	if (!equals) { return equals; }

    return equals;
}

bool ApplicationHostContext::operator != (ApplicationHostContext const & other) const
{
    return !(*this == other);
}

void ApplicationHostContext::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ApplicationHostContext { ");
    w.Write("HostId = {0}, ", hostId_);
    w.Write("HostType = {0}, ", hostType_);
    w.Write("ProcessId = {0}, ", processId_);
	w.Write("IsContainerHost = {0}, ", isContainerHost_);
    w.Write("}");
}

// processId is not pushed in the environment map and not read from the environment map
ErrorCode ApplicationHostContext::FromEnvironmentMap(EnvironmentMap const & envMap, __out ApplicationHostContext & hostContext)
{
    auto hostIdIterator = envMap.find(ApplicationHostContext::EnvVarName_HostId);
    auto hostTypeIterator = envMap.find(ApplicationHostContext::EnvVarName_HostType);

    if (hostIdIterator == envMap.end() && hostTypeIterator == envMap.end())
    {
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    if (hostIdIterator == envMap.end() || hostTypeIterator == envMap.end())
    {
        return ErrorCode(ErrorCodeValue::InvalidState);
    }

    wstring hostId(hostIdIterator->second);
    ApplicationHostType::Enum hostType;
    auto error = ApplicationHostType::FromString(hostTypeIterator->second, hostType);
    if (!error.IsSuccess()) { return error; }

    bool isContainer = ContainerEnvironment::IsContainerHost();

    hostContext = ApplicationHostContext(hostId, hostType, isContainer);

    return ErrorCode::Success();
}

// processId is not pushed in the environment map and not read from the environment map
void ApplicationHostContext::ToEnvironmentMap(EnvironmentMap & envMap) const
{
    envMap[ApplicationHostContext::EnvVarName_HostId] = hostId_;
    envMap[ApplicationHostContext::EnvVarName_HostType] = ApplicationHostType::ToString(hostType_);
	envMap[ContainerEnvironment::IsContainerHostEnvironmentVariableName] = wformatString("{0}", isContainerHost_);
}
