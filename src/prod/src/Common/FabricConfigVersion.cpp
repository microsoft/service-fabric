// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceCategory("FabricConfigVersion");

FabricConfigVersion::FabricConfigVersion()
    : value_()
{
}

FabricConfigVersion::FabricConfigVersion(
    wstring const & value)
    : value_(value)
{
}

FabricConfigVersion::FabricConfigVersion(
    FabricConfigVersion const & other)
    : value_(other.value_)
{
}

FabricConfigVersion::FabricConfigVersion(
    FabricConfigVersion && other)
    : value_(move(other.value_))
{
}

FabricConfigVersion::~FabricConfigVersion() 
{ 
}

FabricConfigVersion const & FabricConfigVersion::operator = (FabricConfigVersion const & other)
{
    if (this != &other)
    {
        this->value_ = other.value_;
    }

    return *this;
}

FabricConfigVersion const & FabricConfigVersion::operator = (FabricConfigVersion && other)
{
    if (this != &other)
    {
        this->value_ = move(other.value_);
    }

    return *this;
}

int FabricConfigVersion::compare(FabricConfigVersion const & other) const
{
    return StringUtility::CompareCaseInsensitive(this->value_, other.value_);
}

bool FabricConfigVersion::operator < (FabricConfigVersion const & other) const
{
    return (this->compare(other) < 0);
}

bool FabricConfigVersion::operator == (FabricConfigVersion const & other) const
{
    return (this->compare(other) == 0);
}

bool FabricConfigVersion::operator != (FabricConfigVersion const & other) const
{
    return !(*this == other);
}

void FabricConfigVersion::WriteTo(__in Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

wstring FabricConfigVersion::ToString() const
{
    return value_;
}

ErrorCode FabricConfigVersion::FromString(wstring const & input)
{
    if (StringUtility::Contains<wstring>(input, FabricVersion::Delimiter))
    {
        auto msg = wformatString(GET_COMMON_RC(Invalid_FabricVersion), input, FabricVersion::Delimiter);

        Trace.WriteWarning(TraceCategory, "{0}", msg);

        return ErrorCode(ErrorCodeValue::InvalidArgument, move(msg));
    }
    else
    {
        value_ = input;
        return ErrorCodeValue::Success;
    }
}

ErrorCode FabricConfigVersion::FromString(wstring const & input, __out FabricConfigVersion & result)
{
    FabricConfigVersion configVersion;
    auto error = configVersion.FromString(input);
    if (error.IsSuccess())
    {
        result = move(configVersion);
    }

    return error;
}

bool FabricConfigVersion::TryParse(std::wstring const & input, __out FabricConfigVersion & result)
{
    auto error = FabricConfigVersion::FromString(input, result);
    return error.IsSuccess();
}

string FabricConfigVersion::AddField(Common::TraceEvent & traceEvent, string const & name)
{   
    string format = "{0}";
    size_t index = 0;

    traceEvent.AddEventField<wstring>(format, name + ".value", index);

    return format;
}

void FabricConfigVersion::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<wstring>(value_);
}
