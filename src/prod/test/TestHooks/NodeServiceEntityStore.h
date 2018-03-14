// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestHooks
{
    /*
        Provides a map of items
        Each item has a name
        Items of the same name can be stored per node, per service
        Thus, it is like a multi-level map
    */
    template<typename T>
    class NodeServiceEntityStore
    {
    public:
        typedef std::shared_ptr<T> EntitySPtrType;
        typedef std::map<std::wstring, EntitySPtrType> EntityMap;
        typedef std::map<std::wstring, EntityMap> ServiceEntityMap;
        typedef std::map<std::wstring, ServiceEntityMap> NodeServiceEntityMap;

        void AddEntity(
            std::wstring const & nodeId,
            std::wstring const & serviceName,
            std::wstring const & entityName,
            EntitySPtrType const & entity)
        {            
            Common::AcquireWriteLock grab(lock_);
            map_[nodeId][serviceName][entityName] = entity;
        }

        void RemoveEntity(
            std::wstring const & nodeId,
            std::wstring const & serviceName,
            std::wstring const & entityName)
        {
            Common::AcquireWriteLock grab(lock_);
            if (!HasEntity(nodeId, serviceName, entityName))
            {
                return;
            }
        }

        bool HasEntity(
            std::wstring const & nodeId,
            std::wstring const & serviceName,
            std::wstring const & entityName) const
        {            
            Common::AcquireExclusiveLock grab(lock_);
            return HasEntity(grab, nodeId, serviceName, entityName);
        }

        EntitySPtrType GetEntity(std::wstring const & nodeId,
            std::wstring const & serviceName,
            std::wstring const & entityName)
        {
            Common::AcquireExclusiveLock grab(lock_);
            if (!HasEntity(grab, nodeId, serviceName, entityName))
            {
                return EntitySPtrType();
            }

            return map_[nodeId][serviceName][entityName];
        }

        std::vector<EntitySPtrType> GetAll()
        {
            Common::AcquireExclusiveLock grab(lock_);

            std::vector<EntitySPtrType> rv;

            for (auto const & i : map_)
            {
                for (auto const & j : i.second)
                {
                    for (auto const & k : j.second)
                    {
                        rv.push_back(k.second);
                    }
                }
            }

            return rv;
        }
    
    private:
        bool HasEntity(
            Common::AcquireExclusiveLock &,
            std::wstring const & nodeId,
            std::wstring const & serviceName,
            std::wstring const & entityName) const
        {
            auto nodeIter = map_.find(nodeId);
            if (nodeIter == map_.end())
            {
                return false;
            }

            auto const & serviceMap = nodeIter->second;
            auto serviceIter = serviceMap.find(serviceName);
            if (serviceIter == serviceMap.end())
            {
                return false;
            }

            auto const & entityMap = serviceIter->second;
            auto entityIter = entityMap.find(entityName);
            if (entityIter == entityMap.end())
            {
                return false;
            }

            return true;
        }

        Common::ExclusiveLock lock_;
        NodeServiceEntityMap map_;
    };
}
