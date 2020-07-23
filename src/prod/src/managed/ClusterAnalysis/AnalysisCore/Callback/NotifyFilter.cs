// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Callback
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;

    /// <summary>
    /// Describe the filter for notify
    /// </summary>
    [DataContract]
    public class NotifyFilter
    {
        /// <summary>
        /// Create an instance of <see cref="NotifyFilter"/>
        /// </summary>
        /// <param name="targetScenario"></param>
        /// <param name="signalAvailableForAtLeastDuration"></param>
        public NotifyFilter(Scenario targetScenario, TimeSpan signalAvailableForAtLeastDuration)
        {
            this.TargetScenario = targetScenario;
            this.SignalAvailableForAtLeastDuration = signalAvailableForAtLeastDuration;
        }

        /// <summary>
        /// Scenario for this filter
        /// </summary>
        [DataMember(IsRequired = true)]
        public Scenario TargetScenario { get; private set; }

        /// <summary>
        /// How long should a signal be available for before we report it.
        /// </summary>
        [DataMember(IsRequired = true)]
        public TimeSpan SignalAvailableForAtLeastDuration { get; private set; }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Scenario: '{0}', SignalAvailableForAtLeastDuration: '{1}'",
                this.TargetScenario,
                this.SignalAvailableForAtLeastDuration);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.TargetScenario.ToString().GetHashCode();
                hash = (hash * 23) + this.SignalAvailableForAtLeastDuration.GetHashCode();
                return hash;
            }
        }

        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }

            var other = obj as NotifyFilter;
            if (other == null)
            {
                return false;
            }

            return this.TargetScenario == other.TargetScenario && this.SignalAvailableForAtLeastDuration == other.SignalAvailableForAtLeastDuration;
        }
    }
}