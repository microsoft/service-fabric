// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Response
{
    using System;
    using System.Runtime.Serialization;

    [DataContract]
    internal class ContainerEventAttribute
    {
        [DataMember(Name = "name", EmitDefaultValue = false)]
        public string Name { get; set; }

        [DataMember(Name = "exitCode", EmitDefaultValue = false)]
        public UInt32 ExitCode { get; set; }
    }

    [DataContract]
    internal class ContainerEventActor
    {
        [DataMember(Name = "ID", EmitDefaultValue = false)]
        public string Id { get; set; }

        [DataMember(Name = "Attributes", EmitDefaultValue = false)]
        public ContainerEventAttribute Attributes { get; set; }
    }

    [DataContract]
    internal class ContainerEventResponse
    {
        private readonly string HealthEventString = "health_status";
        private readonly string HealthyHealthStatusString = "health_status: healthy";

        [DataMember(Name = "status", EmitDefaultValue = false)]
        public string Status { get; set; }

        [DataMember(Name = "id", EmitDefaultValue = false)]
        public string Id { get; set; }

        [DataMember(Name = "from", EmitDefaultValue = false)]
        public string From { get; set; }

        [DataMember(Name = "Type", EmitDefaultValue = false)]
        public string Type { get; set; }

        [DataMember(Name = "Action", EmitDefaultValue = false)]
        public string Action { get; set; }

        [DataMember(Name = "Actor", EmitDefaultValue = false)]
        public ContainerEventActor Actor { get; set; }

        [DataMember(Name = "time", EmitDefaultValue = false)]
        public UInt64 Time { get; set; }

        [DataMember(Name = "timeNano", EmitDefaultValue = false)]
        public UInt64 TimeNano { get; set; }

        public bool IsHealthEvent
        {
            get
            {
                return this.Action.StartsWith(HealthEventString, StringComparison.OrdinalIgnoreCase);
            }
        }

        public bool IsHealthStatusHealthy
        {
            get
            {
                return this.Action.Equals(HealthyHealthStatusString, StringComparison.OrdinalIgnoreCase);
            }
        }
    }
}