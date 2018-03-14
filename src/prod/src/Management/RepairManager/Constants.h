// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class Constants
        {
        public:

            static Common::GlobalWString RepairManagerServicePrimaryCountName;
            static Common::GlobalWString RepairManagerServiceReplicaCountName;

            static Common::GlobalWString DatabaseDirectory;
            static Common::GlobalWString DatabaseFilename;
            static Common::GlobalWString SharedLogFilename;

            // -------------------------------
            // Store Data Keys
            // -------------------------------

            static Common::GlobalWString StoreType_RepairTaskContext;
            static Common::GlobalWString StoreType_HealthCheckStoreData;
            static Common::GlobalWString StoreKey_HealthCheckStoreData;
        };
    }
}
