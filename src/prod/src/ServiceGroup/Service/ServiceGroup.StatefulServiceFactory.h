// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"

#include "ServiceGroup.InternalPublic.h"

namespace ServiceGroup
{
    //
    // Class implementing the service factory for a stateful service group.
    //
    class CStatefulServiceGroupFactory : 
        public IFabricStatefulServiceFactory,
        public IFabricServiceGroupFactory, 
        private Common::ComUnknownBase, 
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupStateful>
    {
        DENY_COPY(CStatefulServiceGroupFactory)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulServiceGroupFactory();
        virtual ~CStatefulServiceGroupFactory();

        BEGIN_COM_INTERFACE_LIST(CStatefulServiceGroupFactory) 
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceFactory) 
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceFactory, IFabricStatefulServiceFactory)  
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupFactory, IFabricServiceGroupFactory) 
            COM_INTERFACE_ITEM(IID_InternalIStatefulServiceGroupFactory, IFabricStatefulServiceFactory)  
        END_COM_INTERFACE_LIST()

        //
        //  IFabricStatefulServiceFactory methods.
        //
        STDMETHOD(CreateReplica)(
            __in LPCWSTR serviceType,
            __in LPCWSTR serviceName,
            __in ULONG initializationDataLength,
            __in const byte* initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __out IFabricStatefulServiceReplica** serviceReplica
            );
        //
        // Initialize.
        //
        HRESULT FinalConstruct(
            __in const std::map<std::wstring, IFabricStatefulServiceFactory*>& statefulServiceFactories,
            __in const std::map<std::wstring, IFabricStatelessServiceFactory*>& statelessServiceFactories,
            __in IFabricCodePackageActivationContext * activationContext
            );

    protected:
        //
        // Map of service types to class factories.
        //
        std::map<std::wstring, IFabricStatefulServiceFactory*> statefulServiceFactories_;
        std::map<std::wstring, IFabricStatelessServiceFactory*> statelessServiceFactories_;

        IFabricCodePackageActivationContext * activationContext_;
    };
}
