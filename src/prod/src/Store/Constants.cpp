// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Store
{
    // Tracing
    Common::StringLiteral const Constants::StoreSource("Store");
    Common::GlobalWString Constants::LifecycleOpen = Common::make_global<std::wstring>(L"LifecycleOpen");
    Common::GlobalWString Constants::LifecycleChangeRole = Common::make_global<std::wstring>(L"LifecycleChangeRole");
    Common::GlobalWString Constants::LifecycleClose = Common::make_global<std::wstring>(L"LifecycleClose");
    Common::GlobalWString Constants::LifecycleCommit = Common::make_global<std::wstring>(L"LifecycleCommit");

    // ReplicatedStore
    Common::StringLiteral const Constants::DataSerializerTraceComponent("DataSerializer");

    Common::GlobalWString Constants::TableName = Common::make_global<std::wstring>(L"LocalStoreData");
    Common::GlobalWString Constants::ProgressDataType = Common::make_global<std::wstring>(L"ProgressData");
    Common::GlobalWString Constants::EpochHistoryDataName = Common::make_global<std::wstring>(L"EpochHistory");
    Common::GlobalWString Constants::CurrentEpochDataName = Common::make_global<std::wstring>(L"CurrentEpoch");
    Common::GlobalWString Constants::TombstoneLowWatermarkDataName = Common::make_global<std::wstring>(L"TombstoneLowWatermark");

    Common::GlobalWString Constants::TombstoneDataType = Common::make_global<std::wstring>(L"ReplicationTombstone");
    Common::GlobalWString Constants::TombstoneKeyDelimiter = Common::make_global<std::wstring>(L"+");
    Common::GlobalWString Constants::TombstoneKeyDoubleDelimiter = Common::make_global<std::wstring>(L"++");
    Common::GlobalWString Constants::TombstoneKeyEscapedDelimiter = Common::make_global<std::wstring>(L"(+)");

    Common::GlobalWString Constants::FabricTimeDataType = Common::make_global<std::wstring>(L"FabricTimeDataType");
    Common::GlobalWString Constants::FabricTimeDataKey = Common::make_global<std::wstring>(L"FabricTimeDataKey");

    Common::GlobalWString Constants::KeyValueStoreItemType = Common::make_global<std::wstring>(L"__default__");

    Common::GlobalWString Constants::LocalStoreIncrementalBackupDataType = Common::make_global<std::wstring>(L"LocalStoreIncrementalBackupDataType");
    Common::GlobalWString Constants::AllowIncrementalBackupDataName = Common::make_global<std::wstring>(L"AllowIncrementalBackup");

    Common::GlobalWString Constants::PartialCopyProgressDataType = Common::make_global<std::wstring>(L"PartialCopyProgressDataType");
    Common::GlobalWString Constants::PartialCopyProgressDataName = Common::make_global<std::wstring>(L"PartialCopyProgressDataKey");

    FABRIC_SEQUENCE_NUMBER Constants::StoreMetadataSequenceNumber = 1;

    int const Constants::UseEseDefaultValue(-1);
    
    // https://msdn.microsoft.com/en-us/library/gg269235(v=exchg.10).aspx (JET_paramSystemPath, JET_paramLogFilePath)
    //
    size_t const Constants::MaxEseDirectoryLength(246);
}
