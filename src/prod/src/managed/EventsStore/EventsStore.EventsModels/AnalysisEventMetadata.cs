// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels
{
    using System;
    using Newtonsoft.Json;

    [JsonObject("AnalysisEventMetadata")]
    public class AnalysisEventMetadata
    {
        /// <summary>
        /// Initializes a new instance of the AnalysisEventMetadata class.
        /// </summary>
        public AnalysisEventMetadata()
        {
        }

        /// <summary>
        /// Initializes a new instance of the AnalysisEventMetadata class.
        /// </summary>
        /// <param name="delay">The analysis delay.</param>
        /// <param name="duration">The duration of analysis.</param>
        public AnalysisEventMetadata(TimeSpan? delay = default(TimeSpan?), TimeSpan? duration = default(TimeSpan?))
        {
            Delay = delay;
            Duration = duration;
        }

        /// <summary>
        /// Gets or sets the analysis delay.
        /// </summary>
        [JsonProperty(PropertyName = "Delay")]
        public TimeSpan? Delay { get; set; }

        /// <summary>
        /// Gets or sets the duration of analysis.
        /// </summary>
        [JsonProperty(PropertyName = "Duration")]
        public TimeSpan? Duration { get; set; }
    }
}