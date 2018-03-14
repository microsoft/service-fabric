// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ServicePackage2Map::ServicePackage2Map()
    : lock_()
	, closed_(false)
	, map_()
	, activationIdToContextMap_()
{
}

ServicePackage2Map::~ServicePackage2Map()
{
}

vector<ServicePackage2SPtr> ServicePackage2Map::Close()
{
    vector<ServicePackage2SPtr> retval;

    {
        AcquireWriteLock writelock(lock_);
        if (!closed_)
        {
            for (auto const & entry : map_)
            {
                for (auto iter = entry.second->begin(); iter != entry.second->end(); ++iter)
                {
                    retval.push_back(move(iter->second));
                }
            }

            map_.clear();
			activationIdToContextMap_.clear();

            closed_ = true;
        }
    }

    return move(retval);
}

ErrorCode ServicePackage2Map::Add(ServicePackage2SPtr const & servicePackage)
{
    {
        AcquireWriteLock writeLock(lock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(servicePackage->ServicePackageName);
        if (iter == map_.end())
        {
            auto innerMap = make_shared<ServicePackageInstanceMap>();
            innerMap->insert(make_pair(servicePackage->Id.ActivationContext, servicePackage));
			
			auto key = this->GetActivationIdMapKey(servicePackage->Id.ServicePackageName, servicePackage->Id.PublicActivationId);
			activationIdToContextMap_.insert(make_pair(key, servicePackage->Id.ActivationContext));

            map_.insert(make_pair(servicePackage->ServicePackageName, innerMap));
        }
        else
        {
            auto innerMap = iter->second;

            auto innerIter = innerMap->find(servicePackage->Id.ActivationContext);
            if (innerIter != innerMap->end())
            {
                return ErrorCode(ErrorCodeValue::AlreadyExists);
            }

            innerMap->insert(make_pair(servicePackage->Id.ActivationContext, servicePackage));
			
			auto key = this->GetActivationIdMapKey(servicePackage->Id.ServicePackageName, servicePackage->Id.PublicActivationId);
			activationIdToContextMap_.insert(make_pair(key, servicePackage->Id.ActivationContext));
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ServicePackage2Map::Remove(
    wstring const & servicePackageName,
    ServicePackageActivationContext const & activationContext,
    int64 const instanceId,
    __out ServicePackage2SPtr & servicePackage)
{
    {
        AcquireWriteLock writelock(lock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(servicePackageName);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        auto innerMap = iter->second;

        auto innerIter = innerMap->find(activationContext);
        if (innerIter == innerMap->end() || innerIter->second->InstanceId != instanceId)
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        servicePackage = innerIter->second;
        innerMap->erase(innerIter);

		auto key = this->GetActivationIdMapKey(servicePackage->Id.ServicePackageName, servicePackage->Id.PublicActivationId);
		activationIdToContextMap_.erase(key);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ServicePackage2Map::Find(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    __out ServicePackage2SPtr & servicePackage) const
{
	{
		AcquireReadLock readlock(lock_);

		if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

		return this->Find_CallerHoldsLock(
			servicePackageInstanceId.ServicePackageName,
			servicePackageInstanceId.ActivationContext,
			servicePackage);
	}
}

ErrorCode ServicePackage2Map::GetInstancesOfServicePackage(
    wstring const & servicePackageName,
    __out vector<ServicePackage2SPtr> & servicePackageInstances) const
{
    {
        AcquireReadLock readlock(lock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(servicePackageName);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::Success);
        }

        auto innerMap = iter->second;

        for (auto innerIter = innerMap->begin(); innerIter != innerMap->end(); innerIter++)
        {
            servicePackageInstances.push_back(innerIter->second);
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ServicePackage2Map::GetAllServicePackageInstances(__out vector<ServicePackage2SPtr> & servicePackageInstances) const
{
    {
        AcquireReadLock readlock(lock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        for (auto iter = map_.begin(); iter != map_.end(); iter++)
        {
            auto innerMap = iter->second;

            for (auto innerIter = innerMap->begin(); innerIter != innerMap->end(); innerIter++)
            {
                servicePackageInstances.push_back(innerIter->second);
            }
        }        
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ServicePackage2Map::Find(
    wstring const & servicePackageName,
    ServicePackageVersion const & version,
    __out ServicePackage2SPtr & servicePackageInstance) const
{
    {
        AcquireReadLock readlock(lock_);
        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(servicePackageName);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::Success);
        }

        auto innerMap = iter->second;

        for (auto innerIter = innerMap->begin(); innerIter != innerMap->end(); innerIter++)
        {
            if (innerIter->second->CurrentVersionInstance.Version == version)
            {
                servicePackageInstance = innerIter->second;
                break;
            }
        }
    }

    return ErrorCode(ErrorCodeValue::NotFound);
}

ErrorCode ServicePackage2Map::Find(
	wstring const & servicePackageName,
	wstring const & publicActivationId,
	__out ServicePackage2SPtr & servicePackage) const
{
	{
		AcquireReadLock readlock(lock_);

		if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

		auto key = this->GetActivationIdMapKey(servicePackageName, publicActivationId);

		auto iter = activationIdToContextMap_.find(key);
		if (iter == activationIdToContextMap_.end())
		{
			return ErrorCode(ErrorCodeValue::NotFound);
		}

		return this->Find_CallerHoldsLock(servicePackageName, iter->second, servicePackage);
	}
}

ErrorCode ServicePackage2Map::Find(
	wstring const & servicePackageName,
	ServicePackageActivationContext const & activationContext,
	__out ServicePackage2SPtr & servicePackage) const
{
	{
		AcquireReadLock readlock(lock_);

		if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

		return this->Find_CallerHoldsLock(servicePackageName, activationContext, servicePackage);
	}
}

ErrorCode ServicePackage2Map::Find_CallerHoldsLock(
    wstring const & servicePackageName,
    ServicePackageActivationContext const & activationContext,
    __out ServicePackage2SPtr & servicePackage) const
{
	auto iter = map_.find(servicePackageName);
	if (iter == map_.end())
	{
		return ErrorCode(ErrorCodeValue::NotFound);
	}

	auto innerMap = iter->second;

	auto innerIter = innerMap->find(activationContext);
	if (innerIter == innerMap->end())
	{
		return ErrorCode(ErrorCodeValue::NotFound);
	}

	servicePackage = innerIter->second;

    return ErrorCode(ErrorCodeValue::Success);
}

wstring ServicePackage2Map::GetActivationIdMapKey(
	wstring const & servicePackageName, 	
	wstring const & publicActivationId) const
{
	return wformatString("{0}_{1}", servicePackageName, publicActivationId);
}
