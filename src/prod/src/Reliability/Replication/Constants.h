// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class Constants
        {
        public:
            // Trace Sources
            static Common::StringLiteral const REConfigSource;
            static Common::StringLiteral const ReplicatorSource;

            static Common::GlobalWString EnqueueOperationTrace;
            static Common::GlobalWString AckOperationTrace;
            static Common::GlobalWString ReplOperationTrace;
            static Common::GlobalWString CopyOperationTrace;
            static Common::GlobalWString CopyContextOperationTrace;
            static Common::GlobalWString CopyContextAckOperationTrace;
            static Common::GlobalWString StartCopyOperationTrace;
            static Common::GlobalWString RequestAckTrace;
            static Common::GlobalWString UpdateEpochDrainQueue;
            static Common::GlobalWString CloseDrainQueue;
            static Common::GlobalWString PromoteIdleDrainQueue;

            static FABRIC_SEQUENCE_NUMBER const InvalidLSN;
            static FABRIC_SEQUENCE_NUMBER const MaxLSN;
            static FABRIC_SEQUENCE_NUMBER const NonInitializedLSN;
            static FABRIC_EPOCH const InvalidEpoch;

            static uint64 const InvalidRequestId;
            static uint64 const InvalidSessionId;

            static Common::GlobalWString HostName;

            static wchar_t const ReplicationEndpointDelimiter;
            static wchar_t const PartitionReplicaDelimiter;
            static wchar_t const ReplicaIncarnationDelimiter;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
