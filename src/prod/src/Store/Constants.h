// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class Constants
    {
    public:
        // Tracing
        static Common::StringLiteral const StoreSource;
        static Common::GlobalWString LifecycleOpen;
        static Common::GlobalWString LifecycleChangeRole;
        static Common::GlobalWString LifecycleClose;
        static Common::GlobalWString LifecycleCommit;

        // ReplicatedStore
        static Common::StringLiteral const DataSerializerTraceComponent;

        static Common::GlobalWString TableName;
        static Common::GlobalWString ProgressDataType;
        static Common::GlobalWString EpochHistoryDataName;
        static Common::GlobalWString CurrentEpochDataName;
        static Common::GlobalWString TombstoneLowWatermarkDataName;

        static Common::GlobalWString TombstoneDataType;
        static Common::GlobalWString TombstoneKeyDelimiter;
        static Common::GlobalWString TombstoneKeyDoubleDelimiter;
        static Common::GlobalWString TombstoneKeyEscapedDelimiter;

        static Common::GlobalWString FabricTimeDataType;
        static Common::GlobalWString FabricTimeDataKey;

        static Common::GlobalWString KeyValueStoreItemType;

        static Common::GlobalWString LocalStoreIncrementalBackupDataType;
        static Common::GlobalWString AllowIncrementalBackupDataName;

        static Common::GlobalWString PartialCopyProgressDataType;
        static Common::GlobalWString PartialCopyProgressDataName;

        static FABRIC_SEQUENCE_NUMBER StoreMetadataSequenceNumber;

        static int const UseEseDefaultValue;
        static size_t const MaxEseDirectoryLength;
    };
}
