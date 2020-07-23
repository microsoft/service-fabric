// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;

namespace sfintegration.envoymodels
{

    public class EnvoyHostTags
    {
        [JsonProperty]
        public string az = "";

        [JsonProperty]
        public bool canary = false;

        [JsonProperty]
        public int load_balancing_weight = 100;

        public static EnvoyHostTags defaultValue = new EnvoyHostTags();
    }


}
