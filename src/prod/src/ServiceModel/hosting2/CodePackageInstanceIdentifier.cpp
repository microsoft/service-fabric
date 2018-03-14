// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

GlobalWString CodePackageInstanceIdentifier::Delimiter = make_global<wstring>(L"#");
GlobalWString CodePackageInstanceIdentifier::EnvVarName_CodePackageName = make_global<wstring>(L"Fabric_CodePackageName");

CodePackageInstanceIdentifier::CodePackageInstanceIdentifier()
    : servicePackageInstanceId_()
    , codePackageName_()
{
}

CodePackageInstanceIdentifier::CodePackageInstanceIdentifier(
    ServicePackageInstanceIdentifier const& servicePackageInstanceId,
    wstring const & codePackageName)
    : servicePackageInstanceId_(servicePackageInstanceId)
    , codePackageName_(codePackageName)
{
}

CodePackageInstanceIdentifier::CodePackageInstanceIdentifier(CodePackageInstanceIdentifier const & other)
    : servicePackageInstanceId_(other.servicePackageInstanceId_)
    , codePackageName_(other.codePackageName_)
{
}

CodePackageInstanceIdentifier::CodePackageInstanceIdentifier(CodePackageInstanceIdentifier && other)
    : servicePackageInstanceId_(move(other.servicePackageInstanceId_))
    , codePackageName_(move(other.codePackageName_))
{
}

CodePackageInstanceIdentifier const & CodePackageInstanceIdentifier::operator = (CodePackageInstanceIdentifier const & other)
{
    if (this != &other)
    {
        this->servicePackageInstanceId_ = other.servicePackageInstanceId_;
        this->codePackageName_ = other.codePackageName_;
    }

    return *this;
}

CodePackageInstanceIdentifier const & CodePackageInstanceIdentifier::operator = (CodePackageInstanceIdentifier && other)
{
    if (this != &other)
    {
        this->servicePackageInstanceId_ = move(other.servicePackageInstanceId_);
        this->codePackageName_ = move(other.codePackageName_);
    }

    return *this;
}

bool CodePackageInstanceIdentifier::operator == (CodePackageInstanceIdentifier const & other) const
{
    bool equals = true;

    equals = this->servicePackageInstanceId_ == other.servicePackageInstanceId_;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->codePackageName_, other.codePackageName_);
    if (!equals) { return equals; }

    return equals;
}

bool CodePackageInstanceIdentifier::operator != (CodePackageInstanceIdentifier const & other) const
{
    return !(*this == other);
}

int CodePackageInstanceIdentifier::compare(CodePackageInstanceIdentifier const & other) const
{
    int comparision = this->servicePackageInstanceId_.compare(other.servicePackageInstanceId_);
    if (comparision != 0) { return comparision; }

    comparision = StringUtility::CompareCaseInsensitive(this->codePackageName_, other.codePackageName_);
    if (comparision != 0) { return comparision; }

    return comparision;
}

bool CodePackageInstanceIdentifier::operator < (CodePackageInstanceIdentifier const & other) const
{
    return (this->compare(other) < 0);
}

void CodePackageInstanceIdentifier::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CodePackageInstanceIdentifier { ");
    w.Write("ServicePackageInstanceId = {0}, ", ServicePackageInstanceId);
    w.Write("CodePackageName = {0} ", CodePackageName);
    w.Write("}");
}

wstring CodePackageInstanceIdentifier::ToString() const
{
    return  wformatString("{0}{1}{2}", ServicePackageInstanceId.ToString(), Delimiter, CodePackageName);
}

ErrorCode CodePackageInstanceIdentifier::FromString(
    wstring const & codePackageInstanceIdStr,
    __out CodePackageInstanceIdentifier & codePackageInstanceId)
{
    if (codePackageInstanceIdStr.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    vector<wstring> tokens;
    StringUtility::Split(codePackageInstanceIdStr, tokens, *Delimiter);

    if (tokens.size() != 2)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    ServicePackageInstanceIdentifier servicePackageInstanceId;
    auto error = ServicePackageInstanceIdentifier::FromString(tokens[0], servicePackageInstanceId);
    if (!error.IsSuccess())
    {
        return error;
    }

    codePackageInstanceId = CodePackageInstanceIdentifier(servicePackageInstanceId, tokens[1]);

    return error;
}

ErrorCode CodePackageInstanceIdentifier::FromEnvironmentMap(EnvironmentMap const& envMap, __out CodePackageInstanceIdentifier & codePackageInstanceIdentifier)
{
    auto codePackageNameIterator = envMap.find(CodePackageInstanceIdentifier::EnvVarName_CodePackageName);

    ServicePackageInstanceIdentifier servicePackageActivationIdentifier;
    ErrorCode error = ServicePackageInstanceIdentifier::FromEnvironmentMap(envMap, servicePackageActivationIdentifier);

    if (error.ReadValue() == ErrorCodeValue::NotFound && codePackageNameIterator == envMap.end())
    {
        //ServicePackageInstanceIdentifier and CodePackageName both not found so returning NotFound error
        return ErrorCode(ErrorCodeValue::NotFound);
    }
    if (error.ReadValue() == ErrorCodeValue::Success && codePackageNameIterator == envMap.end())
    {
        //ServicePackageInstanceIdentifier Found but CodePackageName not found
        return ErrorCode(ErrorCodeValue::InvalidState);
    }
    else if (!error.IsSuccess())
    {
        return error;
    }

    ASSERT_IF(codePackageNameIterator == envMap.end(), "codePackageNameIterator should not be invalid here");
    codePackageInstanceIdentifier = CodePackageInstanceIdentifier(servicePackageActivationIdentifier, codePackageNameIterator->second);

    return ErrorCode(ErrorCodeValue::Success);
}

void CodePackageInstanceIdentifier::ToEnvironmentMap(EnvironmentMap & envMap) const
{
    envMap[CodePackageInstanceIdentifier::EnvVarName_CodePackageName] = codePackageName_;
    servicePackageInstanceId_.ToEnvironmentMap(envMap);
}
