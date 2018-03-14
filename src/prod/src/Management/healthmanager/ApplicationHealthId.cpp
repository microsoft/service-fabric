// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

ApplicationHealthId::ApplicationHealthId()
    : applicationName_()
{
}

ApplicationHealthId::ApplicationHealthId(
    std::wstring const & applicationName)
    : applicationName_(applicationName)
{
}

ApplicationHealthId::ApplicationHealthId(
    std::wstring && applicationName)
    : applicationName_(move(applicationName))
{
}

ApplicationHealthId::ApplicationHealthId(ApplicationHealthId const & other)
    : applicationName_(other.applicationName_)
{
}

ApplicationHealthId & ApplicationHealthId::operator = (ApplicationHealthId const & other)
{
    if (this != &other)
    {
        applicationName_ = other.applicationName_;
    }

    return *this;
}

ApplicationHealthId::ApplicationHealthId(ApplicationHealthId && other)
    : applicationName_(move(other.applicationName_))
{
}

ApplicationHealthId & ApplicationHealthId::operator = (ApplicationHealthId && other)
{
    if (this != &other)
    {
        applicationName_ = move(other.applicationName_);
     }

    return *this;
}

ApplicationHealthId::~ApplicationHealthId()
{
}

bool ApplicationHealthId::operator == (ApplicationHealthId const & other) const
{
    return applicationName_ == other.applicationName_;
}

bool ApplicationHealthId::operator != (ApplicationHealthId const & other) const
{
    return !(*this == other);
}

bool ApplicationHealthId::operator < (ApplicationHealthId const & other) const
{
    return applicationName_ < other.applicationName_;
}

void ApplicationHealthId::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(applicationName_);
}

std::wstring ApplicationHealthId::ToString() const
{
    return wformatString(*this);
}
