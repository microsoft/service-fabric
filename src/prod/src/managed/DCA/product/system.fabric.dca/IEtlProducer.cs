// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    // Interface exposed by the ETL producer to internal consumers
    internal interface IEtlProducer
    {
        // Informs the caller whether or not the ETL files that the producer is 
        // processing are Windows Fabric ETL files
        bool IsProcessingWindowsFabricEtlFiles();
        
        // Informs the caller whether or not a particular ETL file has been fully processed
        bool HasEtlFileBeenFullyProcessed(string fileName);
    }
}