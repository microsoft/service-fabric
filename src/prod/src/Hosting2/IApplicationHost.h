// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // The base interface for accessing an application host 
    class IApplicationHost
    {
        DENY_COPY(IApplicationHost);

    public:
        IApplicationHost() {}

        virtual bool IsLeaseExpired() = 0;

        virtual Common::AsyncOperationSPtr BeginGetFactoryAndCreateInstance(
            std::wstring const & runtimeId,
            ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & isolationContext,
            std::wstring const & serviceName,
            std::vector<byte> const & initializationData,
            Common::Guid const & partitionId,
            int64 instanceId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndGetFactoryAndCreateInstance(
            Common::AsyncOperationSPtr const & operation,
            __out Common::ComPointer<IFabricStatelessServiceInstance> & statelessService) = 0;

        virtual Common::AsyncOperationSPtr BeginGetFactoryAndCreateReplica(
            std::wstring const & runtimeId,
            ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & isolationContext,
            std::wstring const & serviceName,
            std::vector<byte> const & initializationData,
            Common::Guid const & partitionId,
            int64 replicaId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndGetFactoryAndCreateReplica(
            Common::AsyncOperationSPtr const & operation,
            __out Common::ComPointer<IFabricStatefulServiceReplica> & statefulService) = 0;

        virtual Common::ErrorCode GetHostInformation(
            std::wstring & nodeIdOut,
            uint64 & nodeInstanceIdOut,
            std::wstring & hostIdOut,
            std::wstring & nodeNameOut) = 0;

        virtual Common::ErrorCode GetCodePackageActivationContext(
            __in std::wstring const & runtimeId,
            __out Common::ComPointer<IFabricCodePackageActivationContext> & codePackageActivationContext) = 0;

        virtual Common::ErrorCode GetCodePackageActivator(
            __out Common::ComPointer<IFabricCodePackageActivator> & codePackageActivator) = 0;

        virtual Common::ErrorCode GetKtlSystem(
            __out KtlSystem ** ktlSystem) = 0;

        virtual ~IApplicationHost() {}
    };
}
