// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    public class EnvoyRetryPolicy
    {
        [JsonProperty]
        public string retry_on = "5xx, connect-failure";

        [JsonProperty]
        public int num_retries = 5;

        public static EnvoyRetryPolicy defaultValue = new EnvoyRetryPolicy();
    }

}
