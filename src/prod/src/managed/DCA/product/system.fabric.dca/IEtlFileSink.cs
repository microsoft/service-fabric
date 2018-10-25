// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    public interface IEtlFileSink : IEtwSink
    {
        // Method invoked when the producer begins an ETW event processing pass
        void OnEtwEventProcessingPeriodStart();
        
        // Method invoked when the producer finishes an ETW event processing pass
        void OnEtwEventProcessingPeriodStop();
    }
}