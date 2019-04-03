// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    /// <summary>
    /// This enum is passed to the DataLoss testability API to indicate what type of data loss to induce.
    /// </summary>
    public enum DataLossMode
    {
        /// <summary>
        /// PartialDataLoss option will cause a quorum of replicas to go down, triggering an OnDataLoss event in the system for the given partition. 
        /// </summary>
        /// <remarks>
        /// Whether actual data loss happens depends on whether there were committed transactions that were still being replicated at the time the data loss was induced
        /// </remarks>
        PartialDataLoss,
        /// <summary>
        /// FullDataLoss option will drop all the replicas which means that all the data will be lost. 
        /// </summary>
        /// <remarks>
        /// This option is very useful to test out backup and recovery data paths.
        /// </remarks>
        FullDataLoss
    }
}

#pragma warning restore 1591