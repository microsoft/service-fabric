// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    internal static class Constants
    {
        public const string EventStoreConnection = "EventStoreConnection";

        #region System Service Constants

        public static readonly string SystemApplicationName = "fabric:/System";
        public static readonly Guid FmPartitionId = new Guid("00000000-0000-0000-0000-000000000001");

        #endregion
    }
}

#pragma warning restore 1591