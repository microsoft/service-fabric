// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

CodePackageActivationId::CodePackageActivationId()
    : hostId_(),
    instanceId_(0)
{
}

CodePackageActivationId::CodePackageActivationId(wstring const & hostId)
    : hostId_(hostId),
    instanceId_(DateTime::Now().Ticks)
{
}

CodePackageActivationId::CodePackageActivationId(CodePackageActivationId const & other)
    : hostId_(other.hostId_),
    instanceId_(other.instanceId_)
{
}

CodePackageActivationId::CodePackageActivationId(CodePackageActivationId && other)
    : hostId_(move(other.hostId_)),
    instanceId_(other.instanceId_)
{
}

CodePackageActivationId const & CodePackageActivationId::operator = (CodePackageActivationId const & other)
{
    if (this != &other)
    {
        this->hostId_ = other.hostId_;
        this->instanceId_ = other.instanceId_;
    }

    return *this;
}

CodePackageActivationId const & CodePackageActivationId::operator = (CodePackageActivationId && other)
{
    if (this != &other)
    {
        this->hostId_ = move(other.hostId_);
        this->instanceId_ = other.instanceId_;
    }

    return *this;
}

bool CodePackageActivationId::operator == (CodePackageActivationId const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->hostId_, other.hostId_);
    if (!equals) { return equals; }

    equals = this->instanceId_ == other.instanceId_;
    if (!equals) { return equals; }

    return equals;
}

bool CodePackageActivationId::operator != (CodePackageActivationId const & other) const
{
    return !(*this == other);
}

bool CodePackageActivationId::operator < (CodePackageActivationId const & other) const
{
    int comparision = this->hostId_.compare(other.hostId_);
    if (comparision != 0) { return (comparision < 0); }

    return (this->instanceId_ < other.instanceId_);
}

bool CodePackageActivationId::IsEmpty() const
{
    if (StringUtility::AreEqualCaseInsensitive(this->hostId_, L"") && instanceId_ == 0)
    {
        return true;
    }

    return false;
}

void CodePackageActivationId::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CodePackageActivationId { ");
    w.Write("HostId = {0}, ", HostId);
    w.Write("InstanceId = {0},", InstanceId);
    w.Write("}");
}
