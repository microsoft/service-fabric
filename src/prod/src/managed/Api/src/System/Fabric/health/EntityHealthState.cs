// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    /// <summary>
    /// <para>Represents the base class for all entity health state classes.</para>
    /// </summary>
    /// <remarks>An entity health state contains entity identifier and entity aggregated health state.</remarks>
    public abstract class EntityHealthState
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.EntityHealthState" /> class.</para>
        /// </summary>
        protected EntityHealthState()
        {
        }

        /// <summary>
        /// <para>Gets the aggregated health state computed by Health Manager 
        /// based on all reported events and the desired policy.</para>
        /// </summary>
        /// <value>
        /// <para>The aggregated health state computed by Health Manager.</para>
        /// </value>
        public HealthState AggregatedHealthState
        {
            get;
            internal set;
        }
    }
}