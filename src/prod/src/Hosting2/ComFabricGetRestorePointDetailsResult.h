// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class ComFabricGetRestorePointDetailsResult :
        public IFabricGetRestorePointDetailsResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricGetRestorePointDetailsResult)

            BEGIN_COM_INTERFACE_LIST(ComFabricGetRestorePointDetailsResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGetRestorePointDetailsResult)
            COM_INTERFACE_ITEM(IID_IFabricGetRestorePointDetailsResult, IFabricGetRestorePointDetailsResult)
            END_COM_INTERFACE_LIST()

    public:
        ComFabricGetRestorePointDetailsResult(FabricGetRestorePointDetailsResultImplSPtr impl);
        virtual ~ComFabricGetRestorePointDetailsResult();

        // 
        // IFabricGetRestorePointDetailsResult methods
        // 
        const FABRIC_RESTORE_POINT_DETAILS *STDMETHODCALLTYPE get_RestorePointDetails(void);

    private:
        FabricGetRestorePointDetailsResultImplSPtr restorePointDetailsResultImplSPtr_;
    };
}
