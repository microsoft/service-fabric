// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // REST Handler for FabricClient API's
    //  * IFabricQueryClient - GetNodeList
    //  * IFabricClusterManagementClient
    //
    class NodesHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(NodesHandler)

    public:

        NodesHandler(HttpGatewayImpl & server);
        virtual ~NodesHandler();

        Common::ErrorCode Initialize();

    private:

        //
        // Note: GetHealth is supported as a GET as well as a POST. When the request issued as a GET the default health
        // policy will be used.
        //

        void GetAllNodes(Common::AsyncOperationSPtr const& operation);
        void OnGetAllNodesComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void GetNodeByName(Common::AsyncOperationSPtr const& operation);
        void OnGetNodeByNameComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void ActivateNode(Common::AsyncOperationSPtr const& thisSPtr);
        void OnActivateNodeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void DeactivateNode(Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeactivateNodeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void NodeStateRemoved(Common::AsyncOperationSPtr const& thisSPtr);
        void OnNodeStateRemovedComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void RestartNode(Common::AsyncOperationSPtr const& thisSPtr);
        void OnRestartNodeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void StopNode(Common::AsyncOperationSPtr const& thisSPtr);
        void OnStopNodeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void StartNode(Common::AsyncOperationSPtr const & thisSPtr);
        void OnStartNodeComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void ApplicationsOnNode(Common::AsyncOperationSPtr const& thisSPtr);
        void OnApplicationsOnNodeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);
        void GetDeployedApplicationPagedList(
            Common::AsyncOperationSPtr const& thisSPtr,
            HandlerAsyncOperation * const handlerOperation,
            FabricClientWrapper const& client,
            bool expectApplicationName = false);
        void OnApplicationsOnNodePagedComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously,
            bool expectApplicationName);

        void ApplicationsOnNodeByName(Common::AsyncOperationSPtr const& thisSPtr);
        void OnApplicationsOnNodeByNameComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ServicePackagesOnNode(Common::AsyncOperationSPtr const& thisSPtr);
        void OnServicePackagesOnNodeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ServicePackagesOnNodeByName(Common::AsyncOperationSPtr const& thisSPtr);
        void OnServicePackagesOnNodeByNameComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ServiceTypesOnNode(Common::AsyncOperationSPtr const& thisSPtr);
        void OnServiceTypesOnNodeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ServiceTypesOnNodeByType(Common::AsyncOperationSPtr const& thisSPtr);
        void OnServiceTypesOnNodeByTypeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void CodePackagesOnNode(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCodePackagesOnNodeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void RestartDeployedCodePackage(Common::AsyncOperationSPtr const& thisSPtr);
        void OnRestartDeployedCodePackageComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously,
            Management::FaultAnalysisService::RestartDeployedCodePackageDescriptionUsingNodeName const & description);

        void GetContainerLogs(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetContainerLogsComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ContainerApiWrapper(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnContainerApiComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ForwardContainerApi(Common::AsyncOperationSPtr const& thisSPtr);
        void ForwardContainerApi_OnContainerApiComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);
        void ForwardContainerApi_OnGetNodeComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously,
            std::wstring const & nodeName);

        void ServiceReplicasOnNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnServiceReplicasOnNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        // GetDeployedReplicaDetail
        void ServiceReplicasOnHost(Common::AsyncOperationSPtr const & thisSPtr);
        void OnServiceReplicasOnHostComplete(Common::AsyncOperationSPtr const& operation, bool expectCompletedSynchronously);

        void EvaluateNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void EvaluateNodeHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void EvaluateApplicationOnNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void EvaluateApplicationOnNodeHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void EvaluateServicePackagesOnNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void EvaluateServicePackagesOnNodeHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ReportNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void ReportApplicationOnNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void ReportServicePackagesOnNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);

        void RemoveReplica(Common::AsyncOperationSPtr const& thisSPtr);
        void RestartReplica(Common::AsyncOperationSPtr const& thisSPtr);

        void ReportFault(Common::AsyncOperationSPtr const &thisSPtr, Reliability::FaultType::Enum faultType);
        void ReportFaultComplete(Common::AsyncOperationSPtr const & thisSPtr, bool expectedCompletedSynchronously);

        void GetNodeLoadInformation(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetNodeLoadInformationComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void DeployServicePackageToNode(Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeployServicePackageToNodeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetAllNetworksOnNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllNetworksOnNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetAllCodePackagesOnNetworkOnNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllCodePackagesOnNetworkOnNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetCodePackagesOnNetworkOnNodeByName(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetCodePackagesOnNetworkOnNodeByNameComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        bool GetDeployedNetworkCodePackageQueryDescription(
            Common::AsyncOperationSPtr const& thisSPtr,
            bool includeCodePackageName,
            __out ServiceModel::DeployedNetworkCodePackageQueryDescription & queryDescription);

        class DeactivateNodeData : public Common::IFabricJsonSerializable
        {
        public:
            DeactivateNodeData() {};

            __declspec(property(get=get_DeactivationIntent)) FABRIC_NODE_DEACTIVATION_INTENT &DeactivationIntent;
            FABRIC_NODE_DEACTIVATION_INTENT get_DeactivationIntent() const { return deactivationIntent_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::DeactivationIntent, deactivationIntent_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent_;
        };

        class NodeControlData: public Common::IFabricJsonSerializable
        {
        public:
            NodeControlData() {};

            __declspec(property(get=get_NodeInstanceId)) std::wstring const& NodeInstanceId;
            std::wstring const& get_NodeInstanceId() const { return nodeInstanceId_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeInstanceId, nodeInstanceId_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring nodeInstanceId_;
        };

        class RestartNodeControlData : public Common::IFabricJsonSerializable
        {
        public:
            RestartNodeControlData() {};

            __declspec(property(get = get_NodeInstanceId)) std::wstring const& NodeInstanceId;
            std::wstring const& get_NodeInstanceId() const { return nodeInstanceId_; }

            __declspec(property(get = get_CreateFabricDump)) std::wstring const& CreateFabricDump;
            std::wstring const& get_CreateFabricDump() const { return createFabricDump_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeInstanceId, nodeInstanceId_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::CreateFabricDump, createFabricDump_)
                END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring nodeInstanceId_;
            std::wstring createFabricDump_;
        };

        class CodePackageControlData: public Common::IFabricJsonSerializable
        {
        public:
            CodePackageControlData() {};

            __declspec(property(get=get_ServiceManifestName)) std::wstring const& ServiceManifestName;
            std::wstring const& get_ServiceManifestName() const { return serviceManifestName_; }

            __declspec(property(get = get_ServicePackageActivationId)) std::wstring const& ServicePackageActivationId;
            std::wstring const& get_ServicePackageActivationId() const { return servicePackageActivationId_; }

            __declspec(property(get=get_CodePackageName)) std::wstring const& CodePackageName;
            std::wstring const& get_CodePackageName() const { return codePackageName_; }

            __declspec(property(get=get_CodePackageInstanceId)) std::wstring const& CodePackageInstanceId;
            std::wstring const& get_CodePackageInstanceId() const { return codePackageInstanceId_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceManifestName, serviceManifestName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::CodePackageName, codePackageName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::CodePackageInstanceId, codePackageInstanceId_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServicePackageActivationId, servicePackageActivationId_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring serviceManifestName_;
            std::wstring servicePackageActivationId_;
            std::wstring codePackageInstanceId_;
            std::wstring codePackageName_;
        };

        class StartNodeData : public Common::IFabricJsonSerializable
        {
        public:
            StartNodeData()
                : clusterConnectionPort_(0),
                nodeInstanceId_(0xFFFFFFFFFFFFFFFF)
            {};

            __declspec(property(get=get_IpAddressOrFQDN)) std::wstring const& IpAddressOrFQDN;
            std::wstring const& get_IpAddressOrFQDN() const { return ipAddressOrFQDN_; }

            __declspec(property(get=get_ClusterConnectionPort)) ULONG ClusterConnectionPort;
            ULONG get_ClusterConnectionPort() const { return clusterConnectionPort_; }

             __declspec(property(get=get_NodeInstanceId)) uint64 NodeInstanceId;
            uint64 get_NodeInstanceId() const { return nodeInstanceId_; }

            bool Validate()
            {
                auto error = Utility::ValidateString(IpAddressOrFQDN, 0);
                if (!error.IsSuccess())
                {
                    return false;
                }

                if (NodeInstanceId == 0xFFFFFFFFFFFFFFFF)
                {
                    return false;
                }

                return true;
            }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::IpAddressOrFQDN, ipAddressOrFQDN_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ClusterConnectionPort, clusterConnectionPort_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeInstanceId, nodeInstanceId_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            private:
                std::wstring ipAddressOrFQDN_;
                ULONG clusterConnectionPort_;
                uint64 nodeInstanceId_;
        };

        private:
            const std::wstring localNodeName_;
    };
}
