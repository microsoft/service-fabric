// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CHECK_PARAMS(Min, Max, Command)              \
    if (params.size() < Min || params.size() > Max)  \
    {                                                \
        FABRICSESSION.PrintHelp(Command);            \
        return false;                                \
    }                                                \

#define CHECK_MAX_PARAMS(Max, Command)               \
    if (params.size() > Max)                         \
    {                                                \
        FABRICSESSION.PrintHelp(Command);            \
        return false;                                \
    }                                                \


namespace FabricTest
{
    class FabricTestSession;

    class TestFabricClient
    {
        DENY_COPY(TestFabricClient);

        friend class TestFabricClientHealth;
        friend class TestFabricClientQuery;
        friend class TestFabricClientUpgrade;
        friend class TestFabricClientScenarios;

    public:
        TestFabricClient();
        virtual ~TestFabricClient() {}

        bool CreateName(Common::StringCollection const & params);
        bool DeleteName(Common::StringCollection const & params);
        bool DnsNameExists(Common::StringCollection const & params);
        bool NameExists(Common::StringCollection const & params);
        bool PutProperty(Common::StringCollection const & params);
        bool PutCustomProperty(Common::StringCollection const & params);
        bool SubmitPropertyBatch(Common::StringCollection const & params);
        bool DeleteProperty(Common::StringCollection const & params);
        bool GetProperty(Common::StringCollection const & params);
        bool GetMetadata(Common::StringCollection const & params);
        bool EnumerateNames(Common::StringCollection const & params);
        bool EnumerateProperties(Common::StringCollection const & params);
        bool Query(Common::StringCollection const & params);
        bool VerifyPropertyEnumeration(Common::StringCollection const & params);

        bool CreateService(Common::StringCollection const & params);
        bool UpdateService(Common::StringCollection const & params);
        bool CreateServiceFromTemplate(Common::StringCollection const & params);
        bool DeleteService(std::wstring const& serviceName, bool const isForce, std::vector<HRESULT> const& expectedErrors, Common::ComPointer<IFabricServiceManagementClient5> const & client);
        bool GetServiceDescription(Common::StringCollection const & params, Common::ComPointer<IFabricServiceManagementClient> const & client = Common::ComPointer<IFabricServiceManagementClient>());
        bool GetServiceGroupDescription(Common::StringCollection const & params);
        bool DeactivateNode(Common::StringCollection const& params, bool & updateNode, Federation::NodeId & nodeToUpdate);
        bool ActivateNode(Common::StringCollection const& params, Federation::NodeId & nodeToUpdate);
        bool NodeStateRemoved(Common::StringCollection const& params);
        bool RecoverPartitions(Common::StringCollection const& params);
        bool RecoverPartition(Common::StringCollection const& params);
        bool RecoverServicePartitions(Common::StringCollection const& params);
        bool RecoverSystemPartitions(Common::StringCollection const& params);
        bool ReportFault(Common::StringCollection const & params);
        bool ResetPartitionLoad(Common::StringCollection const& params);
        bool ToggleVerboseServicePlacementHealthReporting(Common::StringCollection const& params);

        bool MoveSecondaryReplicaFromClient(Common::StringCollection const & params);
        bool MovePrimaryReplicaFromClient(Common::StringCollection const & params);

        bool CreateServiceGroup(Common::StringCollection const & params);
        bool UpdateServiceGroup(Common::StringCollection const & params);
        bool DeleteServiceGroup(std::wstring const& serviceName, std::vector<HRESULT> const& expectedErrors);

        bool InfrastructureCommand(Common::StringCollection const & params);

        bool CreateRepair(Common::StringCollection const & params);
        bool CancelRepair(Common::StringCollection const & params);
        bool ForceApproveRepair(Common::StringCollection const & params);
        bool DeleteRepair(Common::StringCollection const & params);
        bool UpdateRepair(Common::StringCollection const & params);
        bool ShowRepairs(Common::StringCollection const & params);
        bool UpdateRepairHealthPolicy(StringCollection const & params);
        bool GetRepair(StringCollection const & params);

