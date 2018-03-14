// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ComTestHostServiceFactory : public IFabricStatelessServiceFactory, public IFabricStatefulServiceFactory, private Common::ComUnknownBase
    {
        DENY_COPY(ComTestHostServiceFactory);

        BEGIN_COM_INTERFACE_LIST(ComTestHostServiceFactory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceFactory, IFabricStatelessServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceFactory, IFabricStatefulServiceFactory)
            END_COM_INTERFACE_LIST()

    public:
        ComTestHostServiceFactory(TestHostServiceFactory & factory, std::vector<ServiceModel::ServiceTypeDescription> const& supportedServiceTypes) 
            : root_(factory.shared_from_this())
            , factory_(factory)
            , supportedServiceTypes_(supportedServiceTypes)
        { 
        }

        virtual ~ComTestHostServiceFactory()
        {
        }

        HRESULT STDMETHODCALLTYPE CreateInstance( 
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_INSTANCE_ID instanceId,
            /* [retval][out] */ IFabricStatelessServiceInstance **service);

        HRESULT STDMETHODCALLTYPE CreateReplica( 
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId,
            /* [retval][out] */ IFabricStatefulServiceReplica **service);

    private:
        Common::ComponentRootSPtr root_;
        TestHostServiceFactory & factory_;
        std::vector<ServiceModel::ServiceTypeDescription> supportedServiceTypes_;

        bool TryGetServiceTypeDescription(std::wstring const& serviceType, ServiceModel::ServiceTypeDescription & serviceTypeDescription);
        std::wstring GetInitString(const byte *initData, ULONG nBytes);
    };
};
