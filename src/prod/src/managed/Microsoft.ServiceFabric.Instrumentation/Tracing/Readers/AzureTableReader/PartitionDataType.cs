// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    public enum PartitionDataType
    {
        // It implies that the partition key is directly specified.
        StaticData,

        // It implies that partition key is stored in one of the property
        PropertyName
    }
}