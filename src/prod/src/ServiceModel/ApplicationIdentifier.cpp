// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceType_ApplicationIdentifier("ApplicationIdentifier");

Global<ApplicationIdentifier> ApplicationIdentifier::FabricSystemAppId = make_global<ApplicationIdentifier>(L"__FabricSystem", 4294967295);

GlobalWString ApplicationIdentifier::Delimiter = make_global<wstring>(L"_App");
GlobalWString ApplicationIdentifier::EnvVarName_ApplicationId = make_global<wstring>(L"Fabric_ApplicationId");

INITIALIZE_SIZE_ESTIMATION(ApplicationIdentifier)

ApplicationIdentifier::ApplicationIdentifier()
    : applicationTypeName_(),
    applicationNumber_(0)
{
}

ApplicationIdentifier::ApplicationIdentifier(
    wstring const & applicationTypeName, 
    ULONG applicationNumber)
    : applicationTypeName_(applicationTypeName),
    applicationNumber_(applicationNumber)
{
    ASSERT_IF(applicationTypeName_.empty() && (applicationNumber_ != 0),
        "ApplicationNumber must be 0 for empty ApplicationTypeName, specified {0}", applicationNumber_);
}

ApplicationIdentifier::ApplicationIdentifier(ApplicationIdentifier const & other)
    : applicationTypeName_(other.applicationTypeName_),
    applicationNumber_(other.applicationNumber_)
{
}

ApplicationIdentifier::ApplicationIdentifier(ApplicationIdentifier && other)
    : applicationTypeName_(move(other.applicationTypeName_)),
    applicationNumber_(other.applicationNumber_)
{
}

bool ApplicationIdentifier::IsEmpty() const
{
    return (applicationTypeName_.empty() && (applicationNumber_ == 0));
}

bool ApplicationIdentifier::IsAdhoc() const
{
    return IsEmpty();
}

bool ApplicationIdentifier::IsSystem() const
{
    return (this->operator==(FabricSystemAppId));
}

ApplicationIdentifier const & ApplicationIdentifier::operator = (ApplicationIdentifier const & other)
{
    if (this != &other)
    {
        this->applicationTypeName_ = other.applicationTypeName_;
        this->applicationNumber_ = other.applicationNumber_;
    }

    return *this;
}

ApplicationIdentifier const & ApplicationIdentifier::operator = (ApplicationIdentifier && other)
{
    if (this != &other)
    {
        this->applicationTypeName_ = move(other.applicationTypeName_);
        this->applicationNumber_ = other.applicationNumber_;
    }

    return *this;
}

bool ApplicationIdentifier::operator == (ApplicationIdentifier const & other) const
{
    bool equals = true;

    equals = this->applicationNumber_ == other.applicationNumber_;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->applicationTypeName_, other.applicationTypeName_);
    if (!equals) { return equals; }

    return equals;
}

bool ApplicationIdentifier::operator != (ApplicationIdentifier const & other) const
{
    return !(*this == other);
}

int ApplicationIdentifier::compare(ApplicationIdentifier const & other) const
{
    if (this->applicationNumber_ > other.applicationNumber_)
    {
        return 1;
    }
    else if (this->applicationNumber_ < other.applicationNumber_)
    {
        return -1;
    }
    else
    {
        return StringUtility::CompareCaseInsensitive(this->applicationTypeName_, other.applicationTypeName_);
    }
}

bool ApplicationIdentifier::operator < (ApplicationIdentifier const & other) const
{
    return (this->compare(other) < 0);
}

void ApplicationIdentifier::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w.Write(ToString());
}

wstring ApplicationIdentifier::ToString() const
{
    if (IsEmpty())
    {
        return L"";
    }
    else
    {
        return wformatString("{0}{1}{2}",this->applicationTypeName_, Delimiter, this->applicationNumber_);
    }
}

ErrorCode ApplicationIdentifier::FromString(wstring const & applicationIdStr, __out ApplicationIdentifier & applicationId)
{
    if (applicationIdStr.empty())
    {
        applicationId = ApplicationIdentifier();
    }
    else
    {
        auto pos = applicationIdStr.find_last_of(*Delimiter);
        if (pos == wstring::npos)
        {
            TraceNoise(
                TraceTaskCodes::ServiceModel,
                TraceType_ApplicationIdentifier,
                "FromString: Invalid argument {0}", applicationIdStr);
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }

        wstring applicationTypeName = applicationIdStr.substr(0, pos - Delimiter->length() + 1);
        wstring applicationNumberStr = applicationIdStr.substr(pos + 1);
        ULONG appNumber;
        if (!StringUtility::TryFromWString<ULONG>(applicationNumberStr, appNumber))
        {
            TraceNoise(
                TraceTaskCodes::ServiceModel,
                TraceType_ApplicationIdentifier,
                "FromString: Invalid argument {0}", applicationIdStr);
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }

        applicationId = ApplicationIdentifier(applicationTypeName, appNumber);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void ApplicationIdentifier::ToEnvironmentMap(EnvironmentMap & envMap) const
{
    envMap[ApplicationIdentifier::EnvVarName_ApplicationId] = this->ToString();
}

ErrorCode ApplicationIdentifier::FromEnvironmentMap(EnvironmentMap const & envMap, __out ApplicationIdentifier & applicationId)
{
    auto appIdIterator = envMap.find(ApplicationIdentifier::EnvVarName_ApplicationId);
    if(appIdIterator == envMap.end())
    {
        return ErrorCode(ErrorCodeValue::NotFound);
    }
    else
    {
        return ApplicationIdentifier::FromString(appIdIterator->second, applicationId);
    }
}

string ApplicationIdentifier::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{            
    traceEvent.AddField<bool>(name + ".isEmpty");
    traceEvent.AddField<std::wstring>(name + ".typeName");
    traceEvent.AddField<std::wstring>(name + ".delimiter");
    traceEvent.AddField<uint32>(name + ".applicationNumber");

    return "isEmpty={0};appId={1}{2}{3}";
}

void ApplicationIdentifier::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<bool>(IsEmpty());
    context.Write<std::wstring>(applicationTypeName_);
    context.Write<std::wstring>(Delimiter);
    context.WriteCopy<uint32>(static_cast<uint32>(applicationNumber_));
}