        bool CreateNetwork(Common::StringCollection const & params);
        bool DeleteNetwork(Common::StringCollection const & params);
        bool GetNetwork(Common::StringCollection const & params);
        bool ShowNetworks(Common::StringCollection const & params);

        bool DeployServicePackages(Common::StringCollection const & params);

        void DeployServicePackageToNode(
            std::wstring const & applicationTypeName,
            std::wstring const & applicationTypeVersion,
            std::wstring const & serviceManifestName,
            std::wstring const & nodeName,
            std::wstring const & nodeId,
            FABRIC_PACKAGE_SHARING_POLICY_LIST const & sharingPolicies,
            std::vector<std::wstring> const & expectedInCache,
            std::vector<std::wstring> const & expectedShared);

        bool VerifyDeployedCodePackageCount(Common::StringCollection const & params);

        ULONG VerifyDeployedCodePackageCount(
            std::wstring const & nodeName,
            std::wstring const & applicationName,
            std::wstring const & serviceManifestName);

        bool AddBehavior(Common::StringCollection const & params);
        bool RemoveBehavior(Common::StringCollection const & params);
        bool CheckUnreliableTransportIsDisabled(Common::StringCollection const & params);
        bool CheckTransportBehaviorlist(Common::StringCollection const & params);

        static std::vector<Common::NamingUri> GetPerformanceTestNames(int count, __in CommandLineParser &);
        static std::vector<Common::NamingUri> GetPerformanceTestNames(int count, __in CommandLineParser &, __out Common::NamingUri & baseName);
        static std::vector<Naming::PartitionedServiceDescriptor> GetPerformanceTestServiceDescriptions(
            int serviceCount,
            __in CommandLineParser & parser,
            std::wstring const & appName,
            std::wstring const & serviceType,
            std::vector<Common::NamingUri> const & serviceNames);

        bool PerformanceTest(Common::StringCollection const & params);
        bool PerformanceTestVariation0(Common::StringCollection const & params);
        bool PerformanceTestVariation1(Common::StringCollection const & params);
        bool PerformanceTestVariation2(Common::StringCollection const & params);
        bool PerformanceTestVariation3(Common::StringCollection const & params);

        bool RegisterServiceNotificationFilter(Common::StringCollection const & params);
        bool UnregisterServiceNotificationFilter(Common::StringCollection const & params);

        ITestStoreServiceSPtr ResolveService(
            std::wstring const& serviceName,
            HRESULT expectedError,
            ServiceLocationChangeClientSPtr const & client,
            std::wstring const & cacheItemName,
            bool skipVerifyLocation,
            HRESULT expectedCompareVersionError = S_OK);

        ITestStoreServiceSPtr ResolveService(
            std::wstring const& serviceName,
            __int64 key,
            HRESULT expectedError,
            ServiceLocationChangeClientSPtr const & client,
            std::wstring const & cacheItemName,
            bool skipVerifyLocation,
            HRESULT expectedCompareVersionError = S_OK);

        ITestStoreServiceSPtr ResolveService(
            std::wstring const& serviceName,
            std::wstring const & key,
            HRESULT expectedError,
            ServiceLocationChangeClientSPtr const & client,
            std::wstring const & cacheItemName,
            bool skipVerifyLocation,
            bool savePartitionId,
            bool verifyPartitionId,
            HRESULT expectedCompareVersionError = S_OK);

        ITestStoreServiceSPtr ResolveService(
            std::wstring const& serviceName,
            FABRIC_PARTITION_KEY_TYPE keyType,
            void const * key,
            Common::Guid const& fuId,
            HRESULT expectedError,
            bool isCommand,
            ServiceLocationChangeClientSPtr const & client,
            std::wstring const & cacheItemName,
            bool allowAllError,
            bool skipVerifyLocation = false,
            HRESULT expectedCompareVersionError = S_OK,
            bool savePartitionId = false,
            bool verifyPartitionId = false);

        ITestStoreServiceSPtr ResolveServiceWithEvents(
            std::wstring const& serviceName,
            FABRIC_PARTITION_KEY_TYPE keyType,
            void const * key,
            Common::Guid const& fuId,
            HRESULT expectedError,
            bool isCommand);

