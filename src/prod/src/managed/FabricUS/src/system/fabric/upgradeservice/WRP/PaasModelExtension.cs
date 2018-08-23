// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.WRP.Model
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.UpgradeService;
    using System.Linq;    

    internal static class PassModelExtension
    {
        public static CommandProcessorServiceTypeHealthPolicy ToCommondProcessorServiceTypeHealthPolicy(this PaasServiceTypeHealthPolicy paasServiceTypeHealthPolicy)
        {
            if (paasServiceTypeHealthPolicy == null) { return null; }

            return new CommandProcessorServiceTypeHealthPolicy()
            {
                MaxPercentUnhealthyServices = paasServiceTypeHealthPolicy.MaxPercentUnhealthyServices
            };
        }

        public static CommandProcessorApplicationHealthPolicy ToCommondProcessorServiceTypeHealthPolicy(this PaasApplicationHealthPolicy paasApplicationHealthPolicy)
        {
            if (paasApplicationHealthPolicy == null) { return null; }

            var result = new CommandProcessorApplicationHealthPolicy();
            if(paasApplicationHealthPolicy.DefaultServiceTypeHealthPolicy != null)
            {
                result.DefaultServiceTypeHealthPolicy = paasApplicationHealthPolicy.DefaultServiceTypeHealthPolicy.ToCommondProcessorServiceTypeHealthPolicy();
            }
            
            if(paasApplicationHealthPolicy.SerivceTypeHealthPolicies != null)
            {
                result.SerivceTypeHealthPolicies = paasApplicationHealthPolicy.SerivceTypeHealthPolicies.ToDictionary(
                    keyValuePair => keyValuePair.Key, 
                    keyValuePair => keyValuePair.Value.ToCommondProcessorServiceTypeHealthPolicy());
            }

            return result;
        }        
    }
}