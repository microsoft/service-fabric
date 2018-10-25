// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// Copy flags
    /// </summary>
    internal enum CopyFlag
    {
        /// <summary>
        /// Copy if the folder is different.
        /// </summary>
        CopyIfDifferent,

        /// <summary>
        /// mirror(atomic) copy.
        /// </summary>
        AtomicCopy, 

        /// <summary>
        /// mirror(atomic) copy but skip if destination already exists.
        /// </summary>
        AtomicCopySkipIfExists 
    }
}