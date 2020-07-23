// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using Newtonsoft.Json;

    public class GatewayResourceDescription
    {
        public GatewayResourceDescription()
        {
        }
        
        public GatewayResourceDescription(string name, GatewayResourceProperties properties)
        {
            this.Name = name;
            this.Properties = properties;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "properties")]
        public GatewayResourceProperties Properties { get; set; }
    }
}
