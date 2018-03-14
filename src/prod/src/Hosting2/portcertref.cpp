// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceSource("PortCertRef");

PortCertRef::PortCertRef(
    wstring const & principalSid,
    UINT port,
    std::wstring const & x509FindValue,
    std::wstring const & x509StoreName,
    Common::X509FindType::Enum x509FindType)
    : ownerIds_()
    , principalSid_(principalSid)
    , port_(port)
    , x509FindValue_(x509FindValue)
    , x509StoreName_(x509StoreName)
    , x509FindType_(x509FindType)
    , refCount_(0)
    , lock_()
    , closed_(false)
{
}

PortCertRef::~PortCertRef()
{
}

ErrorCode PortCertRef::ConfigurePortCertIfNeeded(
    wstring const & nodeId,
    wstring const & servicePackageId,
    PortCertificateBindingCallback const & callback)
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
            error = callback(port_, x509FindValue_, x509StoreName_, x509FindType_, principalSid_);
            WriteTrace(
                error.ToLogLevel(),
                TraceSource,
                "HttpCertCfg returned errorcode {0} for port {1}",
                error,
                port_);
            if (!error.IsSuccess())
            {
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
            WriteInfo(
                TraceSource,
                "Port already configured with cert. Adding servicepackage ref port {1}, servicepackageId {1}",
                port_,
                servicePackageId);
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

ErrorCode PortCertRef::RemovePortCertIfNeeded(
    wstring const & nodeId,
    wstring const & servicePackageId,
    PortCertificateBindingCallback const & callback)
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
                error = callback(port_, x509FindValue_, x509StoreName_, x509FindType_, principalSid_);
                closed_ = true;
                WriteTrace(
                    error.ToLogLevel(),
                    TraceSource,
                    "HttpCertCfg cleanup returned errorcode {0} for port {1}",
                    error,
                    port_);
            }
            else
            {
                WriteInfo(
                    TraceSource,
                    "Decremented portcert ref for port {0} to refcount {1}, removing servicepackage {2}",
                    port_,
                    refCount_,
                    servicePackageId);
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

ErrorCode PortCertRef::RemoveAllForNode(
    wstring const & nodeId,
    PortCertificateBindingCallback const & certCallback)
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
                if (refCount_ == 0)
                {
                    error = certCallback(port_, x509FindValue_, x509StoreName_, x509FindType_, principalSid_);
                }
            }
        }
        if (refCount_ == 0)
        {
            closed_ = true;
        }
    }
    return error;
}
