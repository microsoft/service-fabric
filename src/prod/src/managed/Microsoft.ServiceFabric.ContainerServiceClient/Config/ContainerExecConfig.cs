// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class ContainerExecConfig
    {
        [DataMember(Name = "User", EmitDefaultValue = false)]
        public string User { get; set; }

        [DataMember(Name = "Privileged", EmitDefaultValue = false)]
        public bool Privileged { get; set; }

        [DataMember(Name = "Tty", EmitDefaultValue = false)]
        public bool Tty { get; set; }

        [DataMember(Name = "AttachStdin", EmitDefaultValue = false)]
        public bool AttachStdin { get; set; }

        [DataMember(Name = "AttachStderr", EmitDefaultValue = false)]
        public bool AttachStderr { get; set; }

        [DataMember(Name = "AttachStdout", EmitDefaultValue = false)]
        public bool AttachStdout { get; set; }

        [DataMember(Name = "Detach", EmitDefaultValue = false)]
        public bool Detach { get; set; }

        [DataMember(Name = "DetachKeys", EmitDefaultValue = false)]
        public string DetachKeys { get; set; }

        [DataMember(Name = "Env", EmitDefaultValue = false)]
        public IList<string> Env { get; set; }

        [DataMember(Name = "Cmd", EmitDefaultValue = false)]
        public IList<string> Cmd { get; set; }
    }
}