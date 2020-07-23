// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    class EnvoyAccessLogConfig
    {
        public EnvoyAccessLogConfig()
        { }

        [JsonProperty]
        public string path = "./log/sfreverseproxy.access_log.log";
    }
}
