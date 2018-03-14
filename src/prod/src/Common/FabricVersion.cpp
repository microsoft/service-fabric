// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

GlobalWString FabricVersion::Delimiter = make_global<wstring>(L":");

FabricVersion::FabricVersion()
    : codeVersion_(),
    configVersion_()
{
}

FabricVersion::FabricVersion(
    FabricCodeVersion const & codeVersion, 
    FabricConfigVersion const & configVersion)
    : codeVersion_(codeVersion),
    configVersion_(configVersion)
{
}

FabricVersion::FabricVersion(
    FabricVersion const & other)
    : codeVersion_(other.codeVersion_),
    configVersion_(other.configVersion_)
{
}

FabricVersion::FabricVersion(
    FabricVersion && other)
    : codeVersion_(move(other.codeVersion_)),
    configVersion_(move(other.configVersion_))
{
}

FabricVersion::~FabricVersion() 
{ 
}

FabricVersion const & FabricVersion::operator = (FabricVersion const & other)
{
    if (this != &other)
    {
        this->codeVersion_ = other.codeVersion_;
        this->configVersion_ = other.configVersion_;
    }

    return *this;
}

FabricVersion const & FabricVersion::operator = (FabricVersion && other)
{
    if (this != &other)
    {
        this->codeVersion_ = move(other.codeVersion_);
        this->configVersion_ = move(other.configVersion_);
    }

    return *this;
}

bool FabricVersion::operator == (FabricVersion const & other) const
{
    return ((this->codeVersion_.compare(other.codeVersion_) == 0) && 
            (this->configVersion_.compare(other.configVersion_) == 0));
}

bool FabricVersion::operator != (FabricVersion const & other) const
{
    return !(*this == other);
}

int FabricVersion::compare(FabricVersion const & other) const
{
    int result = this->codeVersion_.compare(other.codeVersion_);

    if (result == 0)
    {
        result = this->configVersion_.compare(other.configVersion_);
    }

    return result;
}

bool FabricVersion::operator < (FabricVersion const & other) const
{
    return (this->compare(other) < 0);
}

void FabricVersion::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

wstring FabricVersion::ToString() const
{
    wstring result(codeVersion_.ToString());
    result.append(Delimiter);
    result.append(configVersion_.ToString());

    return result;
}

ErrorCode FabricVersion::FromString(wstring const & fabricVersionString, __out FabricVersion & fabricVersion)
{
    vector<wstring> tokens;  
    StringUtility::Split<wstring>(fabricVersionString, tokens, Delimiter);

    if(tokens.size() != 2)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    FabricCodeVersion fabricCodeVersion;
    auto error = FabricCodeVersion::FromString(tokens[0], fabricCodeVersion);
    if (!error.IsSuccess())
    {
        return error;
    }

    FabricConfigVersion fabricConfigVersion;
    error = FabricConfigVersion::FromString(tokens[1], fabricConfigVersion);
    if (!error.IsSuccess())
    {
        return error;
    }

    fabricVersion = FabricVersion(fabricCodeVersion, fabricConfigVersion);
    return ErrorCode(ErrorCodeValue::Success);
}

bool FabricVersion::TryParse(std::wstring const & input, __out FabricVersion & fabricVersion)
{
    auto error = FabricVersion::FromString(input, fabricVersion);
    return error.IsSuccess();
}

string FabricVersion::AddField(Common::TraceEvent & traceEvent, string const & name)
{   
    string format = "{0}:{1}";
    size_t index = 0;

    traceEvent.AddEventField<FabricCodeVersion>(format, name + ".code", index);
    traceEvent.AddEventField<FabricConfigVersion>(format, name + ".config", index);

    return format;
}

void FabricVersion::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<FabricCodeVersion>(codeVersion_);
    context.Write<FabricConfigVersion>(configVersion_);
}