        bool PrefixResolveService(
            CommandLineParser const &, 
            Api::IClientFactoryPtr const &);

        void GetPartitionedServiceDescriptors(
            Common::StringCollection const& serviceNames,
            std::wstring const & gatewayAddress,
            __out std::vector<Naming::PartitionedServiceDescriptor> & serviceDescriptors);

        void GetPartitionedServiceDescriptors(
            Common::StringCollection const& serviceNames,
            __out std::vector<Naming::PartitionedServiceDescriptor> & serviceDescriptors);

        void GetPartitionedServiceDescriptor(
            std::wstring const & serviceName,
            Common::TimeSpan const operationTimeout,
            __out Naming::PartitionedServiceDescriptor &);

        void StartTestFabricClient(ULONG clientThreads, ULONG nameCount, ULONG propertiesPerName, int64 clientOperationInterval);
        void StopTestFabricClient();
        void VerifyNodeLoad(wstring nodeName, wstring metricName, int64 expectedLoadValue, double expectedDoubleLoad = -1.0);
        bool GetNodeLoadForResources(wstring nodeName, double & cpuUsage, double & memoryUsage);
        void VerifyClusterLoad(wstring metricName,
            int64 expectedClusterLoad,
            int64 expectedMaxNodeLoadValue,
            int64 expectedMinNodeLoadValue,
            double expectedDeviation,
            double expectedDoubleLoad = -1,
            double expectedDoubleMaxNodeLoad = -1,
            double expectedDoubleMinNodeLoad = -1);
        void VerifyPartitionLoad(FABRIC_PARTITION_ID partitionId, wstring metricName, int64 expectedPrimaryLoad, int64 expectedSecondaryLoad);
        void VerifyUnplacedReason(std::wstring serviceName, std::wstring reason);
        void GetApplicationLoadInformation(std::wstring applicationName, ServiceModel::ApplicationLoadInformationQueryResult & result);

        static bool GetServiceMetrics(Common::StringCollection const & metricsString, std::vector<Reliability::ServiceLoadMetricDescription> & metrics, bool isStateful);
        static bool GetCorrelations(Common::StringCollection const & correlationCollection, std::vector<Reliability::ServiceCorrelationDescription> & serviceCorrelations);
        static bool GetPlacementPolicies(Common::StringCollection const & policiesCollection, vector<ServiceModel::ServicePlacementPolicyDescription> & placementPolicies);
        static bool GetServiceScalingPolicy(std::wstring const & scalingCollection, vector<Reliability::ServiceScalingPolicyDescription> & scalingPolicy);

        typedef std::function<HRESULT(DWORD const, Common::ComPointer<ComCallbackWaiter> const &, Common::ComPointer<IFabricAsyncOperationContext> &)>
            BeginFabricClientOperationCallback;

        typedef std::function<HRESULT(Common::ComPointer<IFabricAsyncOperationContext> &)>
            EndFabricClientOperationCallback;

        static ComPointer<IFabricClientSettings2> FabricCreateClient(__in FabricTestFederation & testFederation);

        static Common::ComPointer<Api::ComFabricClient> FabricCreateComFabricClient(__in FabricTestFederation & testFederation);

        static Api::IClientFactoryPtr FabricCreateNativeClientFactory(__in FabricTestFederation & testFederation);

        static HRESULT FabricGetLastErrorMessageString(wstring & message);

        static HRESULT PerformFabricClientOperation(
            std::wstring const & operationName,
            Common::TimeSpan const,
            BeginFabricClientOperationCallback const &,
            EndFabricClientOperationCallback const &,
            HRESULT expectedError = S_OK,
            HRESULT allowedErrorOnRetry = S_OK,
            HRESULT retryableError = S_OK,
            bool failOnError = true,
            std::wstring const & expectedErrorMessageFragment = L"");

        static HRESULT PerformFabricClientOperation(
            std::wstring const & operationName,
            Common::TimeSpan const,
            BeginFabricClientOperationCallback const &,
            EndFabricClientOperationCallback const &,
            std::vector<HRESULT> expectedErrors,
            HRESULT allowedErrorOnRetry = S_OK,
            HRESULT retryableError = S_OK,
            bool failOnError = true,
            std::wstring const & expectedErrorMessageFragment = L"");

