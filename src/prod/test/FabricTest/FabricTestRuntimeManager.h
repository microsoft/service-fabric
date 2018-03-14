// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;

    class FabricTestRuntimeManager
    {
        DENY_COPY(FabricTestRuntimeManager)

    public:
        FabricTestRuntimeManager(
            Federation::NodeId nodeId, 
            std::wstring const & runtimeServiceAddress,
            std::wstring const & namingServiceListenAddress,
            bool retryCreateHost,
            Transport::SecuritySettings const & clientSecuritySettings);

        ~FabricTestRuntimeManager();


        void CreateAllRuntimes();

        __declspec (property(get=get_ServiceFactory)) std::shared_ptr<TestServiceFactory> const& ServiceFactory;
        std::shared_ptr<TestServiceFactory> const& get_ServiceFactory() const
        { 
            return serviceFactory_;
        }

        __declspec (property(get=get_ServiceGroupFactory)) std::shared_ptr<SGServiceFactory> const& ServiceGroupFactory;
        std::shared_ptr<SGServiceFactory> const& get_ServiceGroupFactory() const
        { 
            return serviceGroupFactory_;
        }

        __declspec (property(get=get_NamingServiceListenAddress)) std::wstring const & NamingServiceListenAddress;
        std::wstring const & get_NamingServiceListenAddress() const { return namingServiceListenAddress_; }

        void AddStatelessFabricRuntime(std::vector<std::wstring> serviceTypes);
        void AddStatefulFabricRuntime(std::vector<std::wstring> serviceTypes);
        void AddStatelessHost();
        void AddStatefulHost();
        void UnregisterStatelessFabricRuntime();
        void UnregisterStatefulFabricRuntime();
        void RemoveStatelessFabricRuntime(bool abort = false);
        void RemoveStatefulFabricRuntime(bool abort = false);
        void RemoveStatelessHost(bool abort = false);
        void RemoveStatefulHost(bool abort = false);
        void RegisterStatelessServiceType(std::vector<std::wstring> const & serviceTypes);
        void RegisterStatefulServiceType(std::vector<std::wstring> const & serviceTypes);

        ReliabilityTestApi::ReconfigurationAgentComponentTestApi::ReconfigurationAgentProxyTestHelperUPtr GetProxyForStatefulHost();

    private:        
        Common::ComPointer<Hosting2::ComFabricRuntime> statelessFabricRuntime_;
        Common::ComPointer<Hosting2::ComFabricRuntime> statefulFabricRuntime_;
        Hosting2::ApplicationHostSPtr statelessHost_;
        Hosting2::ApplicationHostSPtr statefulHost_;
        std::shared_ptr<TestServiceFactory> serviceFactory_;
        std::shared_ptr<SGServiceFactory> serviceGroupFactory_;
        std::vector<std::wstring> statelessServiceTypeMap_;
        std::vector<std::wstring> statefulServiceTypeMap_;
        bool retryCreateHost_;
        
        Federation::NodeId nodeId_;
        std::wstring runtimeServiceAddress_;        
        std::wstring namingServiceListenAddress_;
        Api::IClientFactoryPtr clientFactory_;

        static const int MaxRetryCount;

        bool VerifyRegistrationTable(std::vector<std::wstring> const& serviceTypeMap, std::wstring const& runtimeId, std::wstring const& hostId);

        typedef std::function<void(TimeSpan , Common::AsyncCallback const&)> 
            BeginHostOperationCallback;

        typedef std::function<Common::ErrorCode(Common::AsyncOperationSPtr const&)> 
            EndHostOperationCallback;

         void PerformRetryableHostOperation(
            std::wstring const & operationName,
            BeginHostOperationCallback const &,
            EndHostOperationCallback const &);
    };
};
