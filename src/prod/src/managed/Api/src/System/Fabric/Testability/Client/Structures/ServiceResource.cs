// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Testability.Client.Structures
{
    using System.Collections.Generic;

    public sealed class ServiceResource
    {
        public ServiceResource()
        {
        }

        public string name { get; set; }

        public ServiceProperties properties { get; set; }

        public class ServiceProperties
        {
            public string description { get; set; }

            public string healthState { get; set; }

            public string status { get; set; }

            public int replicaCount { get; set; }
        }
    }
}

#pragma warning restore 1591