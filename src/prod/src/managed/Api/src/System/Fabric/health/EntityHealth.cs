// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;

    /// <summary>
    /// <para>Represents the base class for all entity health related classes.</para>
    /// </summary>
    public abstract class EntityHealth
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.EntityHealth" /> class.</para>
        /// </summary>
        protected EntityHealth()
        {
        }

        /// <summary>
        /// <para>Gets the entity aggregated health state.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.HealthState" /> representing the aggregated health state 
        /// computed by Health Manager based on 
        /// reported events on entity and its children (if any) and desired health policy.</para>
        /// </value>
        public HealthState AggregatedHealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the health events reported on the entity.</para>
        /// </summary>
        /// <value>
        /// <para>The health events reported on the entity.</para>
        /// </value>
        /// <remarks>The health events are created based on health reports sent to health store. 
        /// They contain additional metadata added by health store.</remarks>
        public IList<HealthEvent> HealthEvents
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the unhealthy evaluations that show why the current aggregated health state
        /// was returned by Health Manager.</para>
        /// </summary>
        /// <value>
        /// <para>The unhealthy evaluations that describe why the current aggregated health state was returned by Health Manager.</para>
        /// </value>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }
    }
}