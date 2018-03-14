// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

GlobalWString CodePackageContext::EnvVarName_ServicePackageInstanceSeqNum = make_global<wstring>(L"Fabric_ServicePackageInstanceSeqNum");
GlobalWString CodePackageContext::EnvVarName_CodePackageInstanceSeqNum = make_global<wstring>(L"Fabric_CodePackageInstanceSeqNum");
GlobalWString CodePackageContext::EnvVarName_ApplicationName = make_global<wstring>(L"Fabric_ApplicationName");

CodePackageContext::CodePackageContext()
    : codePackageInstanceId_()
    , servicePackageInstanceSeqNum_(0)
    , codePackageInstanceSeqNum_(0)
    , servicePackageVersionInstance_()
    , applicationName_()
{
}

CodePackageContext::CodePackageContext(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    int64 codePackageInstanceSeqNum,
    int64 servicePackageInstanceSeqNum,
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    wstring const & applicationName)
    : codePackageInstanceId_(codePackageInstanceId)
    , codePackageInstanceSeqNum_(codePackageInstanceSeqNum)
    , servicePackageInstanceSeqNum_(servicePackageInstanceSeqNum)
    , servicePackageVersionInstance_(servicePackageVersionInstance)
    , applicationName_(applicationName)
{
}

CodePackageContext::CodePackageContext(CodePackageContext const & other)
    : codePackageInstanceId_(other.codePackageInstanceId_)
    , codePackageInstanceSeqNum_(other.codePackageInstanceSeqNum_)
    , servicePackageInstanceSeqNum_(other.servicePackageInstanceSeqNum_)
    , servicePackageVersionInstance_(other.servicePackageVersionInstance_)
    , applicationName_(other.applicationName_)
{
}

CodePackageContext::CodePackageContext(CodePackageContext && other)
    : codePackageInstanceId_(move(other.codePackageInstanceId_))
    , codePackageInstanceSeqNum_(other.codePackageInstanceSeqNum_)
    , servicePackageInstanceSeqNum_(other.servicePackageInstanceSeqNum_)
    , servicePackageVersionInstance_(move(other.servicePackageVersionInstance_))
    , applicationName_(move(other.applicationName_))
{
}

CodePackageContext const & CodePackageContext::operator = (CodePackageContext const & other)
{
    if (this != &other)
    {
        this->codePackageInstanceId_ = other.codePackageInstanceId_;
        this->codePackageInstanceSeqNum_ = other.codePackageInstanceSeqNum_;
        this->servicePackageInstanceSeqNum_ = other.servicePackageInstanceSeqNum_;
        this->servicePackageVersionInstance_ = other.servicePackageVersionInstance_;
        this->applicationName_ = other.applicationName_;
    }

    return *this;
}

CodePackageContext const & CodePackageContext::operator = (CodePackageContext && other)
{
    if (this != &other)
    {
        this->codePackageInstanceId_ = move(other.codePackageInstanceId_);
        this->codePackageInstanceSeqNum_ = other.codePackageInstanceSeqNum_;
        this->servicePackageInstanceSeqNum_ = other.servicePackageInstanceSeqNum_;
        this->servicePackageVersionInstance_ = move(other.servicePackageVersionInstance_);
        this->applicationName_ = move(other.applicationName_);
    }

    return *this;
}

bool CodePackageContext::operator == (CodePackageContext const & other) const
{
    bool equals = true;

    equals = (this->codePackageInstanceId_ == other.codePackageInstanceId_);
    if (!equals) { return equals; }

    equals = (this->codePackageInstanceSeqNum_ == other.codePackageInstanceSeqNum_);
    if (!equals) { return equals; }

    equals = (this->servicePackageVersionInstance_ == other.servicePackageVersionInstance_);
    if (!equals) { return equals; }

    equals = (this->servicePackageInstanceSeqNum_ == other.servicePackageInstanceSeqNum_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->applicationName_, other.applicationName_);
    if (!equals) { return equals; }

    return equals;
}

bool CodePackageContext::operator != (CodePackageContext const & other) const
{
    return !(*this == other);
}

void CodePackageContext::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CodePackageContext { ");
    w.Write("CodePackageInstanceId = {0}, ", codePackageInstanceId_);
    w.Write("CodePackageInstanceSeqNum = {0}, ", codePackageInstanceSeqNum_);
    w.Write("ServicePackageInstanceSeqNum = {0}, ", servicePackageInstanceSeqNum_);
    w.Write("ServicePackageVersionInstance = {0}, ", servicePackageVersionInstance_);
    w.Write("ApplicationName = {0} ", applicationName_);
    w.Write("}");
}

