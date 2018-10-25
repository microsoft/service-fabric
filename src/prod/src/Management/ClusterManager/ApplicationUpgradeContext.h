// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        //
        // *** Serialization helper class
        //
        class TargetServicePackage : public Serialization::FabricSerializable
        {
        public:

            TargetServicePackage()
                : name_()
                , version_()
            {
            }

            TargetServicePackage(
                ServiceModelPackageName const & name, 
                ServiceModelVersion const & version)
                : name_(name)
                , version_(version)
            {
            }

            __declspec(property(get=get_PackageName)) ServiceModelPackageName const & PackageName;
            ServiceModelPackageName const & get_PackageName() const { return name_; }

            __declspec(property(get=get_PackageVersion)) ServiceModelVersion const & PackageVersion;
            ServiceModelVersion const & get_PackageVersion() const { return version_; }

            FABRIC_FIELDS_02(name_, version_);

        private:
            ServiceModelPackageName name_;
            ServiceModelVersion version_;
        };

        //
        // *** ApplicationUpgradeContext
        //
        class ApplicationUpgradeContext : public RolloutContext
        {
            DENY_COPY(ApplicationUpgradeContext)

        public:
            static const RolloutContextType::Enum ContextType;

            ApplicationUpgradeContext();

            ApplicationUpgradeContext(
                ApplicationUpgradeContext &&) = default;

            ApplicationUpgradeContext & operator=(
                ApplicationUpgradeContext &&) = default;

            explicit ApplicationUpgradeContext(
                Common::NamingUri const &);

            ApplicationUpgradeContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                ApplicationUpgradeDescription const &,
                ServiceModelVersion const &,
                std::wstring const & targetApplicationManifestId,
                uint64 upgradeInstance);

            ApplicationUpgradeContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                ApplicationUpgradeDescription const &,
                ServiceModelVersion const &,
                uint64 upgradeInstance,
                ApplicationDefinitionKind::Enum);

            ApplicationUpgradeContext(
                Store::ReplicaActivityId const &,
                ApplicationUpgradeDescription const &,
                ServiceModelVersion const &,
                std::wstring const & targetApplicationManifestId,
                uint64 upgradeInstance,
                ApplicationDefinitionKind::Enum);

            virtual ~ApplicationUpgradeContext();

            __declspec(property(get=get_UpgradeDescription)) ApplicationUpgradeDescription const & UpgradeDescription;
            ApplicationUpgradeDescription const & get_UpgradeDescription() const { return upgradeDescription_; }

            ApplicationUpgradeDescription && TakeUpgradeDescription() { return std::move(upgradeDescription_); }

            __declspec(property(get=get_UpgradeState, put=set_UpgradeState)) ApplicationUpgradeState::Enum const & UpgradeState;
            ApplicationUpgradeState::Enum const & get_UpgradeState() const { return upgradeState_; }
            void set_UpgradeState(ApplicationUpgradeState::Enum value) { upgradeState_ = value; }

            __declspec(property(get=get_RollbackApplicationTypeVersion)) ServiceModelVersion const & RollbackApplicationTypeVersion;
            ServiceModelVersion const & get_RollbackApplicationTypeVersion() const { return rollbackApplicationTypeVersion_; }

            __declspec(property(get = get_TargetApplicationManifestId, put=set_TargetApplicationManifestId)) std::wstring const & TargetApplicationManifestId;
            std::wstring const & get_TargetApplicationManifestId() const { return targetApplicationManifestId_; }
            void set_TargetApplicationManifestId(std::wstring const & value) { targetApplicationManifestId_ = value; }

            __declspec(property(get=get_InProgressUpgradeDomain)) std::wstring const & InProgressUpgradeDomain;
            std::wstring const & get_InProgressUpgradeDomain() const { return inProgressUpgradeDomain_; }

            __declspec(property(get=get_CompletedUpgradeDomains)) std::vector<std::wstring> const & CompletedUpgradeDomains;
            std::vector<std::wstring> const & get_CompletedUpgradeDomains() const { return completedUpgradeDomains_; }

            __declspec(property(get=get_PendingUpgradeDomains)) std::vector<std::wstring> const & PendingUpgradeDomains;
            std::vector<std::wstring> const & get_PendingUpgradeDomains() const { return pendingUpgradeDomains_; }

            __declspec(property(get=get_RollforwardInstance)) uint64 RollforwardInstance;
            uint64 get_RollforwardInstance() const;

            __declspec(property(get=get_RollbackInstance)) uint64 RollbackInstance;
            uint64 get_RollbackInstance() const;

            __declspec(property(get=get_UpgradeCompletionInstance)) uint64 UpgradeCompletionInstance;
            uint64 get_UpgradeCompletionInstance() const;

            static uint64 GetNextUpgradeInstance(uint64 applicationPackageInstance);
            static uint64 GetDeleteApplicationInstance(uint64 applicationPackageInstance);

            __declspec(property(get=get_IsUpgradeCompletedState)) bool IsUpgradeCompletedState;
            bool get_IsUpgradeCompletedState() const 
            { 
                return upgradeState_ == ApplicationUpgradeState::CompletedRollforward || 
                    upgradeState_ == ApplicationUpgradeState::CompletedRollback; 
            }

            __declspec(property(get = get_IsAutoBaseline)) bool IsAutoBaseline;
            bool get_IsAutoBaseline() const { return false; }

            __declspec(property(get=get_IsRollforwardState)) bool IsRollforwardState;
            bool get_IsRollforwardState() const { return upgradeState_ == ApplicationUpgradeState::RollingForward; }

            __declspec(property(get=get_IsRollbackState)) bool IsRollbackState;
            bool get_IsRollbackState() const { return upgradeState_ == ApplicationUpgradeState::RollingBack; }

            __declspec(property(get=get_IsInterrupted)) bool IsInterrupted;
            bool get_IsInterrupted() const { return commonUpgradeContextData_.IsRollbackAllowed && isInterrupted_; }

            __declspec(property(get=get_IsFMRequestModified)) bool IsFMRequestModified;
            bool get_IsFMRequestModified() const { return isFMRequestModified_; }
            void ClearIsFMRequestModified() { isFMRequestModified_ = false; }

            __declspec(property(get=get_IsHealthPolicyModified)) bool IsHealthPolicyModified;
            bool get_IsHealthPolicyModified() const { return isHealthPolicyModified_; }
            void SetIsHealthPolicyModified(bool value) { isHealthPolicyModified_ = value; }

            __declspec(property(get=get_IsPreparingUpgrade)) bool IsPreparingUpgrade;
            bool get_IsPreparingUpgrade() const { return isPreparingUpgrade_; }
            void ResetPreparingUpgrade() { isPreparingUpgrade_ = false; }

            __declspec(property(get=get_IsDefaultServiceUpdated)) bool IsDefaultServiceUpdated;
            bool get_IsDefaultServiceUpdated() const { return !defaultServiceUpdateDescriptions_.empty(); }

            __declspec(property(get=get_ApplicationDefinitionKind)) ApplicationDefinitionKind::Enum ApplicationDefinitionKind;
            ApplicationDefinitionKind::Enum get_ApplicationDefinitionKind() const { return applicationDefinitionKind_; }

            void AddBlockedServiceType(ServiceModelTypeName const & typeName);
            bool ContainsBlockedServiceType(ServiceModelTypeName const & typeName);
            void ClearBlockedServiceTypes();

            void AddTargetServicePackage(
                ServiceModelTypeName const &, 
                ServiceModelPackageName const &, 
                ServiceModelVersion const &);
            bool TryGetTargetServicePackage(
                ServiceModelTypeName const &, 
                __out ServiceModelPackageName &, 
                __out ServiceModelVersion &);
            void ClearTargetServicePackages();

            __declspec(property(get=get_AddedDefaultServices)) std::vector<ServiceContext> const & AddedDefaultServices;
            std::vector<ServiceContext> const & get_AddedDefaultServices() const { return addedDefaultServices_; }
            void PushAddedDefaultService(ServiceContext && context) { addedDefaultServices_.push_back(std::move(context)); }
            void ClearAddedDefaultServices() { addedDefaultServices_.clear(); }

            __declspec(property(get=get_DeletedDefaultServices)) std::vector<Common::NamingUri> const & DeletedDefaultServices;
            std::vector<Common::NamingUri> const & get_DeletedDefaultServices() const { return deletedDefaultServices_; }
            void PushDeletedDefaultServices(Common::NamingUri const && serviceName) { deletedDefaultServices_.push_back(move(serviceName)); }
            void ClearDeletedDefaultServices() { deletedDefaultServices_.clear(); }

            __declspec(property(get=get_DefaultServiceUpdateDescriptions)) std::map<Common::NamingUri, Naming::ServiceUpdateDescription> const & DefaultServiceUpdateDescriptions;
            std::map<Common::NamingUri, Naming::ServiceUpdateDescription> const & get_DefaultServiceUpdateDescriptions() const { return defaultServiceUpdateDescriptions_; }
            bool InsertDefaultServiceUpdateDescriptions(Common::NamingUri const & key, Naming::ServiceUpdateDescription && value) { return defaultServiceUpdateDescriptions_.insert(std::make_pair(key, move(value))).second; }
            void ClearDefaultServiceUpdateDescriptions() { defaultServiceUpdateDescriptions_.clear(); }

            __declspec(property(get=get_RollbackDefaultServiceUpdateDescriptions)) std::map<Common::NamingUri, Naming::ServiceUpdateDescription> const & RollbackDefaultServiceUpdateDescriptions;
            std::map<Common::NamingUri, Naming::ServiceUpdateDescription> const & get_RollbackDefaultServiceUpdateDescriptions() { return rollbackDefaultServiceUpdateDescriptions_; }
            bool InsertRollbackDefaultServiceUpdateDescriptions(Common::NamingUri const & key, Naming::ServiceUpdateDescription && value) { return rollbackDefaultServiceUpdateDescriptions_.insert(std::make_pair(key, move(value))).second; }
            void ClearRollbackDefaultServiceUpdateDescriptions() { rollbackDefaultServiceUpdateDescriptions_.clear(); }

            __declspec(property(get=get_LastHealthCheckResult)) bool LastHealthCheckResult;
            bool get_LastHealthCheckResult() const { return lastHealthCheckResult_; }

            __declspec(property(get=get_HealthCheckElapsedTime)) Common::TimeSpan const HealthCheckElapsedTime;
            Common::TimeSpan const get_HealthCheckElapsedTime() const { return healthCheckElapsedTime_; }

            __declspec(property(get=get_OverallUpgradeElapsedTime)) Common::TimeSpan const OverallUpgradeElapsedTime;
            Common::TimeSpan const get_OverallUpgradeElapsedTime() const { return overallUpgradeElapsedTime_; }

            __declspec(property(get=get_UpgradeDomainElapsedTime)) Common::TimeSpan const UpgradeDomainElapsedTime;
            Common::TimeSpan const get_UpgradeDomainElapsedTime() const { return upgradeDomainElapsedTime_; }

            __declspec(property(get=get_HealthEvaluationReasons)) std::vector<ServiceModel::HealthEvaluation> const & UnhealthyEvaluations;
            std::vector<ServiceModel::HealthEvaluation> const & get_HealthEvaluationReasons() const { return unhealthyEvaluations_; }
            std::vector<ServiceModel::HealthEvaluation> && TakeUnhealthyEvaluations() { return std::move(unhealthyEvaluations_); }

            __declspec(property(get=get_CurrentUpgradeDomainProgress)) Reliability::UpgradeDomainProgress const & CurrentUpgradeDomainProgress;
            Reliability::UpgradeDomainProgress const & get_CurrentUpgradeDomainProgress() const { return currentUpgradeDomainProgress_; }
            Reliability::UpgradeDomainProgress && TakeCurrentUpgradeDomainProgress() { return std::move(currentUpgradeDomainProgress_); }

            __declspec(property(get=get_CommonUpgradeContextData)) ClusterManager::CommonUpgradeContextData & CommonUpgradeContextData;
            ClusterManager::CommonUpgradeContextData & get_CommonUpgradeContextData() { return commonUpgradeContextData_; }
            ClusterManager::CommonUpgradeContextData && TakeCommonUpgradeContextData() { return std::move(commonUpgradeContextData_); }

            void OnFailRolloutContext() override { } // no-op

            void SetHealthCheckElapsedTimeToWaitDuration(Common::TimeSpan const elapsed);
            void SetLastHealthCheckResult(bool isHealthy);
            void UpdateHealthCheckElapsedTime(Common::TimeSpan const elapsed);
            void UpdateHealthMonitoringTimeouts(Common::TimeSpan const elapsed);
            void UpdateHealthEvaluationReasons(std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations);
            void ClearHealthEvaluationReasons();

            void ResetUpgradeDomainTimeouts();
            void SetRollingBack(bool goalStateExists);
            void RevertToManualUpgrade();
            
            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            void UpdateInProgressUpgradeDomain(std::wstring && upgradeDomain);
            void UpdateCompletedUpgradeDomains(std::vector<std::wstring> && upgradeDomains);
            void UpdatePendingUpgradeDomains(std::vector<std::wstring> && upgradeDomains);
            std::vector<std::wstring> TakeCompletedUpgradeDomains();

            void UpdateUpgradeProgress(Reliability::UpgradeDomainProgress && currentUpgradeDomainProgress);

            Common::ErrorCode CompleteRollforward(Store::StoreTransaction const &);
            Common::ErrorCode CompleteRollback(Store::StoreTransaction const &);

            bool TryInterrupt();
            bool TryModifyUpgrade(
                ApplicationUpgradeUpdateDescription const &,
                __out std::wstring & validationErrorMessage);
                
            FABRIC_FIELDS_27(
                upgradeDescription_, 
                upgradeState_, 
                rollbackApplicationTypeVersion_,
                inProgressUpgradeDomain_,
                completedUpgradeDomains_,
                pendingUpgradeDomains_,
                upgradeInstance_,
                isInterrupted_,
                isPreparingUpgrade_,
                blockedServiceTypes_,
                unusedServiceTypes_, // deprecated
                addedDefaultServices_,
                healthCheckElapsedTime_,
                overallUpgradeElapsedTime_,
                upgradeDomainElapsedTime_,
                lastHealthCheckResult_,
                isFMRequestModified_,
                isHealthPolicyModified_,
                unhealthyEvaluations_,
                currentUpgradeDomainProgress_,
                commonUpgradeContextData_,
                targetServicePackages_,
                deletedDefaultServices_,
                defaultServiceUpdateDescriptions_,
                rollbackDefaultServiceUpdateDescriptions_,
                targetApplicationManifestId_,
                applicationDefinitionKind_);

        protected:
            virtual StringLiteral const & GetTraceComponent() const;
            virtual std::wstring ConstructKey() const;

        private:
            Common::ErrorCode CompleteUpgrade(Store::StoreTransaction const &);

            ApplicationUpgradeDescription upgradeDescription_;
            ApplicationUpgradeState::Enum upgradeState_;
            ServiceModelVersion rollbackApplicationTypeVersion_;
            std::wstring targetApplicationManifestId_;

            // Neither added nor removed types are tracked because service creation will be blocked
            // in both cases during the upgrade.
            //
            std::map<ServiceModelTypeName, TargetServicePackage> targetServicePackages_;

            std::wstring inProgressUpgradeDomain_;
            std::vector<std::wstring> completedUpgradeDomains_;
            std::vector<std::wstring> pendingUpgradeDomains_;

            uint64 upgradeInstance_;    // Used by FM to detect stale upgrade requests
            bool isInterrupted_;        // flags that this upgrade has been interrupted by the client and should be rolled back
            bool isPreparingUpgrade_;   // flags that this upgrade is still loading the application changes and performing pre-upgrade verification

            std::vector<std::wstring> blockedServiceTypes_; // service types that are being removed/moved during upgrade
            std::vector<std::wstring> unusedServiceTypes_;  // (deprecated)service types without any active services during upgrade preparation
            std::vector<ServiceContext> addedDefaultServices_;   // default services to create during upgrade
            std::vector<Common::NamingUri> deletedDefaultServices_; // default services to delete during upgrade
            std::map<Common::NamingUri, Naming::ServiceUpdateDescription> defaultServiceUpdateDescriptions_;   // update description of default services to update during upgrade
            std::map<Common::NamingUri, Naming::ServiceUpdateDescription> rollbackDefaultServiceUpdateDescriptions_;  // active update description of default services to update during upgrade. Persist for rollback. 

            // persist elapsed times across failover
            Common::TimeSpan healthCheckElapsedTime_;
            Common::TimeSpan overallUpgradeElapsedTime_;
            Common::TimeSpan upgradeDomainElapsedTime_;

            bool lastHealthCheckResult_;
            bool isFMRequestModified_;
            bool isHealthPolicyModified_;

            std::vector<ServiceModel::HealthEvaluation> unhealthyEvaluations_;
            Reliability::UpgradeDomainProgress currentUpgradeDomainProgress_;

            ApplicationDefinitionKind::Enum applicationDefinitionKind_;

            // Many of the preceding members can technically be pulled
            // into this class, but must remain outside to preserve
            // backwards compatibility.
            //
            ClusterManager::CommonUpgradeContextData commonUpgradeContextData_;
        };
    }
}

DEFINE_USER_MAP_UTILITY(Management::ClusterManager::ServiceModelTypeName, Management::ClusterManager::TargetServicePackage);
DEFINE_USER_MAP_UTILITY(Common::NamingUri, Naming::ServiceUpdateDescription);
DEFINE_USER_ARRAY_UTILITY(Common::NamingUri);
