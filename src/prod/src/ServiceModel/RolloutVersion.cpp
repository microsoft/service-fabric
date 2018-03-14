// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

const RolloutVersion RolloutVersion::Zero(0,0);

RolloutVersion const RolloutVersion::Invalid(0, 0);

RolloutVersion::RolloutVersion()
    : major_(1)
    , minor_(0)
{
}

RolloutVersion::RolloutVersion(
    ULONG major,
    ULONG minor)
    : major_(major)
    , minor_(minor)
{
}

RolloutVersion::RolloutVersion(
    RolloutVersion const & other)
    : major_(other.major_)
    , minor_(other.minor_)
{
}

RolloutVersion::RolloutVersion(
    RolloutVersion && other)
    : major_(other.major_)
    , minor_(other.minor_)
{
}

RolloutVersion::~RolloutVersion() 
{ 
}

RolloutVersion const & RolloutVersion::operator = (RolloutVersion const & other)
{
    if (this != &other)
    {
        this->major_ = other.major_;
        this->minor_ = other.minor_;
    }

    return *this;
}

RolloutVersion const & RolloutVersion::operator = (RolloutVersion && other)
{
    if (this != &other)
    {
        this->major_ = other.major_;
        this->minor_ = other.minor_;
    }

    return *this;
}

bool RolloutVersion::operator == (RolloutVersion const & other) const
{
    return ((this->major_ == other.major_) && 
        (this->minor_ == other.minor_));
}

bool RolloutVersion::operator != (RolloutVersion const & other) const
{
    return !(*this == other);
}

int RolloutVersion::compare(RolloutVersion const & other) const
{
    if (this->major_ < other.major_)
    {
        return -1;
    } 
    else if (this->major_ > other.major_)
    {
        return 1;
    }
    else if (this->minor_ < other.minor_)
    {
        return -1;
    }
    else if (this->minor_ > other.minor_)
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}

bool RolloutVersion::operator < (RolloutVersion const & other) const
{
    if (this->major_ != other.major_)
    {
        return (this->major_ < other.major_);
    }
    else
    {
        return (this->minor_ < other.minor_);
    }
}

void RolloutVersion::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

wstring RolloutVersion::ToString() const
{
    return wformatString("{0}.{1}", major_, minor_);
}

RolloutVersion RolloutVersion::NextMajor() const
{
    return RolloutVersion(major_ + 1, 0);
}

RolloutVersion RolloutVersion::NextMinor() const
{
    return RolloutVersion(major_, minor_ + 1);
}

ErrorCode RolloutVersion::FromString(std::wstring const & rolloutVersionString, __out RolloutVersion & rolloutVersion)
{
    vector<wstring> tokens;  
    StringUtility::Split<wstring>(rolloutVersionString, tokens, L".");

    ULONG majorVersion, minorVersion;    
    if(tokens.size() == 2 && 
       StringUtility::TryFromWString<ULONG>(tokens[0], majorVersion) && 
       StringUtility::TryFromWString<ULONG>(tokens[1], minorVersion))
    {
        rolloutVersion.major_ = majorVersion;
        rolloutVersion.minor_ = minorVersion;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
}
