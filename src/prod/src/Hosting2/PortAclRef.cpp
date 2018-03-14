// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceSource("PortAclRef");

PortAclRef::PortAclRef(
    wstring const & principalSid,
    UINT port,
    bool isHttps,
    vector<wstring> const & prefixes)
    : ownerIds_()
    , principalSid_(principalSid)
    , port_(port)
    , isHttps_(isHttps)
    , prefixes_(prefixes)
    , refCount_(0)
    , lock_()
    , closed_(false)
{
}

PortAclRef::~PortAclRef()
{
}

ErrorCode PortAclRef::ConfigurePortAclsIfNeeded(
    wstring const & nodeId,
    wstring const & servicePackageId,
    wstring const & sddl,
    HttpPortAclCallback const & callback)
{
    ErrorCode error(ErrorCodeValue::Success);
    {
        AcquireWriteLock lock(lock_);
        if (closed_)
        {
            return ErrorCodeValue::ObjectClosed;
        }
        if (refCount_ == 0)
        {
            error = callback(sddl, port_, isHttps_, prefixes_);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceSource,
                    "RegisterHttpUrlAcl returned errorcode {0} for port {1}",
                    error,
                    port_);
                closed_ = true;
                return error;
            }
        }
        auto it = ownerIds_.find(nodeId);
        if (it == ownerIds_.end())
        {
            set<wstring> packageIds;
            packageIds.insert(servicePackageId);
            ownerIds_.insert(make_pair(nodeId, packageIds));
        }
        else
        {
            auto iter = it->second.find(servicePackageId);
            if (iter != it->second.end())
            {
                return ErrorCodeValue::Success;
            }
            else
            {
                it->second.insert(servicePackageId);
            }
        }
        refCount_++;
    }
    return error;
}

ErrorCode PortAclRef::RemovePortAclsIfNeeded(
    wstring const & nodeId,
    wstring const & servicePackageId,
    wstring const & sddl,
    HttpPortAclCallback const & callback)
{
    ErrorCode error(ErrorCodeValue::Success);
    AcquireWriteLock lock(lock_);
    
    auto it = ownerIds_.find(nodeId);
    if (it != ownerIds_.end())
    {
        auto it2 = it->second.find(servicePackageId);
        if (it2 != it->second.end())
        {
            it->second.erase(it2);
            refCount_--;
            if (refCount_ == 0)
            {
                error = callback(sddl, port_, isHttps_, prefixes_);
                closed_ = true;
            }
            if (it->second.empty())
            {
                ownerIds_.erase(it);
            }
        }
        else
        {
            error = ErrorCodeValue::NotFound;
        }
    }
    else
    {
        error = ErrorCodeValue::NotFound;
    }
    return error;
}

ErrorCode PortAclRef::RemoveAllForNode(
    wstring const & nodeId,
    wstring const & sddl,
    HttpPortAclCallback const & callback)
{
    ErrorCode error(ErrorCodeValue::Success);
    {
        AcquireWriteLock lock(lock_);
        auto it = ownerIds_.find(nodeId);
        if (it != ownerIds_.end())
        {
            if (!it->second.empty())
            {
                refCount_ -= static_cast<UINT>(it->second.size());
                if (refCount_ == 0 )
                {
                    if (!sddl.empty())
                    {
                        error = callback(sddl, port_, isHttps_, prefixes_);
                    }
                    closed_ = true;
                }
            }
        }
    }
    return error;
}