ErrorCode CodePackageContext::FromEnvironmentMap(EnvironmentMap const & envMap, __out CodePackageContext & codePackageContext)
{
    auto servicePackageInstanceSeqNumIter = envMap.find(CodePackageContext::EnvVarName_ServicePackageInstanceSeqNum);
    auto codePackageInstanceSeqNumIter = envMap.find(CodePackageContext::EnvVarName_CodePackageInstanceSeqNum);
    auto applicationNameIterator = envMap.find(CodePackageContext::EnvVarName_ApplicationName);

    CodePackageInstanceIdentifier codePackageInstanceId;
    ServicePackageVersionInstance servicePkgVersionInstance;

    ErrorCode codePackageInstanceIdError = CodePackageInstanceIdentifier::FromEnvironmentMap(envMap, codePackageInstanceId);
    ErrorCode packageVersionInstanceError = ServicePackageVersionInstance::FromEnvironmentMap(envMap, servicePkgVersionInstance);
    
    if ((codePackageInstanceSeqNumIter == envMap.end()) &&
        (servicePackageInstanceSeqNumIter == envMap.end()) &&
       (applicationNameIterator == envMap.end()) &&
       (codePackageInstanceIdError.IsError(ErrorCodeValue::NotFound)) &&
       (packageVersionInstanceError.IsError(ErrorCodeValue::NotFound)))
    {
        // CodePackageInstanceSeqNum, ServicePackageInstanceSeqNum, CodePackageInstanceIdentifier and ServicePackageVersionInstance not found
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    if ((codePackageInstanceSeqNumIter != envMap.end()) &&
        (servicePackageInstanceSeqNumIter != envMap.end()) &&
       (applicationNameIterator != envMap.end()) &&
       (codePackageInstanceIdError.IsSuccess()) &&
       (packageVersionInstanceError.IsSuccess()))
    {
        // CodePackageInstanceSeqNum, ServicePackageInstanceSeqNum, CodePackageInstanceIdentifier and ServicePackageVersionInstance found
        int64 codePackageInstanceSeqNum = 0;
        int64 servicePackageInstanceSeqNum = 0;
        if (!StringUtility::TryFromWString<int64>(codePackageInstanceSeqNumIter->second, codePackageInstanceSeqNum))
        {
            return ErrorCode::FromHResult(E_INVALIDARG);
        }
        if (!StringUtility::TryFromWString<int64>(servicePackageInstanceSeqNumIter->second, servicePackageInstanceSeqNum))
        {
            return ErrorCode::FromHResult(E_INVALIDARG);
        }
        else
        {
            codePackageContext = CodePackageContext(
                codePackageInstanceId,
                codePackageInstanceSeqNum,
                servicePackageInstanceSeqNum,
                servicePkgVersionInstance,
                applicationNameIterator->second);
            
            return ErrorCode(ErrorCodeValue::Success);
        }
    }

    // some of the ServicePackageInstanceSeqNum, CodePackageInsatanceIdentifier and ServicePackageVersion found
    return ErrorCode(ErrorCodeValue::InvalidState);
}

void CodePackageContext::ToEnvironmentMap(EnvironmentMap & envMap) const
{
    wstring codePackageInstanceSeqNumStr;
    {
        StringWriter writer(codePackageInstanceSeqNumStr);
        writer.Write("{0}", codePackageInstanceSeqNum_);
        writer.Flush();
    }

    wstring servicePackageInstanceSeqNumStr;
    {
        StringWriter writer(servicePackageInstanceSeqNumStr);
        writer.Write("{0}", servicePackageInstanceSeqNum_);
        writer.Flush();
    }
    
    codePackageInstanceId_.ToEnvironmentMap(envMap);
    servicePackageVersionInstance_.ToEnvironmentMap(envMap);

    envMap[CodePackageContext::EnvVarName_CodePackageInstanceSeqNum] = codePackageInstanceSeqNumStr;
    envMap[CodePackageContext::EnvVarName_ServicePackageInstanceSeqNum] = servicePackageInstanceSeqNumStr;
    envMap[CodePackageContext::EnvVarName_ApplicationName] = applicationName_;
}