        static std::wstring const EmptyValue;

    private:

        class ComTestServiceChangeHandlerImpl
            : public IFabricServicePartitionResolutionChangeHandler
            , private Common::ComUnknownBase
        {
            BEGIN_COM_INTERFACE_LIST(ComTestServiceChangeHandlerImpl)
                COM_INTERFACE_ITEM(IID_IUnknown,IFabricServicePartitionResolutionChangeHandler)
                COM_INTERFACE_ITEM(IID_IFabricServicePartitionResolutionChangeHandler,IFabricServicePartitionResolutionChangeHandler)
            END_COM_INTERFACE_LIST()

        public:
            ComTestServiceChangeHandlerImpl(
                std::function<void(IFabricServiceManagementClient *, LONGLONG handlerId, IFabricResolvedServicePartitionResult *partition, HRESULT error)> const & callback)
                : callback_(callback)
            {
            }

            void STDMETHODCALLTYPE OnChange(
                IFabricServiceManagementClient * source,
                LONGLONG handlerId,
                IFabricResolvedServicePartitionResult * partition,
                HRESULT error)
            {
                callback_(source, handlerId, partition, error);
            }

        private:
            std::function<void(IFabricServiceManagementClient *, LONGLONG handlerId, IFabricResolvedServicePartitionResult *partition, HRESULT error)> callback_;
        };

    private:

        typedef std::shared_ptr<TestNamingEntry> TestNamingEntrySPtr;
        typedef std::pair<std::wstring, std::wstring> WStringKeyValue;

        void CreateName(Common::NamingUri const& name, std::wstring const& clientName, HRESULT expectedError);
        void CreateName(Common::NamingUri const& name, std::wstring const& clientName, vector<HRESULT> expectedErrors);
        void DeleteName(Common::NamingUri const& name, std::wstring const& clientName, HRESULT expectedError);
        void NameExists(Common::NamingUri const& name, bool expectedExists, std::wstring const& clientName, HRESULT expectedError);

        void PutProperty(
            Common::NamingUri const& name,
            std::wstring const& propertyName,
            std::wstring const& value,
            HRESULT expectedError,
            HRESULT retryableError = S_OK);
        bool PutCustomProperty(
            Common::NamingUri const& name,
            FABRIC_PUT_CUSTOM_PROPERTY_OPERATION const & operation,
            HRESULT expectedError,
            HRESULT retryableError = S_OK);
        bool SubmitPropertyBatch(
            Common::NamingUri const& name,
            std::wstring const& clientName,
            Common::StringCollection const& batchOperations,
            Common::StringCollection const& expectedGetProperties,
            ULONG expectedFailedIndex,
            HRESULT expectedError);
        void DeleteProperty(
            Common::NamingUri const& name,
            std::wstring const& propertyName,
            HRESULT expectedError);
        void GetProperty(
            Common::NamingUri const& name,
            std::wstring const& propertyName,
            std::wstring const& expectedValue,
            std::wstring const& expectedCustomTypeId,
            std::wstring const& seqVarName,
            HRESULT expectedError);
        void GetMetadata(
            Common::NamingUri const& name,
            std::wstring const& propertyName,
            HRESULT expectedError,
            __out Common::ComPointer<IFabricPropertyMetadataResult> & propertyResult);
        void CheckMetadata(
            FABRIC_NAMED_PROPERTY_METADATA const * metadata,
            std::wstring const& propertyName,
            std::wstring const& expectedCustomTypeId,
            ULONG expectedValueSize = static_cast<ULONG>(-1) /*unknown*/);
        void CheckProperty(
            Common::ComPointer<IFabricPropertyValueResult> const& propertyResult,
            std::wstring const& propertyName,
            std::wstring const& expectedValue,
            std::wstring const& expectedCustomTypeId);
        void GetDnsNamePropertyValue(
            std::wstring const& propertyName,
            std::wstring const& expectedValue,
            HRESULT expectedError);
        void CheckDnsProperty(
            Common::ComPointer<IFabricPropertyValueResult> const& propertyResult,
            std::wstring const& expectedValue);
        int EnumerateNames(
            Common::NamingUri const& parentName,
            bool doRecursive,
            std::wstring const& clientName,
            size_t maxResults,
            bool checkExpectedNames,
            __in StringCollection & expectedNames,
            bool checkStatus,
            FABRIC_ENUMERATION_STATUS expectedStatus,
            HRESULT expectedError);
        void EnumerateProperties(
            Common::NamingUri const & name,
            bool includeValues,
            size_t maxResults,
            FABRIC_ENUMERATION_STATUS expectedStatus,
            HRESULT expectedError);
        void VerifyPropertyEnumeration(Common::NamingUri const & name, bool includeValues);

