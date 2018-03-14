// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Constants.h"

namespace Reliability 
{
    namespace ReplicationComponent
    {
        using std::wstring;

        FABRIC_SEQUENCE_NUMBER const Constants::InvalidLSN = 0;

        FABRIC_SEQUENCE_NUMBER const Constants::MaxLSN = 0x7FFFFFFFFFFFFFFF;
        FABRIC_SEQUENCE_NUMBER const Constants::NonInitializedLSN = -1;
        FABRIC_EPOCH const Constants::InvalidEpoch = {Constants::InvalidLSN, Constants::InvalidLSN, NULL};

        uint64 const Constants::InvalidRequestId = 0;
        uint64 const Constants::InvalidSessionId = 0;

        // Configuration
        Common::StringLiteral const Constants::REConfigSource("REConfig");
        Common::StringLiteral const Constants::ReplicatorSource("Replicator");

        Common::GlobalWString Constants::ReplOperationTrace = Common::make_global<std::wstring>(L"REPL");
        Common::GlobalWString Constants::CopyOperationTrace = Common::make_global<std::wstring>(L"COPY");
        Common::GlobalWString Constants::EnqueueOperationTrace = Common::make_global<std::wstring>(L"Enqueue");
        Common::GlobalWString Constants::AckOperationTrace = Common::make_global<std::wstring>(L"Ack");
        Common::GlobalWString Constants::CopyContextOperationTrace = Common::make_global<std::wstring>(L"CopyCONTEXT");
        Common::GlobalWString Constants::CopyContextAckOperationTrace = Common::make_global<std::wstring>(L"CopyContextACK");
        Common::GlobalWString Constants::StartCopyOperationTrace = Common::make_global<std::wstring>(L"StartCOPY");
        Common::GlobalWString Constants::RequestAckTrace = Common::make_global<std::wstring>(L"RequestACK");
        Common::GlobalWString Constants::UpdateEpochDrainQueue = Common::make_global<std::wstring>(L"UpdateEpoch");
        Common::GlobalWString Constants::CloseDrainQueue = Common::make_global<std::wstring>(L"Close");
        Common::GlobalWString Constants::PromoteIdleDrainQueue = Common::make_global<std::wstring>(L"PromoteIdle");

        wchar_t const Constants::ReplicationEndpointDelimiter(L';');
        wchar_t const Constants::PartitionReplicaDelimiter(L'-');
        wchar_t const Constants::ReplicaIncarnationDelimiter(L';');
    } 
}
