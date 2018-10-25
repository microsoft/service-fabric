// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class Constants
        {
        public:
            static Common::GlobalWString FileStoreServiceUserGroup;
            static Common::GlobalWString PrimaryFileStoreServiceUser;
            static Common::GlobalWString SecondaryFileStoreServiceUser;

            static Common::GlobalWString FileStoreServicePrimaryCountName;
            static Common::GlobalWString FileStoreServiceReplicaCountName;

            static Common::GlobalWString StoreRootDirectoryName;
            static Common::GlobalWString StagingRootDirectoryName;
            static Common::GlobalWString StoreShareName;
            static Common::GlobalWString StagingShareName;

            static Common::GlobalWString DatabaseDirectory;
            static Common::GlobalWString DatabaseFilename;
            static Common::GlobalWString SharedLogFilename;
            static Common::GlobalWString SMBShareRemark;

            static Common::GlobalWString LanmanServerParametersRegistryPath;
            static Common::GlobalWString NullSessionSharesRegistryValue;

            static Common::GlobalWString LsaRegistryPath;
            static Common::GlobalWString EveryoneIncludesAnonymousRegistryValue;

            static double const StagingFileLockAcquireTimeoutInSeconds;

            class StoreType
            {
            public:
                static Common::GlobalWString FileMetadata;
                static Common::GlobalWString LastDeletedVersionNumber;
            };

            class Actions
            {
            public:
                static Common::GlobalWString OperationSuccess;
            };

            class ErrorReason
            {
            public:
                static Common::GlobalWString MissingActivityHeader;
                static Common::GlobalWString MissingTimeoutHeader;
                static Common::GlobalWString IncorrectActor;
                static Common::GlobalWString InvalidAction;
            };
        };
    }
}
