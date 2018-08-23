// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// The copy stage.
    /// </summary>
    internal enum CopyStage
    {
        /// <summary>
        /// The invalid.
        /// </summary>
        Invalid = 0, 

        /// <summary>
        /// The copy metadata.
        /// </summary>
        CopyMetadata = 1, 

        /// <summary>
        /// The copy none.
        /// </summary>
        CopyNone = 2, 

        /// <summary>
        /// The copy state.
        /// </summary>
        CopyState = 3, 

        /// <summary>
        /// The copy progress vector.
        /// </summary>
        CopyProgressVector = 4, 

        /// <summary>
        /// The copy false progress.
        /// </summary>
        CopyFalseProgress = 5, 

        /// <summary>
        /// The copy scan to starting lsn.
        /// </summary>
        CopyScanToStartingLsn = 6, 

        /// <summary>
        /// The copy log.
        /// </summary>
        CopyLog = 7, 

        /// <summary>
        /// The copy done.
        /// </summary>
        CopyDone = 8, 
    }
}