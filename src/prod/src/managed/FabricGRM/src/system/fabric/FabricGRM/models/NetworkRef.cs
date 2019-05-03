// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using System.Collections.Generic;
    using Newtonsoft.Json;

    public partial class NetworkRef
    {
        public NetworkRef()
        {
        }

        public NetworkRef(string name)
        {
            this.Name = name;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        public static bool operator ==(NetworkRef ref1, NetworkRef ref2)
        {
            return EqualityComparer<NetworkRef>.Default.Equals(ref1, ref2);
        }

        public static bool operator !=(NetworkRef ref1, NetworkRef ref2)
        {
            return !(ref1 == ref2);
        }

        public override bool Equals(object obj)
        {
            var @ref = obj as NetworkRef;
            return @ref != null &&
                   this.Name == @ref.Name;
        }

        public override int GetHashCode()
        {
            return 539060726 + EqualityComparer<string>.Default.GetHashCode(this.Name);
        }
    }
}
