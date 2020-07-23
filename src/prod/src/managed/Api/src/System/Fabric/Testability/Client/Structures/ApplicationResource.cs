// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Testability.Client.Structures
{
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;

    public sealed class ApplicationResource
    {

        public class ApplicationProperties
        {
            public ApplicationProperties()
            {
            }

            public string description { get; set; }
    
            public string healthState { get; set; }

            public List<string> serviceNames { get; set; }

            public string statusDetails { get; set; }

            public string status { get; set; }

            public string unhealthyEvaluation { get; set; }
        }

        public ApplicationResource()
        {
            this.properties = new ApplicationProperties();
        }

        public string name { get; set; }

        [JsonCustomization(IsIgnored = true)]
        public string description { get { return properties.description; } }

        [JsonCustomization(IsIgnored = true)]
        public string healthState { get { return properties.healthState; } }

        [JsonCustomization(IsIgnored = true)]
        public List<string> serviceNames { get { return properties.serviceNames; } }

        [JsonCustomization(IsIgnored = true)]
        public string status { get { return properties.status; }  }

        [JsonCustomization(IsIgnored = true)]
        public string statusDetails { get { return properties.statusDetails; } }

        [JsonCustomization(IsIgnored = true)]
        public string unhealthyEvaluation { get { return properties.unhealthyEvaluation; } }

        public ApplicationProperties properties { get; set; }
    }
}

#pragma warning restore 1591
