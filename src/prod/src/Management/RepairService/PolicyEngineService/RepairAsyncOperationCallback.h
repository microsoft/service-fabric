// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class RepairPolicyEngineConfiguration;
    class NodeRepairInfo;
    typedef std::function<HRESULT (IFabricAsyncOperationContext*, 
                                   Common::ComPointer<IFabricRepairManagementClient>,
                                   std::shared_ptr<NodeRepairInfo>)> EndFunction;
  
    class RepairAsyncOperationCallback : public FabricAsyncOperationCallback
    {
    public:
        FABRIC_SEQUENCE_NUMBER CommitVersion;

        void Initialize(Common::ComPointer<IFabricRepairManagementClient> fabricRepairManagementClientCPtr,
                        std::shared_ptr<NodeRepairInfo>,
                        EndFunction endFunction);
        void Initialize(Common::ComPointer<IFabricRepairManagementClient> fabricManagementClientCPtr);
        HRESULT STDMETHODCALLTYPE Wait(void);

        /*IFabricAsyncOperationCallback members*/
        void STDMETHODCALLTYPE Invoke(/* [in] */ IFabricAsyncOperationContext *context);

    private:
        Common::ComPointer<IFabricRepairManagementClient> _fabricRepairManagementClientCPtr;
        std::shared_ptr<NodeRepairInfo> _nodeRepairInfo;
        Common::ManualResetEvent _completed;
        HRESULT _hresult;
        EndFunction _endFunction;
    };
}
