// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System.Collections.Generic;

    /// <summary>
    /// Note: existing events can not have this added with a newer version if EventSource versioning needs to be maintained for consumers
    /// as it'll break the back compatibility, which works only if fields are appended at the end and kept at the same positions in future.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method)]
    internal class EventExtendedMetadataAttribute : Attribute
    {
        public static readonly IList<MetadataField> MetadataFields = new List<MetadataField>() {
            new MetadataField("eventName", typeof(string), "EventName: {0}"),
            new MetadataField("category", typeof(string), "Category: {1}") };

        /// <summary>
        /// 
        /// </summary>
        /// <param name="table"></param>
        /// <param name="publicEventName"></param>
        /// <param name="category"></param>
        public EventExtendedMetadataAttribute(TableEntityKind table, string publicEventName, string category)
        {
            this.TableEntityKind = table;

            if (string.IsNullOrEmpty(publicEventName))
            {
                throw new ArgumentNullException("publicEventName");
            }

            if (string.IsNullOrEmpty(category))
            {
                throw new ArgumentNullException("category");
            }

            this.PublicEventName = publicEventName;
            this.Category = category;
        }

        /// <summary>
        /// Entity kind (Cluster, Applications, etc.) corresponding to an Azure table for Operational events
        /// </summary>
        public TableEntityKind TableEntityKind { get; set; }

        /// <summary>
        /// Defines a public name of event, to be appended at top of instance payload's fields as 'eventName'.
        /// </summary>
        public string PublicEventName { get; set; }

        /// <summary>
        /// Defines the category that event corresponds to, to be appended at top of instance payload's fields as 'category'.
        /// </summary>
        public string Category { get; set; }

        internal class MetadataField
        {
            public readonly string Name;
            public readonly Type Type;
            public readonly string FormatString;

            public MetadataField(string name, Type type, string formatString)
            {
                this.Name = name;
                this.Type = type;
                this.FormatString = formatString;
            }
        }
    }

    /// <summary>
    /// Includes the common metadata constants for operational events.
    /// </summary>
    internal class EventConstants
    {
        public const string AnalysisCategory = "Analysis";
        public const string ChaosCategory = "Chaos";
        public const string CorrelationCategory = "Correlation"; 
    }
}