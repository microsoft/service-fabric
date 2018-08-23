// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System;
    using System.IO;

    /// <summary>
    /// Constants for TStore Table format.
    /// </summary>
    internal static class DiskConstants
    {
        /// <summary>
        /// Constants for ReplicatedStore.
        /// </summary>
        public const string state_opening = "opening";
        public const string state_complete = "complete";
    }
}