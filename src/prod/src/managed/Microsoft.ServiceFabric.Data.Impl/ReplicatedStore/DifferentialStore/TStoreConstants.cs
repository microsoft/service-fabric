// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Threading;

    internal static class TStoreConstants
    {
        /// <summary>
        /// Version of on-disk data structures.
        /// </summary>
        internal const int SerializedVersion = 1;

        /// <summary>
        /// Wait time for copy to wait on the checkpoint file lock before it replaces the checkpoint file.
        /// </summary>
        internal const int CopyWaitTimetoAcquireLockOnCheckpointFileInMilliseconds = Timeout.Infinite;

        /// <summary>
        /// Timeout(ms) used for lock acquistion on checkpoint and copy to avoid indefinite checkpoint and copy delays during ClearAsync/PrepareForRemoveStateAsync operations progress.
        /// </summary>
        internal const int CheckpointOrCopyLockTimeoutInMilliseconds = 256 * 1000;

        /// <summary>
        /// Default value of the versionsequencenumber
        /// </summary>
        internal const long UninitializedVersionSequenceNumber = 0;

        /// <summary>
        /// Initial size of the Recovery Component.
        /// Default is set as 8192 as it is a largest order of two number that does not force the intenral List._items to be in LOH.
        /// This default optimizes for TStores that have at least 4097 items.
        /// </summary>
        internal const int InitialRecoveryComponentSize = 1024 << 3;

        /// <summary>
        /// Default number of delta components that can exist before checkpoint decides to consolidate.
        /// </summary>
        internal const int DefaultNumberOfDeltasTobeConsolidated = 3;

        /// <summary>
        /// Default background sweep intreval(5 minutes).
        /// </summary>
        internal const int DefaultBackgroundSweepIntervalInMilliSeconds = 300000;

        /// <summary>
        /// Default number of items loaded into memory before sweep is triggered.
        /// </summary>
        internal const int DefaultSweepThreshold = 500000;


        /// <summary>
        /// Sweep threshold disabled flag.
        /// </summary>
        internal const int SweepThresholdDisabledFlag = -1;

        /// <summary>
        /// Sweep threshold set to default flag
        /// </summary>
        internal const int SweepThresholdDefaultFlag = 0;

        /// <summary>
        /// Sweep threshold set to default flag : 5 million. (inclusive)
        /// </summary>
        internal const int MaxSweepThreshold = 5000000;

        /// <summary>
        /// Timeout for prime lock acquistion on close/abort.
        /// </summary>
        internal const int TimeoutForClosePrimeLockInMilliseconds = 10000;
    }
}