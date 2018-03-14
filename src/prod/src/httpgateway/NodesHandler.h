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

        void GetAllNodes(__in Common::AsyncOperationSPtr const& operation);
        void OnGetAllNodesComplete(__in Common::AsyncOperationSPtr const &operation, __in bool expectedCompletedSynchronously);

        void GetNodeByName(__in Common::AsyncOperationSPtr const& operation);
        void OnGetNodeByNameComplete(__in Common::AsyncOperationSPtr const &operation, __in bool expectedCompletedSynchronously);

        void ActivateNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnActivateNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void DeactivateNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeactivateNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void NodeStateRemoved(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnNodeStateRemovedComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void RestartNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnRestartNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void StopNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnStopNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void StartNode(__in Common::AsyncOperationSPtr const & thisSPtr);
        void OnStartNodeComplete(__in Common::AsyncOperationSPtr const & operation, __in bool expectedCompletedSynchronously);

        void ApplicationsOnNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnApplicationsOnNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);
        void GetDeployedApplicationPagedList(
            Common::AsyncOperationSPtr const& thisSPtr,
            HandlerAsyncOperation * const handlerOperation,
            FabricClientWrapper const& client,
            bool expectApplicationName = false);
        void OnApplicationsOnNodePagedComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously,
            bool expectApplicationName);

        void ApplicationsOnNodeByName(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnApplicationsOnNodeByNameComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ServicePackagesOnNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnServicePackagesOnNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ServicePackagesOnNodeByName(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnServicePackagesOnNodeByNameComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ServiceTypesOnNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnServiceTypesOnNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ServiceTypesOnNodeByType(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnServiceTypesOnNodeByTypeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void CodePackagesOnNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnCodePackagesOnNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void RestartDeployedCodePackage(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnRestartDeployedCodePackageComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously,
            __in Management::FaultAnalysisService::RestartDeployedCodePackageDescriptionUsingNodeName const & description);

        void GetContainerLogs(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetContainerLogsComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void CallContainerApi(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnContainerApiComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ServiceReplicasOnNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnServiceReplicasOnNodeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        // GetDeployedReplicaDetail
        void ServiceReplicasOnHost(__in Common::AsyncOperationSPtr const & thisSPtr);
        void OnServiceReplicasOnHostComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectCompletedSynchronously);

        void EvaluateNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void EvaluateNodeHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void EvaluateApplicationOnNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void EvaluateApplicationOnNodeHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void EvaluateServicePackagesOnNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void EvaluateServicePackagesOnNodeHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ReportNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void ReportApplicationOnNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void ReportServicePackagesOnNodeHealth(Common::AsyncOperationSPtr const& thisSPtr);

        void RemoveReplica(__in Common::AsyncOperationSPtr const& thisSPtr);
        void RestartReplica(__in Common::AsyncOperationSPtr const& thisSPtr);

        void ReportFault(__in Common::AsyncOperationSPtr const &thisSPtr, Reliability::FaultType::Enum faultType);
        void ReportFaultComplete(__in Common::AsyncOperationSPtr const & thisSPtr, __in bool expectedCompletedSynchronously);

        void GetNodeLoadInformation(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetNodeLoadInformationComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void DeployServicePackageToNode(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeployServicePackageToNodeComplete(__in Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

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
    };
}
