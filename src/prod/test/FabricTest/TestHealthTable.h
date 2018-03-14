// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class TestHealthTable;

    typedef std::map<std::wstring, FABRIC_HEALTH_STATE> EntityStateResults;

    class TestEntity
    {
        class TestEvent;
        typedef std::unique_ptr<TestEvent> TestEventUPtr;

    public:
        static const int MaxProperty;
        static const int MaxTTL;
        static const int MinTransientEventTTL;
        static const double ErrorRatio;
        static const double WarningRatio;

        std::wstring const & GetId() const { return id_; }

        int64 GetInstance() const { return instance_; }

        bool HasSystemError() const { return systemError_ > 0; }

        std::vector<TestEventUPtr>::iterator GetEvent(std::wstring const & source, std::wstring const & property);

        void Report(TestWatchDog & watchDog);

        FABRIC_HEALTH_STATE GetState(bool considerWarningAsError) const;

        void SetInvalidatedSequence();

        virtual std::wstring GetParentId() const { return L""; }

        virtual bool CanQuery() const { return true; }

        virtual bool IsParentHealthy() const { return true; }

        bool IsMissing() const;
        bool CanDelete() const;

        void ResetMissing();
        void SetMissing(std::wstring const & reason);

        bool IsReportable() { return !IsMissing() && CanReport(); }

        void UpdateExpiredEvents(std::vector<ServiceModel::HealthEvent> const & events);
        void UpdateCounters();
        void ResetEvents(std::wstring const & reason);

        virtual HRESULT BeginTestQuery(
            Common::ComPointer<IFabricHealthClient4> const & healthClient,
            Common::TimeSpan timeout,
            IFabricAsyncOperationCallback *callback,
            IFabricAsyncOperationContext **context,
            std::wstring & policyString) = 0;

        virtual HRESULT EndTestQuery(
            TestHealthTable const & table,
            Common::ComPointer<IFabricHealthClient4> const & healthClient,
            IFabricAsyncOperationContext *context,
            std::wstring const & policyString)  = 0;

        bool Verify(std::vector<ServiceModel::HealthEvent> const & events);

        virtual void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

    protected:
        TestEntity();

        FABRIC_SEQUENCE_NUMBER GetLatestSequence() const;
        bool VerifyQueryResult(std::wstring const & policyString, FABRIC_HEALTH_STATE state, EntityStateResults & children, EntityStateResults const & queryChildrenStates, std::vector<ServiceModel::HealthEvent> const & events, FABRIC_HEALTH_STATE aggregatedHealthState) const;

        void DumpEvents() const;
        void UpdateInstance(std::shared_ptr<Management::HealthManager::AttributesStoreData> const & attr, bool allowLowerInstance = false);

    protected:
        int64 instance_;
        std::vector<TestEventUPtr> events_;
        FABRIC_SEQUENCE_NUMBER lastInvalidatedSequence_;
        std::wstring id_;
        int expiredCount_;
        int errorCount_;
        int warningCount_;
        int systemError_;
        int systemWarning_;
        Common::DateTime missingTime_;

    private:
        bool VerifyChildrenResult(EntityStateResults const & queryResults, EntityStateResults & children) const;

        virtual FABRIC_HEALTH_REPORT CreateReport(ScopedHeap & heap, FABRIC_HEALTH_INFORMATION *commonHealthInformation) = 0;
        virtual bool CanReport() const { return true; }
    };

    using TestEntitySPtr = std::shared_ptr<TestEntity>;

    class TestHealthTable
    {
        DENY_COPY(TestHealthTable);

    public:
        TestHealthTable();

        void Report(TestWatchDog & watchDog);

        void Pause();
        void Resume();

        bool Update(Management::HealthManager::HealthManagerReplicaSPtr const & hm, __out std::vector<Management::HealthManager::NodeEntitySPtr> & entities);
        bool Update(Management::HealthManager::HealthManagerReplicaSPtr const & hm, __out std::vector<Management::HealthManager::PartitionEntitySPtr> & entities);
        bool Update(Management::HealthManager::HealthManagerReplicaSPtr const & hm, __out std::vector<Management::HealthManager::ReplicaEntitySPtr> & entities);
        bool Update(Management::HealthManager::HealthManagerReplicaSPtr const & hm, __out std::vector<Management::HealthManager::ServiceEntitySPtr> & entities);
        bool Update(Management::HealthManager::HealthManagerReplicaSPtr const & hm, __out std::vector<Management::HealthManager::ApplicationEntitySPtr> & entities);
        bool Update(Management::HealthManager::HealthManagerReplicaSPtr const & hm, __out std::vector<Management::HealthManager::DeployedServicePackageEntitySPtr> & entities);
        bool Update(Management::HealthManager::HealthManagerReplicaSPtr const & hm, __out std::vector<Management::HealthManager::DeployedApplicationEntitySPtr> & entities);

        bool Verify(Management::HealthManager::NodeEntitySPtr const & entity, std::vector<ServiceModel::HealthEvent> const & events, bool isInvalidated);
        bool Verify(Management::HealthManager::PartitionEntitySPtr const & entity, std::vector<ServiceModel::HealthEvent> const & events, bool isInvalidated);
        bool Verify(Management::HealthManager::ReplicaEntitySPtr const & entity, std::vector<ServiceModel::HealthEvent> const & events);
        bool Verify(Management::HealthManager::ServiceEntitySPtr const & entity, std::vector<ServiceModel::HealthEvent> const & events, bool isInvalidated);
        bool Verify(Management::HealthManager::ApplicationEntitySPtr const & entity, std::vector<ServiceModel::HealthEvent> const & events);
        bool Verify(Management::HealthManager::DeployedServicePackageEntitySPtr const & entity, std::vector<ServiceModel::HealthEvent> const & events);
        bool Verify(Management::HealthManager::DeployedApplicationEntitySPtr const & entity, std::vector<ServiceModel::HealthEvent> const & events);

        FABRIC_HEALTH_STATE GetReplicasState(std::wstring const & parentId, BYTE maxPercentUnhealthy, ServiceModel::ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const;
        FABRIC_HEALTH_STATE GetPartitionsState(std::wstring const & parentId, BYTE maxPercentUnhealthy, ServiceModel::ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const;
        FABRIC_HEALTH_STATE GetDeployedServicePackagesState(std::wstring const & parentId, BYTE maxPercentUnhealthy, ServiceModel::ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const;
        FABRIC_HEALTH_STATE GetServicesState(std::wstring const & parentId, ServiceModel::ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const;
        FABRIC_HEALTH_STATE GetDeployedApplicationsState(std::wstring const & parentId, BYTE maxPercentUnhealthy, ServiceModel::ApplicationHealthPolicy const & policy, __out EntityStateResults & results, std::wstring const & upgradeDomain = L"") const;
        FABRIC_HEALTH_STATE GetNodesState(BYTE maxPercentUnhealthy, ServiceModel::ClusterHealthPolicy const & policy, __out EntityStateResults & results, std::wstring const & upgradeDomain = L"") const;
        FABRIC_HEALTH_STATE GetApplicationsState(BYTE maxPercentUnhealthy, ServiceModel::ApplicationTypeHealthPolicyMap const & appTypePolicyMap, ServiceModel::ApplicationHealthPolicyMapSPtr const & appHealthPolicyMap, __out EntityStateResults & results) const;

        bool RunQuery(Common::ComPointer<IFabricHealthClient4> const & healthClient, std::set<std::wstring> & verifiedIds);
        bool TestQuery(Common::ComPointer<IFabricHealthClient4> const & healthClient);

        bool TestClusterHealth(ServiceModel::ClusterHealth const & health, ServiceModel::ClusterHealthPolicy const & policy) const;

    private:
        std::wstring GetUpgradeDomain(std::wstring const & nodeId) const;
        bool TestIsClusterHealthy(std::vector<std::wstring> const & upgradeDomains) const;

        template<class TE>
        void Update(Management::HealthManager::HealthManagerReplicaSPtr const & hm, TE & entityTable);

        void Update();

    private:
        mutable Common::RwLock lock_;
        bool active_;
    };

    typedef std::shared_ptr<TestHealthTable> TestHealthTableSPtr;
}
