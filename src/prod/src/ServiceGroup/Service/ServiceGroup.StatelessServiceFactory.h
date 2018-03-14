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
    // Class implementing the service factory for a stateless service group.
    //
    class CStatelessServiceGroupFactory : 
        public IFabricStatelessServiceFactory, 
        public IFabricServiceGroupFactory, 
        private Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupStateless>
    {
        DENY_COPY(CStatelessServiceGroupFactory)
        
    public:
        //
        // Constructor/Destructor.
        //
        CStatelessServiceGroupFactory();
        virtual ~CStatelessServiceGroupFactory();

        BEGIN_COM_INTERFACE_LIST(CStatelessServiceGroupFactory) 
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceFactory) 
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceFactory, IFabricStatelessServiceFactory)  
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupFactory, IFabricServiceGroupFactory) 
            COM_INTERFACE_ITEM(IID_InternalStatelessServiceGroupFactory, IFabricStatelessServiceFactory)  
        END_COM_INTERFACE_LIST()

        //
        // IFabricStatelessServiceFactory methods.
        //
        STDMETHOD(CreateInstance)(
            __in LPCWSTR serviceType,
            __in LPCWSTR serviceName,
            __in ULONG initializationDataLength,
            __in const byte * initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_INSTANCE_ID instanceId,
            __out IFabricStatelessServiceInstance** serviceInstance
            );
        //
        // Initialize.
        //
        HRESULT FinalConstruct(__in const std::map<std::wstring, IFabricStatelessServiceFactory*>& statelessServiceFactories);

    protected:
        //
        // Map of service types to class factories.
        //
        std::map<std::wstring, IFabricStatelessServiceFactory*> statelessServiceFactories_;
    };
}
