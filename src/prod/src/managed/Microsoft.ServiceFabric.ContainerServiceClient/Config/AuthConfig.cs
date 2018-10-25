// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class AuthConfig
    {
        [DataMember(Name = "username", EmitDefaultValue = false)]
        public string Username { get; set; }

        [DataMember(Name = "password", EmitDefaultValue = false)]
        public string Password { get; set; }

        [DataMember(Name = "auth", EmitDefaultValue = false)]
        public string Auth { get; set; }

        [DataMember(Name = "email", EmitDefaultValue = false)]
        public string Email { get; set; }

        [DataMember(Name = "serveraddress", EmitDefaultValue = false)]
        public string ServerAddress { get; set; }

        [DataMember(Name = "identitytoken", EmitDefaultValue = false)]
        public string IdentityToken { get; set; }

        [DataMember(Name = "registrytoken", EmitDefaultValue = false)]
        public string RegistryToken { get; set; }
    }
}