        void CreateService(
            FABRIC_SERVICE_DESCRIPTION const& serviceDescription,
            std::vector<HRESULT> const& expectedErrors,
            Common::ComPointer<IFabricServiceManagementClient> const& = Common::ComPointer<IFabricServiceManagementClient>());
        void UpdateService(
            std::wstring const& serviceName,
            FABRIC_SERVICE_UPDATE_DESCRIPTION const& serviceUpdateDescription,
            std::vector<HRESULT> const& expectedErrors,
            Common::ComPointer<IFabricServiceManagementClient2> const &);
        void CreateServiceFromTemplate(Common::NamingUri const& serviceName, std::wstring const& type, Common::NamingUri const& applicationName, HRESULT expectedError, Naming::PartitionedServiceDescriptor & paritionedDescriptor);

        void CreateServiceGroup(FABRIC_SERVICE_GROUP_DESCRIPTION const& serviceDescription, HRESULT expectedError);
        void UpdateServiceGroup(std::wstring const& serviceGroupName, FABRIC_SERVICE_GROUP_UPDATE_DESCRIPTION const& serviceGroupUpdateDescription, std::vector<HRESULT> const& expectedErrors);

        std::wstring GetKeyValue(FABRIC_PARTITION_KEY_TYPE keyType, void const * key);
        void InfrastructureCommandIS(std::wstring const & targetServiceName, StringCollection const & commandTokens, HRESULT expectedError, HRESULT expectedQueryError);
        void InfrastructureCommandCM(StringCollection const & commandTokens, HRESULT expectedError, BOOLEAN expectedComplete);

        void GetPartitionedServiceDescriptors(
            Common::StringCollection const& serviceNames,
            std::wstring const & gatewayAddress,
            Common::TimeSpan const operationTimeout,
            __out std::vector<Naming::PartitionedServiceDescriptor> & serviceDescriptors);

        void GetPartitionedServiceDescriptor(
            std::wstring const & serviceName,
            Common::ComPointer<IFabricServiceManagementClient> const &,
            __out Naming::PartitionedServiceDescriptor &);

        void GetPartitionedServiceDescriptor(
            std::wstring const & serviceName,
            std::wstring const & gatewayAddress,
            Common::TimeSpan const operationTimeout,
            __out Naming::PartitionedServiceDescriptor &);

        void GetPartitionedServiceDescriptor(
            std::wstring const & serviceName,
            __out Naming::PartitionedServiceDescriptor &);

        void GetServiceDescription(
            Common::NamingUri const& name,
            HRESULT expectedError,
            Common::ComPointer<IFabricServiceManagementClient> const & client,
            Common::ComPointer<IFabricServiceDescriptionResult> & result);

        void GetServiceGroupDescription(Common::NamingUri const& name, HRESULT expectedError, Common::ComPointer<IFabricServiceGroupDescriptionResult> & result);

