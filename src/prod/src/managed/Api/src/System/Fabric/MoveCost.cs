// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes the move cost of a replica.</para>
    /// </summary>
    /// <remarks>
    /// When a new replica or service instance is created it will get a default value for move cost. This value will be selected based on the service:
    /// <list type="bullet">
    ///     <item><description>For replicas or instances of default services default move cost value will be <see cref="System.Fabric.MoveCost.Zero"/>,
    ///     meaning that moving these replicas is free.</description></item>
    ///     <item><description>For replicas or instances of other services default move cost will be <see cref="System.Fabric.MoveCost.Low"/>.</description></item>
    /// </list>
    /// </remarks>
    public enum MoveCost
    {
        /// <summary>
        /// <para>Specifies the move cost of a replica as Zero.</para>
        /// </summary>
        Zero = NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_ZERO,
        /// <summary>
        /// <para>Specifies the move cost of a replica as Low.</para>
        /// </summary>
        Low = NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_LOW,
        /// <summary>
        /// <para>Specifies the move cost of a replica as Medium.</para>
        /// </summary>
        Medium = NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_MEDIUM,
        /// <summary>
        /// <para>Specifies the move cost of a replica as High.</para>
        /// </summary>
        High = NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_HIGH,
    }
}