// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.Common.Exceptions
{
    using System;

    internal sealed class ChaosInconsistentClusterSnapshotException : Exception
    {
        public ChaosInconsistentClusterSnapshotException()
        {
        }

        public ChaosInconsistentClusterSnapshotException(string message)
        : base(message)
        {
        }

        public ChaosInconsistentClusterSnapshotException(string message, Exception inner)
        : base(message, inner)
        {
        }
    }
}