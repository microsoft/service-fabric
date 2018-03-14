// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ApplicationContext : public DeletableRolloutContext
        {
            DENY_COPY(ApplicationContext)

        public:
            ApplicationContext();

            explicit ApplicationContext(
                Common::NamingUri const &);

            explicit ApplicationContext(
                Common::NamingUri const &,
                bool const);

            ApplicationContext(
                ApplicationContext &&);

            ApplicationContext & operator=(
                ApplicationContext &&);

            ApplicationContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                uint64 globalInstanceCount,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::wstring const &,
                std::map<std::wstring, std::wstring> const &);

            ApplicationContext(
                Store::ReplicaActivityId const &,
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                uint64 globalInstanceCount,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::wstring const &,
                std::map<std::wstring, std::wstring> const &,
                ApplicationDefinitionKind::Enum const);

            virtual ~ApplicationContext();

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            __declspec(property(get=get_ApplicationId)) ServiceModelApplicationId const & ApplicationId;
            __declspec(property(get=get_GlobalInstanceCount)) uint64 const & GlobalInstanceCount;
            __declspec(property(get=get_ApplicationVersion)) uint64 const & ApplicationVersion;
            __declspec(property(get=get_PackageInstance)) uint64 const & PackageInstance;
            __declspec(property(get=get_TypeName)) ServiceModelTypeName const & TypeName;
            __declspec(property(get=get_TypeVersion)) ServiceModelVersion const & TypeVersion;
            __declspec(property(get=get_ManifestId, put=put_ManifestId)) std::wstring const & ManifestId;
            __declspec(property(get=get_Params)) std::map<std::wstring, std::wstring> const & Parameters;
            __declspec(property(get = get_ApplicationCapacity, put = put_ApplicationCapacity)) Reliability::ApplicationCapacityDescription const & ApplicationCapacity;
            __declspec(property(get=get_IsUpgrading)) bool IsUpgrading;
            __declspec(property(get=get_HasReportedHealthEntity)) bool HasReportedHealthEntity;
            __declspec(property(get=get_ApplicationDefinitionKind)) ApplicationDefinitionKind::Enum ApplicationDefinitionKind;
            
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            ServiceModelApplicationId const & get_ApplicationId() const { return applicationId_; }
            uint64 const & get_GlobalInstanceCount() const { return globalInstanceCount_; }
            uint64 const & get_ApplicationVersion() const { return applicationVersion_; }
            uint64 const & get_PackageInstance() const { return packageInstance_; }
            ServiceModelTypeName const & get_TypeName() const { return appTypeName_; }
            ServiceModelVersion const & get_TypeVersion() const { return appTypeVersion_; }
            std::wstring const & get_ManifestId() const { return appManifestId_; }
            void put_ManifestId(std::wstring const & value) { appManifestId_ = value; }
            std::map<std::wstring, std::wstring> const & get_Params() const { return appParameters_; }
            Reliability::ApplicationCapacityDescription const & get_ApplicationCapacity() const { return capacityDescription_; }
            void put_ApplicationCapacity(Reliability::ApplicationCapacityDescription const& appCapacity) { capacityDescription_ = appCapacity; }
            bool get_IsUpgrading() const { return isUpgrading_; }
            bool get_HasReportedHealthEntity() const { return hasReportedHealthEntity_; }
            ApplicationDefinitionKind::Enum get_ApplicationDefinitionKind() const { return applicationDefinitionKind_; }

            bool CanAcceptUpgrade(ApplicationUpgradeDescription const &, StoreDataApplicationManifest const &) const;

            void SetApplicationVersion(uint64);
            void SetApplicationAndTypeVersion(uint64 applicationVersion, ServiceModelVersion const & applicationTypeVersion);
            void SetApplicationParameters(std::map<std::wstring, std::wstring> const & applicationParameters);
            Common::ErrorCode SetApplicationCapacity(Reliability::ApplicationCapacityDescription const & applicationCapacity);
            void SetHasReportedHealthEntity();

            static std::wstring GetKeyPrefix(ServiceModelApplicationId const &);

            Common::ErrorCode SetUpgradePending(Store::StoreTransaction const &, uint64 packageInstance = 0);
            Common::ErrorCode SetUpgradeComplete(Store::StoreTransaction const &, uint64 packageInstance);
 
            // Optimize communication to Naming Service on retries if all previous
            // CM->Naming communications succeeded already
            //
            __declspec(property(get=get_IsCommitPending)) bool IsCommitPending;
            bool get_IsCommitPending() const { return isCommitPending_; }

            void SetCommitPending() { isCommitPending_ = true; }
            void ResetCommitPending() { isCommitPending_ = false; }
            void OnFailRolloutContext() override { this->ResetCommitPending(); }

            __declspec(property(get=get_PendingDefaultServices)) std::vector<ServiceModelServiceNameEx> const & PendingDefaultServices;
            std::vector<ServiceModelServiceNameEx> const & get_PendingDefaultServices() const { return pendingDefaultServices_; }

            void AddPendingDefaultService(ServiceModelServiceNameEx &&);
            void ClearPendingDefaultServices();

            __declspec(property(get = get_UpdateID, put = put_UpdateID)) uint64 UpdateID;
            uint64 get_UpdateID() const { return updateID_; }
            void put_UpdateID(uint64 id) { updateID_ = id; }

            ServiceModel::ApplicationQueryResult ToQueryResult(bool excludeApplicationParameters = false) const;

            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_16(
                applicationName_, 
                applicationId_, 
                globalInstanceCount_, 
                applicationVersion_,
                packageInstance_,
                appTypeName_,
                appTypeVersion_,
                appParameters_,
                isUpgrading_,
                pendingDefaultServices_,
                hasReportedHealthEntity_,
                updateID_,
                capacityDescription_,
                isForceDelete_,
                applicationDefinitionKind_,
                appManifestId_);
        
        protected:
            virtual std::wstring ConstructKey() const;

        private:
            Common::NamingUri applicationName_;
            ServiceModelApplicationId applicationId_;

            // Increments globally for each application instance created of any application type.
            // Used to generate application Id.
            //
            uint64 globalInstanceCount_;

            // Generated by ImageBuilder for each upgrade of an application.
            // Appears in the ApplicationInstance.<version>.xml filename, which points to the
            // relevant application/service packages.
            // Used to identify the current application version to the ImageBuilder UpgradeApplication() API.
            //
            uint64 applicationVersion_;

            // Generated by CM for each upgrade of an application.
            // Used for stale placement request detection by the Reliability subsystem.
            // The RolloutVersion for a package that was not upgraded will not change, but 
            // the package instance will change.
            //
            uint64 packageInstance_;

            ServiceModelTypeName appTypeName_;
            ServiceModelVersion appTypeVersion_;
            std::wstring appManifestId_;
            std::map<std::wstring, std::wstring> appParameters_;
            uint64 updateID_;
            Reliability::ApplicationCapacityDescription capacityDescription_;
            bool isUpgrading_;
            bool isCommitPending_;
            ApplicationDefinitionKind::Enum applicationDefinitionKind_;

            Common::ExclusiveLock pendingDefaultServicesLock_;
            std::vector<ServiceModelServiceNameEx> pendingDefaultServices_;

            // Used to detect V1 applications without a reported health entity
            bool hasReportedHealthEntity_;
        };
    }
}
