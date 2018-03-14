// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

GlobalWString FabricCodeVersion::Delimiter = make_global<wstring>(L".");

FabricCodeVersion::FabricCodeVersion()
    : majorVersion_(0),
    minorVersion_(0),
    buildVersion_(0),
    hotfixVersion_(0)
{
}

FabricCodeVersion::FabricCodeVersion(
    uint codeVersion, 
    uint configVersion,
    uint buildVersion,
    uint hotfixVersion)
    : majorVersion_(codeVersion),
    minorVersion_(configVersion),
    buildVersion_(buildVersion),
    hotfixVersion_(hotfixVersion)
{
}

FabricCodeVersion::FabricCodeVersion(
    FabricCodeVersion const & other)
    : majorVersion_(other.majorVersion_),
    minorVersion_(other.minorVersion_),
    buildVersion_(other.buildVersion_),
    hotfixVersion_(other.hotfixVersion_)
{
}

FabricCodeVersion::FabricCodeVersion(
    FabricCodeVersion && other)
    : majorVersion_(move(other.majorVersion_)),
    minorVersion_(move(other.minorVersion_)),
    buildVersion_(move(other.buildVersion_)),
    hotfixVersion_(move(other.hotfixVersion_))
{
}

FabricCodeVersion::~FabricCodeVersion() 
{ 
    
}

bool FabricCodeVersion::get_IsValid() const 
{ 
    return (!FabricCodeVersion::Invalid->compare(*this) == 0);
}

bool FabricCodeVersion::IsBaseline() const 
{
    return *this == *Baseline;
}

FabricCodeVersion const & FabricCodeVersion::operator = (FabricCodeVersion const & other)
{
    if (this != &other)
    {
        this->majorVersion_ = other.majorVersion_;
        this->minorVersion_ = other.minorVersion_;
        this->buildVersion_ = other.buildVersion_;
        this->hotfixVersion_ = other.hotfixVersion_;
    }

    return *this;
}

FabricCodeVersion const & FabricCodeVersion::operator = (FabricCodeVersion && other)
{
    if (this != &other)
    {
        this->majorVersion_ = move(other.majorVersion_);
        this->minorVersion_ = move(other.minorVersion_);
        this->buildVersion_ = move(other.buildVersion_);
        this->hotfixVersion_ = move(other.hotfixVersion_);
    }

    return *this;
}

int FabricCodeVersion::compare(FabricCodeVersion const & other) const
{
    if(this->majorVersion_ > other.majorVersion_)
    {
        return 1;
    }
    else if(this->majorVersion_ < other.majorVersion_)
    {
        return -1;
    }
    else
    {
        if(this->minorVersion_ > other.minorVersion_)
        {
            return 1;
        }
        else if(this->minorVersion_ < other.minorVersion_)
        {
            return -1;
        }
        else
        {
            if(this->buildVersion_ > other.buildVersion_)
            {
                return 1;
            }
            else if(this->buildVersion_ < other.buildVersion_)
            {
                return -1;
            }
            else
            {
                if(this->hotfixVersion_ > other.hotfixVersion_)
                {
                    return 1;
                }
                else if(this->hotfixVersion_ < other.hotfixVersion_)
                {
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
        }
    }
}

bool FabricCodeVersion::operator < (FabricCodeVersion const & other) const
{
    return (this->compare(other) < 0);
}

bool FabricCodeVersion::operator == (FabricCodeVersion const & other) const
{
    return (this->compare(other) == 0);
}

bool FabricCodeVersion::operator != (FabricCodeVersion const & other) const
{
    return !(*this == other);
}

void FabricCodeVersion::WriteTo(__in Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}    

wstring FabricCodeVersion::ToString() const
{
    return wformatString("{0}.{1}.{2}.{3}", majorVersion_, minorVersion_, buildVersion_, hotfixVersion_);
}

ErrorCode FabricCodeVersion::FromString(wstring const & fabricCodeVersionString, __out FabricCodeVersion & fabricCodeVersion)
{
    std::vector<wstring> tokens;
    StringUtility::Split<wstring>(fabricCodeVersionString, tokens, Delimiter);

    uint major, minor, build, privateVer;
    if ((tokens.size() == 4) &&
        StringUtility::TryFromWString<uint>(tokens[0], major) &&
        StringUtility::TryFromWString<uint>(tokens[1], minor) &&
        StringUtility::TryFromWString<uint>(tokens[2], build) &&
        StringUtility::TryFromWString<uint>(tokens[3], privateVer))
    {
        fabricCodeVersion = FabricCodeVersion(major, minor, build, privateVer);
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }
}

bool FabricCodeVersion::TryParse(std::wstring const & input, __out FabricCodeVersion & result)
{
    auto error = FabricCodeVersion::FromString(input, result);
    return error.IsSuccess();
}

string FabricCodeVersion::AddField(Common::TraceEvent & traceEvent, string const & name)
{   
    string format = "{0}.{1}.{2}.{3}";
    size_t index = 0;

    traceEvent.AddEventField<uint>(format, name + ".major", index);
    traceEvent.AddEventField<uint>(format, name + ".minor", index);
    traceEvent.AddEventField<uint>(format, name + ".build", index);    
    traceEvent.AddEventField<uint>(format, name + ".private", index);    

    return format;
}

void FabricCodeVersion::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<uint>(majorVersion_);
    context.Write<uint>(minorVersion_);
    context.Write<uint>(buildVersion_);
    context.Write<uint>(hotfixVersion_);
}
