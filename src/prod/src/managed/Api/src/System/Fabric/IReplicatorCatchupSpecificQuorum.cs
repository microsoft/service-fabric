// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// <para>Allows an IReplicator to indicate that it supports catching up specific quorums with the use of the MustCatchup flag in ReplicaInformation.</para>
    /// </summary>
    public interface IReplicatorCatchupSpecificQuorum
    {
    }
}