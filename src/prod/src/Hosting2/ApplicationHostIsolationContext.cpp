// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

GlobalWString ApplicationHostIsolationContext::CodePackageSharing = make_global<wstring>(L"CodePackage:");
GlobalWString ApplicationHostIsolationContext::ApplicationSharing = make_global<wstring>(L"Application:");

ApplicationHostIsolationContext::ApplicationHostIsolationContext(
    wstring const & processContext,
    wstring const & userContext)
    : processContext_(processContext),
    userContext_(userContext)
{
}

ApplicationHostIsolationContext::ApplicationHostIsolationContext(ApplicationHostIsolationContext const & other)
    : processContext_(other.processContext_),
    userContext_(other.userContext_)
{
}

ApplicationHostIsolationContext::ApplicationHostIsolationContext(ApplicationHostIsolationContext && other)
    : processContext_(move(other.processContext_)),
    userContext_(move(other.userContext_))
{
}

ApplicationHostIsolationContext const & ApplicationHostIsolationContext::operator = (ApplicationHostIsolationContext const & other)
{
    if (this != &other)
    {
        this->processContext_ = other.processContext_;
        this->userContext_ = other.userContext_;
    }

    return *this;
}

ApplicationHostIsolationContext const & ApplicationHostIsolationContext::operator = (ApplicationHostIsolationContext && other)
{
    if (this != &other)
    {
        this->processContext_ = move(other.processContext_);
        this->userContext_ = move(other.userContext_);
    }

    return *this;
}

bool ApplicationHostIsolationContext::operator == (ApplicationHostIsolationContext const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->processContext_, other.processContext_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->userContext_, other.userContext_);
    if (!equals) { return equals; }

    return equals;
}

bool ApplicationHostIsolationContext::operator != (ApplicationHostIsolationContext const & other) const
{
    return !(*this == other);
}

int ApplicationHostIsolationContext::compare(ApplicationHostIsolationContext const & other) const
{
    int comparision = this->processContext_.compare(other.processContext_);
    if (comparision != 0) { return comparision; }

    comparision = this->userContext_.compare(other.userContext_);
    if (comparision != 0) { return comparision; }

    return comparision;
}

bool ApplicationHostIsolationContext::operator < (ApplicationHostIsolationContext const & other) const
{
    return (this->compare(other) < 0);
}

void ApplicationHostIsolationContext::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ApplicationHostIsolationContext { ");
    w.Write("ProcessContext = {0}, ", processContext_);
    w.Write("UserContext = {0}, ", userContext_);
    w.Write("}");
}

ApplicationHostIsolationContext::ProcessSharingLevel ApplicationHostIsolationContext::GetProcessSharingLevel() const
{
    if (StringUtility::StartsWith(processContext_, *(ApplicationHostIsolationContext::CodePackageSharing)))
    {
        return ProcessSharingLevel::ProcessSharingLevel_CodePackage;
    } 
    else if (StringUtility::StartsWith(processContext_, *(ApplicationHostIsolationContext::ApplicationSharing)))
    {
        return ProcessSharingLevel::ProcessSharingLevel_Application;
    }
    else
    {
        Assert::CodingError(
            "ApplicationHostIsolationContext::GetProcessSharingLevel: Invalid ProcessContext {0}",
            processContext_);
    }
}


ApplicationHostIsolationContext ApplicationHostIsolationContext::Create(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    wstring const & runAs,
    uint64 instanceId,
    CodePackageIsolationPolicyType::Enum const & codeIsolationPolicy)
{
    wstring userContext = L"User:" + runAs;
    wstring processContext;

    if (codeIsolationPolicy == CodePackageIsolationPolicyType::DedicatedProcess)
    {
        // do not share process with another code package
        processContext = wformatString("{0}{1}{2}",
            *(ApplicationHostIsolationContext::CodePackageSharing),
            codePackageInstanceId.ToString(),
            instanceId);
    }
    else
    {
        // share within the same application
        processContext = *(ApplicationHostIsolationContext::ApplicationSharing) + codePackageInstanceId.ServicePackageInstanceId.ApplicationId.ToString();
    }

    return ApplicationHostIsolationContext(processContext, userContext);
}
