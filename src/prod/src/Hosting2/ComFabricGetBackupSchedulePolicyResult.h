// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class ComFabricGetBackupSchedulePolicyResult :
        public IFabricGetBackupSchedulePolicyResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricGetBackupSchedulePolicyResult)

            BEGIN_COM_INTERFACE_LIST(ComFabricGetBackupSchedulePolicyResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGetBackupSchedulePolicyResult)
            COM_INTERFACE_ITEM(IID_IFabricGetBackupSchedulePolicyResult, IFabricGetBackupSchedulePolicyResult)
            END_COM_INTERFACE_LIST()

    public:
        ComFabricGetBackupSchedulePolicyResult(FabricGetBackupSchedulePolicyResultImplSPtr impl);
        virtual ~ComFabricGetBackupSchedulePolicyResult();

        // 
        // IFabricGetBackupSchedulePolicyResult methods
        // 
        const FABRIC_BACKUP_POLICY *STDMETHODCALLTYPE get_BackupSchedulePolicy(void);

    private:
        FabricGetBackupSchedulePolicyResultImplSPtr policyResultImplSPtr_;
    };
}
