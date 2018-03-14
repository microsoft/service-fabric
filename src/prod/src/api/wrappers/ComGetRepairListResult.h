// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {348470c2-b4e1-4cd8-a310-bc8e1fc7431d}
    static const GUID CLSID_ComGetRepairTaskListResult =
    { 0x348470c2, 0xb4e1, 0x4cd8, {0xa3, 0x10, 0xbc, 0x8e, 0x1f, 0xc7, 0x43, 0x1d} };
    
    class ComGetRepairTaskListResult :
        public IFabricGetRepairTaskListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetRepairTaskListResult)

        COM_INTERFACE_LIST2(
            ComGetRepairTaskListResult,
            IID_IFabricGetRepairTaskListResult,
            IFabricGetRepairTaskListResult,
            CLSID_ComGetRepairTaskListResult,
            ComGetRepairTaskListResult)

    public:
        explicit ComGetRepairTaskListResult(std::vector<Management::RepairManager::RepairTask> && repairList);

        // 
        // IFabricGetRepairTaskListResult methods
        // 
        const FABRIC_REPAIR_TASK_LIST * STDMETHODCALLTYPE get_Tasks();

    private:
        Common::ReferencePointer<FABRIC_REPAIR_TASK_LIST> repairList_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetRepairTaskListResult> ComGetRepairTaskListResultCPtr;
}
