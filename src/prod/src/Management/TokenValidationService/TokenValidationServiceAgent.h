// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace TokenValidationService
    {
        class TokenValidationServiceAgent 
            : public Api::ITokenValidationServiceAgent
            , public Common::ComponentRoot
            , private Federation::NodeTraceComponent<Common::TraceTaskCodes::TokenValidationService>
        {
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::TokenValidationService>::WriteNoise;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::TokenValidationService>::WriteInfo;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::TokenValidationService>::WriteWarning;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::TokenValidationService>::WriteError;

        private:
            static Common::GlobalWString ExpirationClaim;

        public:
            virtual ~TokenValidationServiceAgent();

            static Common::ErrorCode Create(__out std::shared_ptr<TokenValidationServiceAgent> & result);

            __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
            std::wstring const & get_TraceId() const { return NodeTraceComponent::TraceId; }

            virtual void Release();

            virtual void RegisterTokenValidationService(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID,
                Api::ITokenValidationServicePtr const &,  
                __out std::wstring & serviceAddress);

            virtual void UnregisterTokenValidationService(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID);


        private:
           TokenValidationServiceAgent(
                 Federation::NodeInstance const & nodeInstance,
                __in Transport::IpcClient & ipcClient,
                Common::ComPointer<IFabricRuntime> const & runtimeCPtr);

           void GetTokenServiceMetadata(Api::ITokenValidationServicePtr const &,
                __in Transport::IpcReceiverContextUPtr & );
           
           
           class DispatchMessageAsyncOperationBase;
           class ValidateTokenAsyncOperation;

            void ReplaceServiceLocation(SystemServices::SystemServiceLocation const & location);
            void DispatchMessage(Api::ITokenValidationServicePtr const &, __in Transport::MessageUPtr &, __in Transport::IpcReceiverContextUPtr &);
            void OnDispatchMessageComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            Common::ErrorCode Initialize();

            SystemServices::ServiceRoutingAgentProxyUPtr routingAgentProxyUPtr_;
            std::list<SystemServices::SystemServiceLocation> registeredServiceLocations_;
            Common::ExclusiveLock lock_;

            Common::ComPointer<IFabricRuntime> runtimeCPtr_;
        };
    }
}
