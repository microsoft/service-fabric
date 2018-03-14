// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::RepairManager;

namespace Api
{
    ComGetRepairTaskListResult::ComGetRepairTaskListResult(
        vector<RepairTask> && repairList)
        : repairList_(),
        heap_()
    {
        repairList_ = heap_.AddItem<FABRIC_REPAIR_TASK_LIST>();
        ComConversionUtility::ToPublicList<
            RepairTask,
            FABRIC_REPAIR_TASK,
            FABRIC_REPAIR_TASK_LIST>(
                heap_, 
                move(repairList), 
                *repairList_);
    }

    const FABRIC_REPAIR_TASK_LIST * STDMETHODCALLTYPE ComGetRepairTaskListResult::get_Tasks()
    {
        return repairList_.GetRawPointer();
    }
}
