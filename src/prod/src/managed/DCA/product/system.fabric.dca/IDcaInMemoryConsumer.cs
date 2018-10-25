// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Fabric.Dca.EtlConsumerHelper;

    public interface IDcaInMemoryConsumer : IDcaConsumer
    {
        // Consumer method to be invoked when trace event is available
        Action<DecodedEventWrapper, string> ConsumerProcessTraceEventAction { get; }

        // Last event index processed by the consumer
        Func<string, EventIndex> MaxIndexAlreadyProcessed { get; }

        // Method invoked when consumer processing is about to start
        void OnProcessingPeriodStart(string traceFileName, bool isActiveEtl, string traceFileSubFolder);

        // Method invoked when consumer processing is about to stop
        void OnProcessingPeriodStop(string traceFileName, bool isActiveEtl, string traceFileSubFolder);
    }
}