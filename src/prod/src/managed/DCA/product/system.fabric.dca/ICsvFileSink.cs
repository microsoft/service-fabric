// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    internal interface ICsvFileSink : ICsvSink
    {
        // Method invoked when the producer begins an ETW event processing pass
        void OnCsvEventProcessingPeriodStart();
        
        // Method invoked when the producer finishes an ETW event processing pass
        void OnCsvEventProcessingPeriodStop();
    }
}