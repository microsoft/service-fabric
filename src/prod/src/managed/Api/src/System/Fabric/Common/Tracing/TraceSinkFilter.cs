// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common;

    internal class TraceSinkFilter
    {
        private const bool DefaultProvisionalFeatureState = true;

        private readonly TraceSinkType sinkType;
        private readonly List<TraceFilterDescription> filters;
        private EventLevel defaultLevel;
        private int defaultSamplingRatio;
        private bool isProvisionalFeatureEnabledForCurrentSinkType;

        public TraceSinkFilter(TraceSinkType sinkType, EventLevel defaultLevel)
        {
            this.sinkType = sinkType;
            this.defaultLevel = defaultLevel;
            this.defaultSamplingRatio = 0;
            this.filters = new List<TraceFilterDescription>();
            this.isProvisionalFeatureEnabledForCurrentSinkType = DefaultProvisionalFeatureState;
        }

        public TraceSinkType SinkType
        {
            get
            {
                return this.sinkType;
            }
        }

        /// <summary>
        /// Gets or sets if Provisional writes are enabled for this sink type.
        /// </summary>
        public bool ProvisionalFeatureStatus
        {
            get
            {
                return this.isProvisionalFeatureEnabledForCurrentSinkType;
            }
            set
            {
                this.isProvisionalFeatureEnabledForCurrentSinkType = value;
            }
        }

        public EventLevel DefaultLevel
        {
            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
            get
            {
                return this.defaultLevel;
            }

            set
            {
                this.defaultLevel = value;
            }
        }

        public int DefaultSamplingRatio
        {
            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
            get
            {
                return this.defaultSamplingRatio;
            }

            [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
            set
            {
                this.defaultSamplingRatio = value;
            }
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public void AddFilter(EventTask taskId, string eventName, EventLevel level, int samplingRatio)
        {
            TraceFilterDescription filter = new TraceFilterDescription(taskId, eventName, level, samplingRatio);

            for (int i = 0; i < this.filters.Count; i++)
            {
                if (this.filters[i].Matches(taskId, eventName))
                {
                    this.filters[i] = filter;
                    return;
                }
            }

            this.filters.Add(filter);
        }

        public void RemoveFilter(EventTask taskId, string eventName)
        {
            for (int i = 0; i < this.filters.Count; i++)
            {
                if (this.filters[i].Matches(taskId, eventName))
                {
                    this.filters.RemoveAt(i);
                    return;
                }
            }
        }

        public void ClearFilters()
        {
            this.filters.Clear();
        }

        public bool StaticCheck(EventLevel level, EventTask taskId, string eventName, out int samplingRatio)
        {
            int result = -1;
            bool enable = (level <= this.defaultLevel);
            samplingRatio = this.defaultSamplingRatio;

            foreach (TraceFilterDescription filter in this.filters)
            {
                int rank = filter.StaticCheck(taskId, eventName);
                if (rank > result)
                {
                    enable = (level <= filter.Level);
                    samplingRatio = filter.SamplingRatio;
                    result = rank;
                }
            }

            return enable;
        }
    }
}