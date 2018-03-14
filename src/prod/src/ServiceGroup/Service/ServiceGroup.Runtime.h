// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"
#include "FabricClient.h"

namespace ServiceGroup
{
    class CServiceGroupFactoryBuilder : 
        public IFabricServiceGroupFactoryBuilder,
        public Common::ComUnknownBase, 
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupCommon>
    {
        DENY_COPY(CServiceGroupFactoryBuilder)
    
    public:
        //
        // Constructor/Destructor.
        //
        CServiceGroupFactoryBuilder(__in IFabricCodePackageActivationContext * activationContext);
        virtual ~CServiceGroupFactoryBuilder(void);

        BEGIN_COM_INTERFACE_LIST(CServiceGroupFactoryBuilder)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricServiceGroupFactoryBuilder)
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupFactoryBuilder, IFabricServiceGroupFactoryBuilder)
        END_COM_INTERFACE_LIST()

        //
        // IFabricServiceGroupFactoryBuilder methods.
        //
        STDMETHOD(AddStatelessServiceFactory)(
            __in LPCWSTR memberServiceType, 
            __in IFabricStatelessServiceFactory * factory);
        
        STDMETHOD(AddStatefulServiceFactory)(
            __in LPCWSTR memberServiceType, 
            __in IFabricStatefulServiceFactory * factory);

        STDMETHOD(RemoveServiceFactory)(__in LPCWSTR memberServiceType);
        
        STDMETHOD(ToServiceGroupFactory)(__out IFabricServiceGroupFactory ** factory);

    protected:
        //
        // Concurrency control.
        //
        Common::RwLock lock_;
        //
        // Map of service types to class factories.
        //
        std::map<std::wstring, IFabricStatefulServiceFactory*> statefulServiceFactories_;
        std::map<std::wstring, IFabricStatelessServiceFactory*> statelessServiceFactories_;

        IFabricCodePackageActivationContext * activationContext_;
    };
}
