// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Threading.Tasks;

    internal interface IOperationProcessor
    {
        RecordProcessingMode IdentifyProcessingModeForRecord(LogRecord record);

        Task ImmediatelyProcessRecord(LogRecord record, Exception flushException, RecordProcessingMode processingMode);

        void PrepareToProcessLogicalRecord();

        void PrepareToProcessPhysicalRecord();

        Task ProcessLoggedRecordAsync(LogRecord record);

        void UpdateDispatchingBarrierTask(CompletionTask barrierTask);
    }
}