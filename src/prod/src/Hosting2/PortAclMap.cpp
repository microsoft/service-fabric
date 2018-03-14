// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceSource("PortAclMap");

PortAclMap::PortAclMap()
    : lock_(),
    closed_(false),
    map_()
{
}

PortAclMap::~PortAclMap()
{
}

ErrorCode PortAclMap::GetOrAdd(
    wstring const & principalSid,
    UINT port, 
    bool isHttps,
    vector<wstring> const & prefixes,
    PortAclRefSPtr & portAclRef)
{
    {
        AcquireWriteLock writeLock(lock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(port);
        if (iter != map_.end())
        {
            if (!principalSid.empty() &&
                !StringUtility::AreEqualCaseInsensitive(principalSid, iter->second->PrincipalSid))
            {
                return ErrorCodeValue::AddressAlreadyInUse;
            }
            portAclRef = iter->second;
        }
        else
        {
            portAclRef = make_shared<PortAclRef>(
                principalSid,
                port,
                isHttps, 
                prefixes);
            map_.insert(make_pair(port, portAclRef));
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode PortAclMap::Find(
    wstring const & principalSid,
    UINT port,
    __out PortAclRefSPtr & portAclRef) const
{
    {
        AcquireReadLock readlock(lock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(port);
        if (iter == map_.end() ||
            !StringUtility::AreEqualCaseInsensitive(iter->second->PrincipalSid, principalSid))
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }
         
        portAclRef = iter->second;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode PortAclMap::Remove(UINT port)
{
    {
        AcquireWriteLock writelock(lock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(port);
        if (iter == map_.end())
        {
            WriteInfo(
                TraceSource,
                "Did not find entry for port {0} in portaclmap",
                port);
            return ErrorCode(ErrorCodeValue::NotFound);
        }
         
        map_.erase(iter);        
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode PortAclMap::GetOrAddCertRef(
    UINT port,
    wstring const & principalSid,
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    Common::X509FindType::Enum x509FindType,
    PortCertRefSPtr & portCertRef)
{
    {
        AcquireWriteLock writeLock(certLock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = certRefMap_.find(port);
        if (iter != certRefMap_.end())
        {
            if (!StringUtility::AreEqualCaseInsensitive(x509FindValue, iter->second->CertFindValue))
            {
                return ErrorCode(
                    ErrorCodeValue::AlreadyExists,
                    wformatString(StringResource::Get(IDS_HOSTING_CertificateBindingAlreadyExists), iter->second->CertFindValue, port, x509FindValue));
            }
            portCertRef = iter->second;
        }
        else
        {
            portCertRef = make_shared<PortCertRef>(
                principalSid,
                port,
                x509FindValue,
                x509StoreName,
                x509FindType);
            certRefMap_.insert(make_pair(port, portCertRef));
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode PortAclMap::GetCertRef(
    UINT port,
    wstring const &,
    wstring const & x509FindValue,
    wstring const &,
    Common::X509FindType::Enum,
    PortCertRefSPtr & portCertRef)
{
    {
        AcquireWriteLock writeLock(certLock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = certRefMap_.find(port);
        if (iter != certRefMap_.end())
        {
            if (!StringUtility::AreEqualCaseInsensitive(x509FindValue, iter->second->CertFindValue))
            {
                return ErrorCode(
                    ErrorCodeValue::AlreadyExists,
                    wformatString(StringResource::Get(IDS_HOSTING_CertificateBindingAlreadyExists), iter->second->CertFindValue, port, x509FindValue));
            }
            portCertRef = iter->second;
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode PortAclMap::FindCertRef(
    UINT port,
    __out PortCertRefSPtr & portCertRef) const
{
    {
        AcquireReadLock readlock(certLock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = certRefMap_.find(port);
        if (iter == certRefMap_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        portCertRef = iter->second;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode PortAclMap::RemoveCertRef(UINT port)
{
    {
        AcquireWriteLock writelock(certLock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = certRefMap_.find(port);
        if (iter == certRefMap_.end())
        {
            WriteInfo(
                TraceSource,
                "Did not find entry for port {0} in portcertmap",
                port);
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        certRefMap_.erase(iter);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

vector<PortAclRefSPtr> PortAclMap::Close()
{
     vector<PortAclRefSPtr> retval;

     {
        AcquireWriteLock writelock(lock_);
        if (!closed_)
        {
            for(auto iter = map_.begin(); iter != map_.end(); ++iter)
            {
                retval.push_back(move(iter->second));
            }

            map_.clear();
            closed_ = true;
        }
     }
     return retval;
}

void PortAclMap::GetAllPortRefs(
    vector<PortAclRefSPtr> & portAcls,
    vector<PortCertRefSPtr> & portCertRefs)
{
    {
        AcquireReadLock lock(lock_);
        if (!closed_)
        {
            for (auto iter = map_.begin(); iter != map_.end(); ++iter)
            {
                portAcls.push_back(iter->second);
            }
        }
    }
    {
        AcquireReadLock lock(certLock_);
        if (!closed_)
        {
            for (auto iter = certRefMap_.begin(); iter != certRefMap_.end(); iter++)
            {
                portCertRefs.push_back(iter->second);
            }
        }
    }
}
