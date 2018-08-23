// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    /// <summary>
    /// <para>Indicates that this service is associated with another service, and describes the relationship with that service.</para>
    /// </summary>
    /// <remarks>
    ///   <para />
    /// </remarks>
    public enum ServiceCorrelationScheme
    {
        /// <summary>
        /// <para>An invalid correlation scheme. Cannot be used.</para>
        /// </summary>
        Invalid = 0,
        /// <summary>
        /// <para>Indicates that this service has an affinity relationship with another service. Provided for backwards compatibility, consider preferring 
        /// the Aligned or NonAlignedAffinity options.</para>
        /// </summary>
        Affinity = 1,
        /// <summary>
        /// <para>Aligned affinity ensures that the primaries of the partitions of the affinitized services are collocated on the same nodes. This is the 
        /// default and is the same as selecting the “Affinity” scheme.</para>
        /// </summary>
        AlignedAffinity = 2,
        /// <summary>
        /// <para>Non-Aligned affinity guarantees that all replicas of each service will be placed on the same nodes. Unlike Aligned Affinity, this does not 
        /// guarantee that replicas of particular role will be collocated. </para>
        /// </summary>
        NonAlignedAffinity = 3
    }
}