        void DeactivateNode(std::wstring const& nodeName, FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent, HRESULT expectedError);
        void ActivateNode(std::wstring const& nodeName);
        void NodeStateRemoved(std::wstring const& nodeHostName, HRESULT expectedError);
        void RecoverPartitions(HRESULT expectedError);
        void RecoverPartition(FABRIC_PARTITION_ID partitionId, HRESULT expectedError);
        void RecoverServicePartitions(std::wstring const& serviceName, HRESULT expectedError);
        void RecoverSystemPartitions(HRESULT expectedError);
        void ResetPartitionLoad(FABRIC_PARTITION_ID partitionId, HRESULT expectedError);
        void ToggleVerboseServicePlacementHealthReporting(bool enabled, HRESULT expectedError);
        void ReportFault(std::wstring const & nodeName, FABRIC_PARTITION_ID partitionId, FABRIC_REPLICA_ID replicaId, FABRIC_FAULT_TYPE faultType, bool isForce, HRESULT expectedError);
        void MovePrimaryReplicaFromClient(std::wstring const & nodeName, FABRIC_PARTITION_ID partitionId, bool ignoreConstraints, HRESULT expectedError);
        void MoveSecondaryReplicaFromClient(std::wstring const & currentNodeName, std::wstring const & newNodeName, FABRIC_PARTITION_ID partitionId, bool ignoreConstraints, HRESULT expectedError);

        ComPointer<IFabricNetworkManagementClient> CreateNetworkClient();
        ComPointer<IFabricGetNetworkListResult> GetNetworkList(
            __in std::wstring const & operationName,
            __in ComPointer<IFabricNetworkManagementClient> const & networkClient,
            __in FABRIC_NETWORK_QUERY_DESCRIPTION const & queryDescription) const;

        static bool CheckExpectedErrorMessage(
            std::wstring const & operationName,
            bool beginMethod,
            HRESULT hr,
            std::wstring const & expectedErrorMessageFragment);

        static HRESULT PerformFabricClientOperation(
            std::wstring const & operationName,
            Common::TimeSpan const,
            BeginFabricClientOperationCallback const &,
            EndFabricClientOperationCallback const &,
            std::vector<HRESULT> const & expectedErrors,
            std::vector<HRESULT> const & allowedErrorsOnRetry,
            std::vector<HRESULT> const & retryableErrors,
            bool failOnError = true,
            std::wstring const & expectedErrorMessageFragment = L"");

        bool NameExistsInternal(Common::NamingUri const& name, std::wstring const& clientName, HRESULT expectedError);
        void DoWork(ULONG threadId);
        void SignalThreadDone();

        bool ValidatePredeployedBinaries(
            std::wstring const & applicationTypeName,
            std::wstring const & serviceManifestName,
            std::wstring const & nodeId,
            std::vector<std::wstring> const & expectedInCache,
            std::vector<std::wstring> const & expectedShared);

        ComPointer<IFabricRepairManagementClient2> CreateRepairClient();
        ComPointer<IFabricGetRepairTaskListResult> GetRepairTaskList(
            __in std::wstring const & operationName,
            __in ComPointer<IFabricRepairManagementClient2> const & repairClient,
            __in FABRIC_REPAIR_TASK_QUERY_DESCRIPTION const & queryDescription) const;
        HRESULT GetExpectedHResult(
            __in CommandLineParser const & parser, 
            __in std::wstring const & name, 
            __in std::wstring const & defaultValue = L"Success") const;

        bool CompareRepairIntProperty(
            __in CommandLineParser const & parser,
            __in wstring const & parsedPropertyName,
            __in wstring const & propertyName,
            __in int actualValue) const;
        bool CompareRepairBoolProperty(
            __in CommandLineParser const & parser, 
            __in wstring const & parsedPropertyName,
            __in wstring const & propertyName, 
            __in bool actualValue) const;

    private:

        void SaveSequenceNumber(std::wstring const & varName, uint64 value);
        bool TryLoadSequenceNumber(std::wstring const & varName, __out uint64 & value);

        void SavePartitionId(std::wstring const & serviceName, std::wstring const & partitionName, Common::Guid const & partitionId);
        void VerifySavedPartitiondId(std::wstring const & serviceName, std::wstring const & partitionName, Common::Guid const & toVerify);

        std::vector<TestNamingEntrySPtr> testNamingEntries_;
        volatile LONG clientThreads_;
        ULONG clientThreadRange_;
        Common::ManualResetEvent threadDoneEvent_;
        Common::Random random_;
        int64 maxClientOperationInterval_;
        bool closed_;
        std::map<std::wstring, uint64> propertySequenceNumberVariables_;
        std::map<std::wstring, Common::Guid> namedPartitionIdMap_;
    };
}
