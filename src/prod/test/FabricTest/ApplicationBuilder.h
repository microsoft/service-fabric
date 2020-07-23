// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ApplicationBuilder : public Common::ComponentRoot
    {
    public:
        ApplicationBuilder(
            std::wstring const& applicationTypeName, 
            std::wstring const& applicationVersion,
            std::wstring const& clientCredentialsType);

        static bool IsApplicationBuilderCommand(std::wstring const& command);
        static bool ExecuteCommand(
            std::wstring const& command, 
            Common::StringCollection const& params, 
            std::wstring const& clientCredentialsType,
            Common::ComPointer<IFabricNativeImageStoreClient> const & = Common::ComPointer<IFabricNativeImageStoreClient>());
        static void GetDefaultServices(std::wstring const& appType, std::wstring const& version, Common::StringCollection & defaultServices);
        static std::wstring GetApplicationBuilderName(std::wstring const& appType, std::wstring const& version);
        static bool TryGetDefaultServices(
            std::wstring const& appBuilderName,
            Common::StringCollection & defaultServices);
        static bool TryGetServiceManifestDescription(
            std::wstring const& appBuilderName, 
            std::wstring const& servicePackageName, 
            ServiceModel::ServiceManifestDescription & serviceManifest);

        static bool TryGetSupportedServiceTypes(
            std::wstring const& appBuilderName,
            ServiceModel::CodePackageIdentifier const& codePackageId, 
            std::vector<std::wstring> & serviceTypes);

        // ApplicationBuilder commands
        static std::wstring const ApplicationBuilderCreateApplicationType;
        static std::wstring const ApplicationBuilderSetCodePackage;
        static std::wstring const ApplicationBuilderSetServicePackage;
        static std::wstring const ApplicationBuilderSetServiceTypes;
        static std::wstring const ApplicationBuilderSetServiceTemplates;
        static std::wstring const ApplicationBuilderSetDefaultService;
        static std::wstring const ApplicationBuilderSetUser;
        static std::wstring const ApplicationBuilderSetGroup;
        static std::wstring const ApplicationBuilderRunAs;
        static std::wstring const ApplicationBuilderSharedPackage;
        static std::wstring const ApplicationBuilderHealthPolicy;
        static std::wstring const ApplicationBuilderNetwork;
        static std::wstring const ApplicationBuilderEndpoint;
        static std::wstring const ApplicationBuilderUpload;
        static std::wstring const ApplicationBuilderDelete;
        static std::wstring const ApplicationBuilderClear;
        static std::wstring const ApplicationBuilderSetParameters;

		static std::wstring const ApplicationPackageFolderInImageStore;
   
        enum PartitionScheme { Singleton, UniformInt64, Named };

    private:
        class ServiceBase
        {
        public:
            ServiceBase()
                : TypeName()
                , Scheme()
                , ScaleOutCount()
                , PartitionCount()
                , LowKey()
                , HighKey()
                , PartitionNames()
                , PlacementConstraints()
                , Metrics()
                , PlacementPolicies()
                , ServiceCorrelations()
                , DefaultMoveCost()
            {
            }

            std::wstring TypeName;
            PartitionScheme Scheme;
            std::wstring ScaleOutCount;
            std::wstring PartitionCount;
            std::wstring LowKey;
            std::wstring HighKey;
            std::vector<std::wstring> PartitionNames;
            std::wstring ServiceDnsName;

            std::wstring PlacementConstraints;
            std::vector<Reliability::ServiceLoadMetricDescription> Metrics;
            std::vector<ServiceModel::ServicePlacementPolicyDescription> PlacementPolicies;
            std::vector<Reliability::ServiceCorrelationDescription> ServiceCorrelations;
            std::vector<Reliability::ServiceScalingPolicyDescription> ScalingPolicies;
            std::wstring DefaultMoveCost;
        };
        
        // StatelessServiceType in xsd
        class StatelessService : public ServiceBase
        {
        public:
            StatelessService()
                : InstanceCount()
            {
            }

            std::wstring InstanceCount;

            std::wstring ServicePackageActivationMode;
        };

        // StatefulServiceType in xsd
        class StatefulService : public ServiceBase
        {
        public:
            StatefulService()
                : TargetReplicaSetSize()
                , MinReplicaSetSize()
                , ReplicaRestartWaitDurationInSeconds()
                , QuorumLossWaitDurationInSeconds()
                , StandByReplicaKeepDurationInSeconds()
            {
            }

            std::wstring TargetReplicaSetSize;
            std::wstring MinReplicaSetSize;
            std::wstring ReplicaRestartWaitDurationInSeconds;
            std::wstring QuorumLossWaitDurationInSeconds;
            std::wstring StandByReplicaKeepDurationInSeconds;

            std::wstring ServicePackageActivationMode;
        };

        class User
        {
        public:
            User()
                : AccountType()
                , AccontName()
                , AccountPassword()
                , IsPasswordEncrypted(false)
                , SystemGroups()
                , Groups()
            {
            }

            std::wstring AccountType;
            std::wstring AccontName;
            std::wstring AccountPassword;
            bool IsPasswordEncrypted;

            std::vector<std::wstring> SystemGroups;
            std::vector<std::wstring> Groups;
        };

        class Group
        {
        public:
            Group()
                : DomainGroup()
                , SystemGroups()
                , DomainUsers()
            {
            }

            std::vector<std::wstring> DomainGroup;
            std::vector<std::wstring> SystemGroups;
            std::vector<std::wstring> DomainUsers;
        };

        bool SetCodePackage(
            Common::StringCollection const& params,
            std::wstring const & clientCredentialsType);
        bool UploadApplication(
            std::wstring const& applicationFriendlyName, 
            Common::StringCollection const& params,
            Common::ComPointer<IFabricNativeImageStoreClient> const &);
        bool DeleteApplication(std::wstring const& applicationFriendlyName);
        bool SetServicePackage(Common::StringCollection const& params);
        bool SetServicetypes(Common::StringCollection const& params);
        bool SetServiceTemplates(Common::StringCollection const& params);
        bool SetDefaultService(Common::StringCollection const& params);
        bool SetUser(Common::StringCollection const& params);
        bool SetGroup(Common::StringCollection const& params);
        bool SetRunAs(Common::StringCollection const& params);
        bool SetSharedPackage(Common::StringCollection const & params);
        bool SetHealthPolicy(Common::StringCollection const& params);
        bool SetNetwork(Common::StringCollection const& params);
        bool SetEndpoint(Common::StringCollection const& params);
        bool SetParameters(Common::StringCollection const& params);
        bool Clear();

        void GetDefaultServices(Common::StringCollection & requiresServices);

        void GetApplicationManifestString(std::wstring & applicationManifest);
        void GetServiceManifestString(std::wstring & serviceManifest, ServiceModel::ServiceManifestDescription & serviceManifestDescription);

        void WriteHealthPolicy(Common::StringWriter & writer);
        void WriteStatelessServiceType(Common::StringWriter & writer, StatelessService & statelessService);
        void WriteStatefulServiceType(Common::StringWriter & writer, StatefulService & statefulService);
        DWORD GetComTimeout(Common::TimeSpan cTimeout);
		
        static std::wstring const DefaultStatelessServicePackage;
        static std::wstring const DefaultStatefulServicePackage;

        std::wstring applicationTypeName_;
        std::wstring applicationTypeVersion_;
        std::map<std::wstring, std::wstring> applicationParameters_;
        std::vector<StatelessService> statelessServiceTemplates_;
        std::vector<StatefulService> statefulServiceTemplates_;
        std::map<std::wstring, StatelessService> requiredStatelessServices_;
        std::map<std::wstring, StatefulService> requiredStatefulService_;

        std::map<std::wstring, ServiceModel::ServiceManifestDescription> serviceManifestImports_;
        std::set<std::wstring> skipUploadServiceManifests_;
        std::map<std::wstring, std::set<std::wstring>> skipUploadCodePackages_;
        std::map<std::wstring, Common::ConfigSettings> configSettings_;

        std::map<std::wstring, User> userMap_;
        std::map<std::wstring, Group> groupMap_;

        std::map<std::wstring, std::map<std::wstring, std::wstring>> runAsPolicies_;
        std::map<std::wstring, std::map<std::wstring, std::wstring>> rgPolicies_;
        std::map<std::wstring, std::map<std::wstring, std::wstring>> spRGPolicies_;
        std::wstring defaultRunAsPolicy_;
        std::shared_ptr<ServiceModel::ApplicationHealthPolicy> healthPolicy_;
        std::map<std::wstring, std::vector<std::wstring>> sharedPackagePolicies_;
        std::map<std::wstring, std::set<std::wstring>> containerHosts_;
        std::map<std::wstring, ServiceModel::NetworkPoliciesDescription> networkPolicies_;

        static Common::ExclusiveLock applicationTypesLock_;
		static std::map<std::wstring, ApplicationBuilder> applicationTypes_;
    };
};
