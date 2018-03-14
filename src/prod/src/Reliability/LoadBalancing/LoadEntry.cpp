// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "LoadEntry.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

LoadEntry::LoadEntry()
    : values_()
{
}

LoadEntry::LoadEntry(size_t metricCount, int64 value)
{
    values_.resize(metricCount, value);
}

LoadEntry::LoadEntry(vector<int64> && values)
    : values_(move(values))
{
}

LoadEntry::LoadEntry(LoadEntry const& other)
    : values_(other.values_)
{
}

LoadEntry::LoadEntry(LoadEntry && other)
    : values_(move(other.values_))
{
}

LoadEntry & LoadEntry::operator = (LoadEntry && other)
{
    if (this != &other)
    {
        values_ = move(other.values_);
    }

    return *this;
}

LoadEntry & LoadEntry::operator +=(LoadEntry const & other)
{
    ASSERT_IFNOT(values_.size() == other.values_.size(), "Cannot add a LoadEntry with different size");

    for (size_t i = 0; i < values_.size(); i++)
    {
        AddLoadValue(values_[i], other.values_[i], i);
    }

    return *this;
}

LoadEntry & LoadEntry::operator -=(LoadEntry const & other)
{
    ASSERT_IFNOT(values_.size() == other.values_.size(), "Cannot subtract a LoadEntry with different size");

    for (size_t i = 0; i < values_.size(); i++)
    {
        AddLoadValue(values_[i], -other.values_[i], i);
    }

    return *this;
}

bool LoadEntry::operator >= (LoadEntry const & other) const
{
    ASSERT_IFNOT(values_.size() == other.values_.size(), "Cannot compare two LoadEntry with different size");
    bool ret = true;
    for (size_t i = 0; i < values_.size(); ++i)
    {
        if (values_[i] < other.values_[i])
        {
            ret = false;
            break;
        }
    }

    return ret;
}

bool LoadEntry::operator <= (LoadEntry const & other) const
{
    ASSERT_IFNOT(values_.size() == other.values_.size(), "Cannot compare two LoadEntry with different size");
    bool ret = true;
    for (size_t i = 0; i < values_.size(); ++i)
    {
        if (values_[i] > other.values_[i])
        {
            ret = false;
            break;
        }
    }

    return ret;
}

bool LoadEntry::operator == (LoadEntry const & other) const
{
    ASSERT_IFNOT(values_.size() == other.values_.size(), "Cannot compare two LoadEntry with different size");
    bool ret = true;
    for (size_t i = 0; i < values_.size(); ++i)
    {
        if (values_[i] != other.values_[i])
        {
            ret = false;
            break;
        }
    }

    return ret;
}

bool LoadEntry::operator != (LoadEntry const & other) const
{
    return !( *this == other);
}

void LoadEntry::WriteTo(TextWriter& writer, FormatOptions const&) const
{
        writer.Write(this->ToString());
}

wstring LoadEntry::ToString() const
{
    wstringstream output;
    for (size_t i = 0; i < values_.size(); ++i)
    {
        output << wformatString("{0}:{1} ", i, values_[i]);
    }
    return output.str();
}
