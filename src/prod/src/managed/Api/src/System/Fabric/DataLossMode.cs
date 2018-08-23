// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// This enum is passed to the DataLoss testability API to indicate what type of data loss to induce.
    /// </summary>
    [Serializable]
    public enum DataLossMode
    {
        /// <summary>
        /// Reserved.  Do not pass into API.
        /// </summary>
        Invalid = NativeTypes.FABRIC_DATA_LOSS_MODE.FABRIC_DATA_LOSS_MODE_INVALID,
        /// <summary>
        /// PartialDataLoss option will cause a quorum of replicas to go down, triggering an OnDataLoss event in the system for the given partition. 
        /// </summary>
        /// <remarks>
        /// Whether actual data loss happens depends on whether there were committed transactions that were still being replicated at the time the data loss was induced
        /// </remarks>
        PartialDataLoss = NativeTypes.FABRIC_DATA_LOSS_MODE.FABRIC_DATA_LOSS_MODE_PARTIAL,
        /// <summary>
        /// FullDataLoss option will drop all the replicas which means that all the data will be lost. 
        /// </summary>
        /// <remarks>
        /// This option is very useful to test out backup and recovery data paths.
        /// </remarks>
        FullDataLoss = NativeTypes.FABRIC_DATA_LOSS_MODE.FABRIC_DATA_LOSS_MODE_FULL
    }
}