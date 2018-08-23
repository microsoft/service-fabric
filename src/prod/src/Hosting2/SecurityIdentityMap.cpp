// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

template SecurityIdentityMap<SecurityGroupSPtr>::SecurityIdentityMap();
template SecurityIdentityMap<SecurityGroupSPtr>::~SecurityIdentityMap<SecurityGroupSPtr>();
template ErrorCode SecurityIdentityMap<SecurityGroupSPtr>::Add(wstring const & principalId, SecurityGroupSPtr const & principal);
template ErrorCode SecurityIdentityMap<SecurityGroupSPtr>::Get( wstring const & principalId, __out SecurityGroupSPtr & principal);
template ErrorCode SecurityIdentityMap<SecurityGroupSPtr>::Contains( wstring const & principalId, __out bool & contains);
template ErrorCode SecurityIdentityMap<SecurityGroupSPtr>::Remove( wstring const & principalId, __out SecurityGroupSPtr & principal);
template vector<SecurityGroupSPtr> SecurityIdentityMap<SecurityGroupSPtr>::Close();
template vector<SecurityGroupSPtr> SecurityIdentityMap<SecurityGroupSPtr>::GetAll();

template SecurityIdentityMap<NodeEnvironmentManagerSPtr>::SecurityIdentityMap();
template SecurityIdentityMap<NodeEnvironmentManagerSPtr>::~SecurityIdentityMap<NodeEnvironmentManagerSPtr>();
template ErrorCode SecurityIdentityMap<NodeEnvironmentManagerSPtr>::Add(wstring const & principalId, NodeEnvironmentManagerSPtr const & principal);
template ErrorCode SecurityIdentityMap<NodeEnvironmentManagerSPtr>::Get( wstring const & principalId, __out NodeEnvironmentManagerSPtr & principal);
template ErrorCode SecurityIdentityMap<NodeEnvironmentManagerSPtr>::Contains( wstring const & principalId, __out bool & contains);
template ErrorCode SecurityIdentityMap<NodeEnvironmentManagerSPtr>::Remove( wstring const & principalId, __out NodeEnvironmentManagerSPtr & principal);
template vector<NodeEnvironmentManagerSPtr> SecurityIdentityMap<NodeEnvironmentManagerSPtr>::Close();
template vector<NodeEnvironmentManagerSPtr> SecurityIdentityMap<NodeEnvironmentManagerSPtr>::GetAll();

template SecurityIdentityMap<ActivatorRequestorSPtr>::SecurityIdentityMap();
template SecurityIdentityMap<ActivatorRequestorSPtr>::~SecurityIdentityMap<ActivatorRequestorSPtr>();
template ErrorCode SecurityIdentityMap<ActivatorRequestorSPtr>::Add(wstring const & principalId, ActivatorRequestorSPtr const & principal);
template ErrorCode SecurityIdentityMap<ActivatorRequestorSPtr>::Get( wstring const & principalId, __out ActivatorRequestorSPtr & principal);
template ErrorCode SecurityIdentityMap<ActivatorRequestorSPtr>::Contains( wstring const & principalId, __out bool & contains);
template ErrorCode SecurityIdentityMap<ActivatorRequestorSPtr>::Remove( wstring const & principalId, __out ActivatorRequestorSPtr & principal);
template vector<ActivatorRequestorSPtr> SecurityIdentityMap<ActivatorRequestorSPtr>::Close();
template vector<ActivatorRequestorSPtr> SecurityIdentityMap<ActivatorRequestorSPtr>::GetAll();

template <class T>
SecurityIdentityMap<T>::SecurityIdentityMap()
    : lock_(),
    map_(),
    isClosed_(false)
{
}

template <class T>
SecurityIdentityMap<T>::~SecurityIdentityMap()
{
}

template <class T>
ErrorCode SecurityIdentityMap<T>::Add(
    wstring const & principalId,
    T const & principal)
{
    {
        AcquireWriteLock writeLock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(principalId);
        if (iter != map_.end())
        {
            return ErrorCode(ErrorCodeValue::AlreadyExists);
        }
        else
        {
            map_.insert(make_pair(principalId, principal));
        }
    }
    return ErrorCode(ErrorCodeValue::Success);
}

template <class T>
ErrorCode SecurityIdentityMap<T>::Contains(wstring const & principalId, __out bool & contains)
{
    T principal;
    auto error = this->Get(principalId, principal);
    if (error.IsSuccess())
    {
        contains = true;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        contains = false;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        contains = false;
        return error;
    }
}

template <class T>
ErrorCode SecurityIdentityMap<T>::Get(
    wstring const & principalId, 
    __out T & principal)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(principalId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }
        principal = iter->second;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

template <class T>
ErrorCode SecurityIdentityMap<T>::Remove(
    wstring const & principalId,
    __out T & obj)
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(principalId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }
        obj = iter->second;
        map_.erase(iter);
    }
    return ErrorCode(ErrorCodeValue::Success);
}

template <class T>
vector<T> SecurityIdentityMap<T>::Close()
{
    vector<T> retval;

    {
        AcquireWriteLock writelock(lock_);
        if (!isClosed_)
        {
            for(auto i = map_.begin(); i != map_.end(); ++i)
            {
                retval.push_back(move(i->second));
            }

            map_.clear();
            isClosed_ = true;
        }
    }

    return retval;
}

template <class T>
vector<T> SecurityIdentityMap<T>::GetAll()
{
    vector<T> retval;

    {
        AcquireReadLock lock(lock_);
        if (!isClosed_)
        {
            for (auto i = map_.begin(); i != map_.end(); ++i)
            {
                retval.push_back(i->second);
            }
        }
    }

    return retval;
}