// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    //
    // Test faulty state provider APIs
    //
    namespace FaultStateProviderType
    {
        enum FaultAPI
        {
            Initialize =                0,
            BeginOpenAsync =            1,
            EndOpenAsync =              2,
            ChangeRoleAsync =           3,
            BeginCloseAsync =           4,
            EndCloseAsync =             5,
            PrepareCheckpoint =         6,
            PerformCheckpointAsync =    7,
            CompleteCheckpointAsync =   8,
            RecoverCheckpointAsync =    9,
            BackupCheckpointAsync =     10,
            RestoreCheckpointAsync =    11,
            RemoveStateAsync =          12,
            GetCurrentState =           13,
            BeginSettingCurrentState =  14,
            SetCurrentStateAsync =      15,
            EndSettingCurrentStateAsync = 16,
            PrepareForRemoveAsync =     17,
            Constructor =               18,
            None =                      19
        };
    }
}
