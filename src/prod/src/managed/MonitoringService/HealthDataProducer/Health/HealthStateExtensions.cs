// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Health;
    using System.Linq;

    internal static class HealthStateExtensions
    {
        /// <summary>
        /// Checks if the healthState is warning or error state.
        /// </summary>
        /// <returns>Boolean result.</returns>
        internal static bool IsWarningOrError(this HealthState state)
        {
            return state == HealthState.Error || state == HealthState.Warning;
        }

        /// <summary>
        /// Invokes the Action<EntityHealthEvent> on each element in IEnumerable<EntityHealthEvent> sequence.
        /// </summary>
        /// <param name="healthEvents">Sequence of </param>
        /// <param name="action"></param>
        internal static void ForEachHealthEvent(this IEnumerable<EntityHealthEvent> healthEvents, Action<EntityHealthEvent> action)
        {
            if (healthEvents == null || action == null)
            {
                return;
            }

            healthEvents.ToList().ForEach(healthEvent => action(healthEvent));
        }
    }
}