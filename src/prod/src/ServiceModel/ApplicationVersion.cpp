// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

const ApplicationVersion ApplicationVersion::Zero(RolloutVersion(0,0));

const ApplicationVersion ApplicationVersion::Invalid(RolloutVersion::Invalid);

ApplicationVersion::ApplicationVersion()
    : value_()
{
}

ApplicationVersion::ApplicationVersion(
    RolloutVersion const & value)
    : value_(value)
{
}

ApplicationVersion::ApplicationVersion(
    ApplicationVersion const & other)
    : value_(other.value_)
{
}

ApplicationVersion::ApplicationVersion(
    ApplicationVersion && other)
    : value_(move(other.value_))
{
}

ApplicationVersion::~ApplicationVersion() 
{ 
}

ApplicationVersion const & ApplicationVersion::operator = (ApplicationVersion const & other)
{
    if (this != &other)
    {
        this->value_ = other.value_;
    }

    return *this;
}

ApplicationVersion const & ApplicationVersion::operator = (ApplicationVersion && other)
{
    if (this != &other)
    {
        this->value_ = move(other.value_);
    }

    return *this;
}

bool ApplicationVersion::operator == (ApplicationVersion const & other) const
{
    return (this->value_ == other.value_);
}

bool ApplicationVersion::operator != (ApplicationVersion const & other) const
{
    return !(*this == other);
}

int ApplicationVersion::compare(ApplicationVersion const & other) const
{
    return (this->value_.compare(other.value_));
}

bool ApplicationVersion::operator < (ApplicationVersion const & other) const
{
    return (this->compare(other) < 0);
}

void ApplicationVersion::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ApplicationVersion::ToString() const
{
    return value_.ToString();
}

ErrorCode ApplicationVersion::FromString(std::wstring const & applicationVersionString, __out ApplicationVersion & applicationVersion)
{
    RolloutVersion value;
    auto error = RolloutVersion::FromString(applicationVersionString, value);
    if (!error.IsSuccess()) { return error; }

    applicationVersion = ApplicationVersion(value);
    return ErrorCode(ErrorCodeValue::Success);
}
