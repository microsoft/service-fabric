// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
  class ServiceFactoryManager :
        public Common::RootedObject,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ServiceFactoryManager)

    public:
        ServiceFactoryManager(
            Common::ComponentRoot & root,
            FabricRuntimeImpl & runtime);

        Common::AsyncOperationSPtr BeginRegisterStatelessServiceFactory(
            std::wstring const & originalServiceTypeName, 
            Common::ComPointer<IFabricStatelessServiceFactory> const & statelessFactory,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRegisterStatelessServiceFactory(
            Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr BeginRegisterStatefulServiceFactory(
            std::wstring const & originalServiceTypeName, 
            Common::ComPointer<IFabricStatefulServiceFactory> const & statefulFactory,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRegisterStatefulServiceFactory(
            Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr BeginGetStatelessServiceFactory(
			ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndGetStatelessServiceFactory(
            Common::AsyncOperationSPtr const & operation,
            __out std::wstring & originalServiceTypeName,
            __out ComProxyStatelessServiceFactorySPtr & factory);

        Common::AsyncOperationSPtr BeginGetStatefulServiceFactory(
			ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndGetStatefulServiceFactory(
            Common::AsyncOperationSPtr const & operation,
            __out std::wstring & originalServiceTypeName,
            __out ComProxyStatefulServiceFactorySPtr & factory);

        //For Test Only
        bool Test_IsServiceTypeInvalid(ServiceModel::ServiceTypeIdentifier const & serviceTypeInstanceId) const;

    private:
        // get servicetypeidentifier from the user registered service type name based on the service package
		ServiceModel::ServiceTypeIdentifier GetMappedServiceType(std::wstring const & originalServiceTypeName);

    private:
        class RegisterServiceFactoryAsyncOperation;
        friend class RegisterServiceFactoryAsyncOperation;

    private:
        FabricRuntimeImpl & runtime_;
        ServiceFactoryRegistrationTableUPtr table_;
    };
}
