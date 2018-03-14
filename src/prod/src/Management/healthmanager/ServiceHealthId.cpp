// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

ServiceHealthId::ServiceHealthId()
    : serviceName_()
{
}

ServiceHealthId::ServiceHealthId(
    std::wstring const & serviceName)
    : serviceName_(serviceName)
{
}

ServiceHealthId::ServiceHealthId(
    std::wstring && serviceName)
    : serviceName_(move(serviceName))
{
}

ServiceHealthId::ServiceHealthId(ServiceHealthId const & other)
    : serviceName_(other.serviceName_)
{
}

ServiceHealthId & ServiceHealthId::operator = (ServiceHealthId const & other)
{
    if (this != &other)
    {
        serviceName_ = other.serviceName_;
    }

    return *this;
}

ServiceHealthId::ServiceHealthId(ServiceHealthId && other)
    : serviceName_(move(other.serviceName_))
{
}

ServiceHealthId & ServiceHealthId::operator = (ServiceHealthId && other)
{
    if (this != &other)
    {
        serviceName_ = move(other.serviceName_);
     }

    return *this;
}

ServiceHealthId::~ServiceHealthId()
{
}

bool ServiceHealthId::operator == (ServiceHealthId const & other) const
{
    return serviceName_ == other.serviceName_;
}

bool ServiceHealthId::operator != (ServiceHealthId const & other) const
{
    return !(*this == other);
}

bool ServiceHealthId::operator < (ServiceHealthId const & other) const
{
    return serviceName_ < other.serviceName_;
}

void ServiceHealthId::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(serviceName_);
}

std::wstring ServiceHealthId::ToString() const
{
    return wformatString(*this);
}
