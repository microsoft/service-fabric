// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ServiceContext : public DeletableRolloutContext
        {
            DENY_COPY_ASSIGNMENT(ServiceContext)

        public:
            ServiceContext();

            ServiceContext(
                ServiceModelApplicationId const & appId,
                Common::NamingUri const & appName,
                Common::NamingUri const & absoluteServiceName);

            ServiceContext(
                ServiceModelApplicationId const & appId,
                Common::NamingUri const & appName,
                Common::NamingUri const & absoluteServiceName,
                bool const isForceDelete);

            ServiceContext(
                ServiceContext const &);

            ServiceContext(
                ServiceContext &&);

            ServiceContext & operator=(
                ServiceContext &&);

            // for default services
            ServiceContext(
                Store::ReplicaActivityId const &,
                ServiceModelApplicationId const &,
                ServiceModelPackageName const &,
                ServiceModelVersion const &,
                uint64 packageInstance,
                Naming::PartitionedServiceDescriptor const &);

            // for non-default services
            ServiceContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                ServiceModelApplicationId const &,
                ServiceModelPackageName const &,
                ServiceModelVersion const &,
                uint64 packageInstance,
                Naming::PartitionedServiceDescriptor const &);

            virtual ~ServiceContext();

            __declspec(property(get=get_ApplicationId)) ServiceModelApplicationId const & ApplicationId;
            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            __declspec(property(get=get_AbsoluteServiceName)) Common::NamingUri const & AbsoluteServiceName;
            __declspec(property(get=get_ServiceName)) ServiceModelServiceName const & ServiceName;
            __declspec(property(get=get_ServiceTypeName)) ServiceModelTypeName const & ServiceTypeName;
            __declspec(property(get=get_PackageName)) ServiceModelPackageName const & PackageName;
            __declspec(property(get=get_PackageVersion)) ServiceModelVersion const & PackageVersion;
            __declspec(property(get=get_PackageInstance)) uint64 PackageInstance;
            __declspec(property(get=get_ServiceDescriptor)) Naming::PartitionedServiceDescriptor const & ServiceDescriptor;
            __declspec(property(get=get_IsRequired)) bool IsDefaultService;

            ServiceModelApplicationId const & get_ApplicationId() const { return applicationId_; }
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            Common::NamingUri const & get_AbsoluteServiceName() const { return absoluteServiceName_; }
            ServiceModelServiceName const & get_ServiceName() const { return serviceName_; }
            ServiceModelTypeName const & get_ServiceTypeName() const { return typeName_; }
            ServiceModelPackageName const & get_PackageName() const { return packageName_; }
            ServiceModelVersion const & get_PackageVersion() const { return packageVersion_; }
            uint64 const & get_PackageInstance() const { return packageInstance_; }
            Naming::PartitionedServiceDescriptor const & get_ServiceDescriptor() const { return partitionedServiceDescriptor_; }
            bool get_IsRequired() const { return isDefaultService_; }

            static Common::ErrorCode ReadApplicationServices(
                Store::StoreTransaction const &, 
                ServiceModelApplicationId const &, 
                __out std::vector<ServiceContext> &);

            static ServiceModelServiceName GetRelativeServiceName(std::wstring const & appName, std::wstring const & absoluteServiceName);
            // Performance optimization:
            // If the service was successfully created or deleted at the Naming Service,
            // then do not send another Naming request if processing of this context is
            // retried (e.g. due to write conflicts). This reduces load on the Naming Service
            // in high contention scenarios (many parallel create/delete service requests on a single
            // application).
            //
            __declspec(property(get=get_IsCommitPending)) bool IsCommitPending;
            bool get_IsCommitPending() const { return isCommitPending_; }

            void SetCommitPending() { isCommitPending_ = true; }
            void ResetCommitPending() { isCommitPending_ = false; }

            // reset the commit pending state if creation fails and we attempt to revert the creation
            //
            void OnFailRolloutContext() override { this->ResetCommitPending(); }

            ServiceModel::ServiceQueryResult ToQueryResult(std::wstring const &) const;
            FABRIC_QUERY_SERVICE_STATUS GetQueryStatus() const;

            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_11(
                applicationId_, 
                applicationName_, 
                absoluteServiceName_, 
                serviceName_, 
                typeName_, 
                packageName_,
                packageVersion_,
                packageInstance_,
                isDefaultService_,
                partitionedServiceDescriptor_,
                isForceDelete_);

        protected:
            virtual std::wstring ConstructKey() const;
            
        private:
            static Common::NamingUri StringToNamingUri(std::wstring const & appName);

            ServiceModelApplicationId applicationId_;
            Common::NamingUri applicationName_;
            Common::NamingUri absoluteServiceName_;
            ServiceModelServiceName serviceName_;
            ServiceModelTypeName typeName_;
            ServiceModelPackageName packageName_;
            ServiceModelVersion packageVersion_;
            uint64 packageInstance_;
            Naming::PartitionedServiceDescriptor partitionedServiceDescriptor_;
            bool isDefaultService_;
            bool isCommitPending_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::ClusterManager::ServiceContext);
