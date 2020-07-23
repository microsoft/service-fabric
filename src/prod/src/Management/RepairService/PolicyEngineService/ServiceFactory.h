// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class ServiceFactory :
        public IFabricStatefulServiceFactory,
        private Common::ComUnknownBase
    {
        DENY_COPY(ServiceFactory);

        COM_INTERFACE_LIST1(ServiceFactory, IID_IFabricStatefulServiceFactory, IFabricStatefulServiceFactory);
        
    public:
        ServiceFactory();
        ~ServiceFactory();

        void Initialize(IFabricCodePackageActivationContext* activationContext, Common::ManualResetEvent* exitServiceHostEventPtr);

        STDMETHODIMP CreateReplica( 
            __in LPCWSTR serviceType,
            __in FABRIC_URI serviceName,
            __in ULONG initializationDataLength,
            __in const byte *initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __out IFabricStatefulServiceReplica **serviceReplica);
        
    private:
        Common::ComPointer<IFabricCodePackageActivationContext> activationContextCPtr_;
        Common::ManualResetEvent * exitServiceHostEventPtr_;
    };
}
