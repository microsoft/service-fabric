// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationNameQueryResult::ApplicationNameQueryResult()
    : applicationName_()
{
}

ApplicationNameQueryResult::ApplicationNameQueryResult(Uri const & applicationName)
    : applicationName_(applicationName)
{
}

ApplicationNameQueryResult::ApplicationNameQueryResult(ApplicationNameQueryResult const & other)
    : applicationName_(other.applicationName_)
{
}

ApplicationNameQueryResult::ApplicationNameQueryResult(ApplicationNameQueryResult && other)
    : applicationName_(move(other.applicationName_))
{
}

ApplicationNameQueryResult const & ApplicationNameQueryResult::operator = (ApplicationNameQueryResult const & other)
{
    if (this != &other)
    {
        this->applicationName_ = other.applicationName_;
    }

    return *this;
}

ApplicationNameQueryResult const & ApplicationNameQueryResult::operator = (ApplicationNameQueryResult && other)
{
    if (this != &other)
    {
        this->applicationName_ = move(other.applicationName_);
    }

    return *this;
}

void ApplicationNameQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_APPLICATION_NAME_QUERY_RESULT & publicQueryResult) const 
{
    publicQueryResult.ApplicationName = heap.AddString(applicationName_.ToString());
}

ErrorCode ApplicationNameQueryResult::FromPublicApi(__in FABRIC_APPLICATION_NAME_QUERY_RESULT const& publicQueryResult)
{
    if (!Uri::TryParse(publicQueryResult.ApplicationName, applicationName_))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode::Success();
}

void ApplicationNameQueryResult::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ApplicationNameQueryResult::ToString() const
{
    return wformatString("ApplicationName = '{0}'", applicationName_.ToString()); 
}
