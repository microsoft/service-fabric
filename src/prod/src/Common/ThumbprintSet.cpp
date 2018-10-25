// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace std;

ErrorCode ThumbprintSet::Add(wstring const & thumbprintString)
{
    Thumbprint thumbprint;
    auto error = thumbprint.Initialize(thumbprintString);
    if (!error.IsSuccess())
    {
        return error;
    }

    Add(thumbprint);
    return error;
}

ErrorCode ThumbprintSet::Add(LPCWSTR thumbprintptr)
{
    wstring thumbprintString;
    auto hr = StringUtility::LpcwstrToWstring(thumbprintptr, false, thumbprintString);
    if (FAILED(hr)) 
    {
        return ErrorCode::FromHResult(hr); 
    }

    auto error = Add(thumbprintString);
    return error;
}

void ThumbprintSet::Add(Thumbprint const& thumbprint)
{
    value_.emplace(move(thumbprint));
}

void ThumbprintSet::Add(ThumbprintSet const & other)
{
    value_.insert(other.value_.cbegin(), other.value_.cend());
}

ErrorCode ThumbprintSet::Set(wstring const & thumbprints)
{
    value_.clear();

    auto thumbprintsCopy(thumbprints);
    StringUtility::TrimWhitespaces(thumbprintsCopy);
    if (thumbprintsCopy.empty())
    {
        return ErrorCode();
    }

    vector<wstring> thumbprintList;
    StringUtility::Split<wstring>(thumbprintsCopy, thumbprintList, L",", true);
    if (thumbprintList.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    for (auto & thumbprintString : thumbprintList)
    {
        StringUtility::TrimWhitespaces(thumbprintString);
        ErrorCode error = Add(thumbprintString);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCode();
}

set<Thumbprint> const & ThumbprintSet::Value() const
{
    return value_;
}

bool ThumbprintSet::Contains(Common::Thumbprint const & thumbprint) const
{
    return value_.find(thumbprint) != value_.cend();
}

bool ThumbprintSet::Contains(ThumbprintSet const & other) const
{
    for (auto const & itemInOther : other.value_)
    {
        if (!Contains(itemInOther))
        {
            return false;
        }
    }

    return true;
}

bool ThumbprintSet::operator == (ThumbprintSet const & rhs) const
{
    return (value_.size() == rhs.value_.size()) && this->Contains(rhs) && rhs.Contains(*this);
}

bool ThumbprintSet::operator != (ThumbprintSet const & rhs) const
{
    return !(*this == rhs);
}

void ThumbprintSet::WriteTo(TextWriter & w, FormatOptions const &) const
{
    bool outputGenerated = false;
    for (auto const & thumbprint : value_)
    {
        if (outputGenerated)
        {
            w.Write(",{0}", thumbprint);
        }
        else
        {
            w.Write("{0}", thumbprint);
            outputGenerated = true;
        }
    }
}

wstring ThumbprintSet::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}
