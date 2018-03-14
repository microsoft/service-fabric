// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "LoadMetric.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

LoadMetric::LoadMetric(std::wstring && name, uint value)
    : name_(move(name)), value_(value)
{
}

LoadMetric::LoadMetric(LoadMetric && other)
    : name_(move(other.name_)), value_(other.value_)
{
}

LoadMetric::LoadMetric(LoadMetric const& other)
    : name_(other.name_), value_(other.value_)
{
}

LoadMetric & LoadMetric::operator = (LoadMetric && other)
{
    if (this != &other) 
    {
        name_ = move(other.name_);
        value_ = other.value_;
    }

    return *this;
}

void LoadMetric::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0}/{1}", name_, value_);
}
