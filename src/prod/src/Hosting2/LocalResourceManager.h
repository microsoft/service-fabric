// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Hosting2
{
    //
    // This type represents a Local Resource Manager (LRM) that is governing all resources on a node.
    // It complements Cluster Resource Manager (PLB) that orchestrates resources on a cluster level.
    //
    class LocalResourceManager :
        public Common::RootedObject,
        public Common::FabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        friend class LocalResourceManagerTestHelper;
        DENY_COPY(LocalResourceManager);
    public:
        LocalResourceManager(
            Common::ComponentRoot const & root, 
            __in Common::FabricNodeConfigSPtr const & fabricNodeConfig,
            __in HostingSubsystem & hosting);

        // Registers a service package with LocalResourceManager.
        // If there are not enough resources, returns an error.
        // If there are enough resources, assigns cores to SP, and reserves memory for SP.
        Common::ErrorCode RegisterServicePackage(VersionedServicePackage const & package);

        // Unregisters service package from the resource governor.
        // Frees up resources that were assigned to it.
        void UnregisterServicePackage(VersionedServicePackage const & package);

        // Sets correct values for service package so that governing object (Job Object, cgroup, or container) can govern CPU correctly.
        Common::ErrorCode AdjustCPUPoliciesForCodePackage(CodePackage const & package,
            ServiceModel::ResourceGovernancePolicyDescription & rg,
            bool isContainer,
            bool isContainerGroup);

        // Retrieves the usage on this node.
        double GetResourceUsage(std::wstring const& resourceName);

        // Retrieves the capacity on this node for RG metrics.
        uint64 GetResourceNodeCapacity(std::wstring const& resourceName) const;

        // Retrieves available container images from docker
        void GetNodeAvailableContainerImages();
        void OnGetNodeAvailableContainerImagesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
        // Method for sending avilable containers images to the FM
        void SendAvailableContainerImagesToFM() const;

    private:
        // FabricComponent functions.
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();
        bool GetCapacityFromConfiguration(std::wstring const & metric, uint64 & capacity);

        double GetCpuFraction(CodePackage const & package, ServiceModel::ResourceGovernancePolicyDescription const & rg, bool & isDefined) const;

        // Checks for available resources and compares to user-provided capacities.
        Common::ErrorCode CheckAndCorrectAvailableResources();

        // Extracted to be able to call from test without creating VersionedServicePackage
        Common::ErrorCode RegisterServicePackageInternal(
            ServicePackageInstanceIdentifier const&,
            ServiceModel::ServicePackageResourceGovernanceDescription const&,
            std::wstring const& = wstring());
        void UnregisterServicePackageInternal(
            ServicePackageInstanceIdentifier const&,
            ServiceModel::ServicePackageResourceGovernanceDescription const&);

        void ReportResourceCapacityMismatch();
        void ReportResourceCapacityNotDefined(ServicePackageInstanceIdentifier const &, bool, bool, std::wstring const &);
        std::wstring GetExtraDescriptionHealthWarning(
            bool isCapacityMismatch,
            bool reportMemoryNotDefined = false,
            bool reportCpuNotDefined = false,
            std::wstring const & applicationName = wstring());

        // Used to determine node capacity when starting LRM.
        Common::FabricNodeConfigSPtr fabricNodeConfig_;

        // Number of available cores on the node.
        uint64 numAvailableCores_;
        // Number of cores that are in use multiplied with ResourceGovernanceCpuCorrectionFactor
        uint64 numUsedCoresCorrected_;

        // All SPs that are registered, and their RG settings.
        std::map<ServicePackageInstanceIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription> registeredSPs_;

        // Amount of available memory on the node.
        uint64 availableMemoryInMB_;
        // Amount of memory in use
        uint64 memoryMBInUse_;

        // Every access to LRM is guarded with this lock.
        Common::RwLock lock_;

        // reference on hostingSunsystem - needed for reporting health warnings
        HostingSubsystem & hosting_;

        // these mask will determine if node capacities are not specified correctly and we need to emit a health warning
        uint healthWarning_;
        enum HealthWarningEnum
        {
            CpuWarning = 0x0001,
            MemoryWarning = 0x0002
        };

        // Memory in MB that is present on the machine
        uint64 systemMemoryInMB_;
        // Number of cores that are present on the machine
        uint64 systemCpuCores_;

        // Timer for GetImages action
        Common::TimerSPtr getImagesTimer_;

        // Counter for errors
        int64 getImagesQueryErrors_;

        // Available images on node
        std::vector<wstring> images_;
    };

    class LocalResourceManagerTestHelper
    {
    public:
        static Common::ErrorCode RegisterServicePackage(LocalResourceManager & lrm,
            ServicePackageInstanceIdentifier const& id,
            ServiceModel::ServicePackageResourceGovernanceDescription const& rg)
        {
            return lrm.RegisterServicePackageInternal(id, rg);
        }

        static void UnregisterServicePackage(LocalResourceManager & lrm,
            ServicePackageInstanceIdentifier const& id,
            ServiceModel::ServicePackageResourceGovernanceDescription const& rg)
        {
            lrm.UnregisterServicePackageInternal(id, rg);
        }
    };
}
