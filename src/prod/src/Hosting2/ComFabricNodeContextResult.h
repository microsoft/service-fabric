// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class ComFabricNodeContextResult :
        public IFabricNodeContextResult2,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricNodeContextResult)

        BEGIN_COM_INTERFACE_LIST(ComFabricNodeContextResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricNodeContextResult)
            COM_INTERFACE_ITEM(IID_IFabricNodeContextResult, IFabricNodeContextResult)
            COM_INTERFACE_ITEM(IID_IFabricNodeContextResult2, IFabricNodeContextResult2)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricNodeContextResult(FabricNodeContextResultImplSPtr impl);
        virtual ~ComFabricNodeContextResult();

         // 
        // IFabricNodeContextResult methods
        // 
        const FABRIC_NODE_CONTEXT *STDMETHODCALLTYPE get_NodeContext( void);

        //IFabricNodeContextResult2 methods
        HRESULT STDMETHODCALLTYPE GetDirectory(
            /* [in] */ LPCWSTR logicalDirectoryName,
            /* [out, retval] */ IFabricStringResult **directoryPath);

    private:
        FabricNodeContextResultImplSPtr nodeContextImplSPtr_;
    };
}
