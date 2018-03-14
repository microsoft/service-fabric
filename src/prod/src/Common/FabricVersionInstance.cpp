// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

GlobalWString FabricVersionInstance::Delimiter = make_global<wstring>(L":");

FabricVersionInstance::FabricVersionInstance()
    : version_()
    , instanceId_(0)
{
}

FabricVersionInstance::FabricVersionInstance(
    FabricVersion const & fabricVersion, 
    uint64 instanceId)
    : version_(fabricVersion)
    , instanceId_(instanceId)
{
}

FabricVersionInstance::FabricVersionInstance(
    FabricVersionInstance const & other)
    : version_(other.version_)
    , instanceId_(other.instanceId_)
{
}

FabricVersionInstance::FabricVersionInstance(
    FabricVersionInstance && other)
    : version_(move(other.version_))
    , instanceId_(other.instanceId_)
{
}

FabricVersionInstance::~FabricVersionInstance() 
{ 
}

FabricVersionInstance const & FabricVersionInstance::operator = (FabricVersionInstance const & other)
{
    if (this != &other)
    {
        this->version_ = other.version_;
        this->instanceId_ = other.instanceId_;
    }

    return *this;
}

FabricVersionInstance const & FabricVersionInstance::operator = (FabricVersionInstance && other)
{
    if (this != &other)
    {
        this->version_ = move(other.version_);
        this->instanceId_ = other.instanceId_;
    }

    return *this;
}

bool FabricVersionInstance::operator == (FabricVersionInstance const & other) const
{
    return (this->version_ == other.version_ && this->instanceId_ == other.instanceId_);
}

bool FabricVersionInstance::operator != (FabricVersionInstance const & other) const
{
    return !(*this == other);
}

int FabricVersionInstance::compare(FabricVersionInstance const & other) const
{
    int result = this->version_.compare(other.version_);

    if (this->instanceId_ > other.instanceId_)
    {
        result = 1;
    }
    else if (this->instanceId_ < other.instanceId_)
    {
        result = -1;
    }
    else 
    {
        result = 0;
    }

    return result;
}

bool FabricVersionInstance::operator < (FabricVersionInstance const & other) const
{
    return (this->compare(other) < 0);
}

void FabricVersionInstance::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

wstring FabricVersionInstance::ToString() const
{
    return wformatString("{0}{1}{2}", version_.ToString(), Delimiter, instanceId_); 
}

ErrorCode FabricVersionInstance::FromString(wstring const & fabricVersionInstanceString, __out FabricVersionInstance & fabricVersionInstance)
{
    vector<wstring> tokens;  
    StringUtility::Split<wstring>(fabricVersionInstanceString, tokens, Delimiter);
    
    uint64 instanceId;
    if((tokens.size() != 3) || 
       (!StringUtility::TryFromWString<uint64>(tokens[2], instanceId)))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    FabricCodeVersion fabricCodeVersion;
    auto error = FabricCodeVersion::FromString(tokens[0], fabricCodeVersion);
    if (!error.IsSuccess())
    {
        return error;
    }

    FabricConfigVersion fabricConfigVersion(tokens[1]);

    fabricVersionInstance = FabricVersionInstance(FabricVersion(fabricCodeVersion, fabricConfigVersion), instanceId);
    return ErrorCode(ErrorCodeValue::Success);
}

string FabricVersionInstance::AddField(Common::TraceEvent & traceEvent, string const & name)
{   
    string format = "{0}:{1}";
    size_t index = 0;

    traceEvent.AddEventField<FabricVersion>(format, name + ".version", index);
    traceEvent.AddEventField<uint64>(format, name + ".instanceId", index);

    return format;
}

void FabricVersionInstance::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<FabricVersion>(version_);
    context.Write<uint64>(instanceId_);
}
