// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace V1ReplPerf
{
    class ComFactory : public IFabricStatefulServiceFactory, Common::ComUnknownBase
    {
        DENY_COPY(ComFactory);

        BEGIN_COM_INTERFACE_LIST(ComFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceFactory, IFabricStatefulServiceFactory)
        END_COM_INTERFACE_LIST()

    public:
        ComFactory(Factory & factory)
            : root_(factory.shared_from_this())
            , factory_(factory)
        {
        }

        virtual ~ComFactory()
        {
        }

       HRESULT STDMETHODCALLTYPE CreateReplica( 
            LPCWSTR serviceType,
            FABRIC_URI serviceName,
            ULONG initializationDataLength,
            const byte *initializationData,
            FABRIC_PARTITION_ID partitionId,
            FABRIC_REPLICA_ID replicaId,
            IFabricStatefulServiceReplica **service);

    private:
        Common::ComponentRootSPtr root_;
        Factory & factory_;
    };
}
