// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    public class EvoyOutlierDetection
    {
        [JsonProperty]
        public int consecutive_5xx = 3;

        static public EvoyOutlierDetection defaultValue = new EvoyOutlierDetection();
    }
}
