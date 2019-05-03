// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class Constants
        {
        public:

            static Common::GlobalWString CentralSecretServicePrimaryCountName;
            static Common::GlobalWString CentralSecretServiceReplicaCountName;
            static Common::GlobalWString DatabaseDirectory;
            static Common::GlobalWString DatabaseFilename;
            static Common::GlobalWString SharedLogFilename;
            
            class StoreType {
            public:
                static Common::GlobalWString Secrets;
                static Common::GlobalWString SecretsMetadata;
            };
        };
    }
}
