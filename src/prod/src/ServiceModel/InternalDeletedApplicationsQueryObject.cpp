// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

InternalDeletedApplicationsQueryObject::InternalDeletedApplicationsQueryObject()
    : applicationIds_()
{
}

InternalDeletedApplicationsQueryObject::InternalDeletedApplicationsQueryObject(
    std::vector<std::wstring> const & applicationIds)
    : applicationIds_(applicationIds)
{
}

InternalDeletedApplicationsQueryObject::InternalDeletedApplicationsQueryObject(
    InternalDeletedApplicationsQueryObject && other) 
    : applicationIds_(move(other.applicationIds_))
{
}

InternalDeletedApplicationsQueryObject const & InternalDeletedApplicationsQueryObject::operator= (InternalDeletedApplicationsQueryObject && other)
{
    if(this != &other)
    {
        this->applicationIds_ = move(other.applicationIds_);
    }
    return *this;
}

void InternalDeletedApplicationsQueryObject::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("ApplicationIds { ");
    for(auto iter = applicationIds_.begin(); iter != applicationIds_.end(); ++iter)
    {
        w.Write("ApplicationId = {0}", *iter);
    }
    w.Write("}");
}
