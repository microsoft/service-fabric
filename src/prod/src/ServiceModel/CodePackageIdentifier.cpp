// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

GlobalWString CodePackageIdentifier::EnvVarName_CodePackageName = make_global<wstring>(L"Fabric_CodePackageName");

CodePackageIdentifier::CodePackageIdentifier()
    : servicePackageId_(),
    codePackageName_()
{
}

CodePackageIdentifier::CodePackageIdentifier(
    ServicePackageIdentifier const& servicePackageId, 
    wstring const & codePackageName)
    : servicePackageId_(servicePackageId),
    codePackageName_(codePackageName)
{
}

CodePackageIdentifier::CodePackageIdentifier(CodePackageIdentifier const & other)
    : servicePackageId_(other.servicePackageId_),
    codePackageName_(other.codePackageName_)
{
}

CodePackageIdentifier::CodePackageIdentifier(CodePackageIdentifier && other)
    : servicePackageId_(move(other.servicePackageId_)),
    codePackageName_(move(other.codePackageName_))
{
}

CodePackageIdentifier const & CodePackageIdentifier::operator = (CodePackageIdentifier const & other)
{
    if (this != &other)
    {
        this->servicePackageId_ = other.servicePackageId_;
        this->codePackageName_ = other.codePackageName_;
    }

    return *this;
}

CodePackageIdentifier const & CodePackageIdentifier::operator = (CodePackageIdentifier && other)
{
    if (this != &other)
    {
        this->servicePackageId_ = move(other.servicePackageId_);
        this->codePackageName_ = move(other.codePackageName_);
    }

    return *this;
}

bool CodePackageIdentifier::operator == (CodePackageIdentifier const & other) const
{
    bool equals = true;

    equals = this->servicePackageId_ == other.servicePackageId_;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->codePackageName_, other.codePackageName_);
    if (!equals) { return equals; }

    return equals;
}

bool CodePackageIdentifier::operator != (CodePackageIdentifier const & other) const
{
    return !(*this == other);
}

int CodePackageIdentifier::compare(CodePackageIdentifier const & other) const
{
    int comparision = this->servicePackageId_.compare(other.servicePackageId_);
    if (comparision != 0) { return comparision; }

    comparision = StringUtility::CompareCaseInsensitive(this->codePackageName_, other.codePackageName_);
    if (comparision != 0) { return comparision; }

    return comparision;
}

bool CodePackageIdentifier::operator < (CodePackageIdentifier const & other) const
{
    return (this->compare(other) < 0);
}

void CodePackageIdentifier::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CodePackageIdentifier { ");
    w.Write("ServicePackageId = {0}, ", ServicePackageId);
    w.Write("CodePackageName = {0} ", CodePackageName);
    w.Write("}");
}

wstring CodePackageIdentifier::ToString() const
{
    return (ServicePackageId.ToString() + L":" + CodePackageName);
}

ErrorCode CodePackageIdentifier::FromEnvironmentMap(EnvironmentMap const& envMap, __out CodePackageIdentifier & codePackageIdentifier)
{
    auto codePackageNameIterator = envMap.find(CodePackageIdentifier::EnvVarName_CodePackageName);

    ServicePackageIdentifier servicePackageIdentifier;
    ErrorCode error = ServicePackageIdentifier::FromEnvironmentMap(envMap, servicePackageIdentifier);

    if(error.ReadValue() == ErrorCodeValue::NotFound && codePackageNameIterator == envMap.end())
    {
        //ServicePackageIdentifier and CodePackageName both not found so returning NotFound error
        return ErrorCode(ErrorCodeValue::NotFound);
    }
    if(error.ReadValue() == ErrorCodeValue::Success && codePackageNameIterator == envMap.end())
    {
        //ServicePackageIdentifier Found but CodePackageName not found
        return ErrorCode(ErrorCodeValue::InvalidState);
    }
    else if (!error.IsSuccess())
    {
        return error;
    }

    ASSERT_IF(codePackageNameIterator == envMap.end(), "codePackageNameIterator should not be invalid here");
    codePackageIdentifier = CodePackageIdentifier(servicePackageIdentifier, codePackageNameIterator->second);

    return ErrorCode(ErrorCodeValue::Success);
}

void CodePackageIdentifier::ToEnvironmentMap(EnvironmentMap & envMap) const
{
    envMap[CodePackageIdentifier::EnvVarName_CodePackageName] = codePackageName_;
    servicePackageId_.ToEnvironmentMap(envMap);
}
