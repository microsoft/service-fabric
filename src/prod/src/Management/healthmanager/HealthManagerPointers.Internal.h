// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        typedef std::map<std::wstring, std::wstring> AttributesMap;

        class RequestContext;
        typedef std::unique_ptr<RequestContext> RequestContextUPtr;

        class QueryRequestContext;
        typedef std::unique_ptr<QueryRequestContext> QueryRequestContextUPtr;
       
        class ClusterEntity;
        typedef std::shared_ptr<ClusterEntity> ClusterEntitySPtr;
        typedef std::weak_ptr<ClusterEntity> ClusterEntityWPtr;

        class NodeEntity;
        typedef std::shared_ptr<NodeEntity> NodeEntitySPtr;
        typedef std::weak_ptr<NodeEntity> NodeEntityWPtr;

        class ReplicaEntity;
        typedef std::shared_ptr<ReplicaEntity> ReplicaEntitySPtr;
        typedef std::weak_ptr<ReplicaEntity> ReplicaEntityWPtr;

        class PartitionEntity;
        typedef std::shared_ptr<PartitionEntity> PartitionEntitySPtr;
        typedef std::weak_ptr<PartitionEntity> PartitionEntityWPtr;
        
        class ServiceEntity;
        typedef std::shared_ptr<ServiceEntity> ServiceEntitySPtr;
        typedef std::weak_ptr<ServiceEntity> ServiceEntityWPtr;

        class ApplicationEntity;
        typedef std::shared_ptr<ApplicationEntity> ApplicationEntitySPtr;
        typedef std::weak_ptr<ApplicationEntity> ApplicationEntityWPtr;

        class DeployedApplicationEntity;
        typedef std::shared_ptr<DeployedApplicationEntity> DeployedApplicationEntitySPtr;
        typedef std::weak_ptr<DeployedApplicationEntity> DeployedApplicationEntityWPtr;

        class DeployedServicePackageEntity;
        typedef std::shared_ptr<DeployedServicePackageEntity> DeployedServicePackageEntitySPtr;
        typedef std::weak_ptr<DeployedServicePackageEntity> DeployedServicePackageEntityWPtr;

        class ProcessRequestAsyncOperation;
        typedef std::shared_ptr<ProcessRequestAsyncOperation> ProcessRequestAsyncOperationSPtr;

        class AttributesStoreData;
        typedef std::shared_ptr<AttributesStoreData> AttributesStoreDataSPtr;
        typedef std::unique_ptr<AttributesStoreData> AttributesStoreDataUPtr;

        class HealthEntity;
        typedef std::shared_ptr<HealthEntity> HealthEntitySPtr;
        typedef std::weak_ptr<HealthEntity> HealthEntityWPtr;

        class HealthEventStoreData;
        typedef std::unique_ptr<HealthEventStoreData> HealthEventStoreDataUPtr;
        
        class EntityCacheManager;
        typedef std::unique_ptr<EntityCacheManager> EntityCacheManagerUPtr;

        class ReplicatedStoreWrapper;
        typedef std::unique_ptr<ReplicatedStoreWrapper> ReplicatedStoreWrapperUPtr;

        class EntityJobQueueManager;
        typedef std::unique_ptr<EntityJobQueueManager> EntityJobQueueManagerUPtr;

        class IHealthJobItem;
        using IHealthJobItemSPtr = std::shared_ptr<IHealthJobItem>;

        typedef std::function<Common::ErrorCode(
            Store::ReplicaActivityId const & replicaActivityId,
            __out Store::IStoreBase::TransactionSPtr & tx)> PrepareTransactionCallback;
        typedef std::function<void(__in IHealthJobItem & jobItem, Common::AsyncOperationSPtr const & operation)> TransactionCompletedCallback;

        typedef std::function<void(HealthEntitySPtr const & entity)> EntityWorkCallback;
    }
}
