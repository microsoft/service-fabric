// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using Runtime.InteropServices;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>
    /// Represents health state count for entities of the specified entity kind.
    /// It is returned as part of health statistics when requested by the health queries.
    /// </para>
    /// </summary>
    public sealed class EntityKindHealthStateCount
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.EntityKindHealthStateCount" /> class.</para>
        /// </summary>
        internal EntityKindHealthStateCount()
        {
        }

        /// <summary>
        /// Gets the entity kind. The <see cref="System.Fabric.Health.HealthStateCount"/> shows how many entities of this kind are in 
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// </summary>
        /// <value>The entity kind.</value>
        public EntityKind EntityKind
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the health state count for the entities of the specified kind.
        /// </summary>
        /// <value>The health state count for the entities of the specified kind.</value>
        public HealthStateCount HealthStateCount
        {
            get;
            internal set;
        }

        /// <summary>
        /// Returns a string representation of the health state count.
        /// </summary>
        /// <returns>A string representation of the health state count.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "{0}: {1}", this.EntityKind, this.HealthStateCount);
        }

        internal static unsafe EntityKindHealthStateCount FromNative(NativeTypes.FABRIC_ENTITY_KIND_HEALTH_STATE_COUNT nativeEntityKindHealthStateCount)
        {
            var entityKindHealthStateCount = new EntityKindHealthStateCount();
            entityKindHealthStateCount.EntityKind = (EntityKind)nativeEntityKindHealthStateCount.EntityKind;
            entityKindHealthStateCount.HealthStateCount = HealthStateCount.FromNative(nativeEntityKindHealthStateCount.HealthStateCount);

            return entityKindHealthStateCount;
        }
    }
}
