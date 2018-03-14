// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ComCalculatorService : public IFabricStatelessServiceInstance, public Common::ComUnknownBase
    {
        DENY_COPY(ComCalculatorService);

        COM_INTERFACE_LIST1(
            ComCalculatorService,
            IID_IFabricStatelessServiceInstance,
            IFabricStatelessServiceInstance)

    public:
        explicit ComCalculatorService(CalculatorService & calculatorService);
        virtual ~ComCalculatorService(){ }

        HRESULT STDMETHODCALLTYPE BeginOpen( 
            IFabricStatelessServicePartition *servicePartition,
            IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndOpen( 
            IFabricAsyncOperationContext *context,
            __out IFabricStringResult **serviceEndpoint);
        
        HRESULT STDMETHODCALLTYPE BeginClose( 
            IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndClose( 
            IFabricAsyncOperationContext *context);

        void STDMETHODCALLTYPE Abort();

    private:
        Common::ComponentRootSPtr root_;
        CalculatorService & calculatorService_;
        bool isClosedCalled_;

        void CheckForReportFaultsAndDelays(Common::ComPointer<IFabricStatelessServicePartition3> partition, ApiFaultHelper::ComponentName compName, std::wstring operationName);
    };
};
