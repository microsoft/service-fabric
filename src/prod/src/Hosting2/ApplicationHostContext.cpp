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
GlobalWString ApplicationHostContext::EnvVarName_IsCodePackageActivatorHost = make_global<wstring>(L"Fabric_IsCodePackageActivatorHost");

ApplicationHostContext::ApplicationHostContext()
    : hostId_(Guid::NewGuid().ToString()),
    hostType_(ApplicationHostType::NonActivated),
    processId_(0),
    isContainerHost_(false),
    isCodePackageActivatorHost_(false)
{
}

ApplicationHostContext::ApplicationHostContext(
    wstring const & hostId, 
    ApplicationHostType::Enum const hostType,
    bool isContainerHost,
    bool isCodePackageActivatorHost)
    : hostId_(hostId),
    hostType_(hostType),
    processId_(0),
    isContainerHost_(isContainerHost),
    isCodePackageActivatorHost_(isCodePackageActivatorHost)
{
    ASSERT_IF(
        hostType_ != ApplicationHostType::Activated_SingleCodePackage &&
        hostType_ != ApplicationHostType::Activated_InProcess &&
        isCodePackageActivatorHost_,
        "Only SingleCodePackage or InProcess application host can have on-demand CP activation enabled.");
}

ApplicationHostContext::ApplicationHostContext(ApplicationHostContext const & other)
    : hostId_(other.hostId_),
    hostType_(other.hostType_),
    processId_(other.processId_),
    isContainerHost_(other.isContainerHost_),
    isCodePackageActivatorHost_(other.isCodePackageActivatorHost_)
{
}

ApplicationHostContext::ApplicationHostContext(ApplicationHostContext && other)
    : hostId_(move(other.hostId_)),
    hostType_(other.hostType_),
    processId_(other.processId_),
    isContainerHost_(other.isContainerHost_),
    isCodePackageActivatorHost_(other.isCodePackageActivatorHost_)
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
        this->isCodePackageActivatorHost_ = other.isCodePackageActivatorHost_;
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
        this->isCodePackageActivatorHost_ = other.isCodePackageActivatorHost_;
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

    equals = (this->isCodePackageActivatorHost_ == other.isCodePackageActivatorHost_);
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
    w.Write("HostId={0}, ", hostId_);
    w.Write("HostType={0}, ", hostType_);
    w.Write("ProcessId={0}, ", processId_);
    w.Write("IsContainerHost={0}, ", isContainerHost_);
    w.Write("IsCodePackageActivatorHost={0} ", isCodePackageActivatorHost_);
    w.Write("}");
}

// processId is not pushed in the environment map and not read from the environment map
ErrorCode ApplicationHostContext::FromEnvironmentMap(
    EnvironmentMap const & envMap, 
    __out ApplicationHostContext & hostContext)
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

    auto isContainerHost = false;
    auto containerHostIter = envMap.find(ContainerEnvironment::IsContainerHostEnvironmentVariableName);
    if (containerHostIter != envMap.end())
    {
        if (!StringUtility::ParseBool::Try(containerHostIter->second, isContainerHost))
        {
            CODING_ASSERT(
                "Invalid value '{0}' for environment variable '{1}'.",
                containerHostIter->second,
                *ContainerEnvironment::IsContainerHostEnvironmentVariableName);
        }
    }

    auto isCodePackageActivatorHost = false;
    auto activatorHostIter = envMap.find(ApplicationHostContext::EnvVarName_IsCodePackageActivatorHost);
    if (activatorHostIter != envMap.end())
    {
        if (!StringUtility::ParseBool::Try(activatorHostIter->second, isCodePackageActivatorHost))
        {
            CODING_ASSERT(
                "Invalid value '{0}' for environment variable '{1}'.",
                activatorHostIter->second,
                *EnvVarName_IsCodePackageActivatorHost);
        }
    }

    hostContext = ApplicationHostContext(hostId, hostType, isContainerHost, isCodePackageActivatorHost);

    return ErrorCode::Success();
}

// processId is not pushed in the environment map and not read from the environment map
void ApplicationHostContext::ToEnvironmentMap(EnvironmentMap & envMap) const
{
    envMap[ApplicationHostContext::EnvVarName_HostId] = hostId_;
    envMap[ApplicationHostContext::EnvVarName_HostType] = ApplicationHostType::ToString(hostType_);
    envMap[ContainerEnvironment::IsContainerHostEnvironmentVariableName] = wformatString("{0}", isContainerHost_);
    envMap[ApplicationHostContext::EnvVarName_IsCodePackageActivatorHost] = wformatString("{0}", isCodePackageActivatorHost_);
}
