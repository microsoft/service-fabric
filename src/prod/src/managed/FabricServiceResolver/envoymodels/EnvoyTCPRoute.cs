// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    class EnvoyTCPRoute
    {
        public EnvoyTCPRoute(string cluster)
        {
            this.cluster = cluster;
        }

        [JsonProperty]
        public string cluster;
    }
}
