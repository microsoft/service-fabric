// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Linq;
    using System.Net;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Newtonsoft.Json;

    internal class GoalStateModel
    {
        public List<PackageDetails> Packages
        {
            get;
            set;
        }

        public override string ToString()
        {
            if (this.Packages == null || this.Packages.Count() == 0)
            {
                return string.Empty;
            }

            var sb = new StringBuilder();
            sb.AppendLine("GoalStateModel packages:");
            foreach (var p in this.Packages)
            {
                sb.AppendLine(string.Format(CultureInfo.CurrentCulture, p.ToString()));
            }

            return sb.ToString();
        }

        internal static async Task<string> GetGoalStateFileJsonAsync(Uri goalStateUri)
        {
            string goalStateJson = null;

            try
            {
                goalStateJson = await StandaloneUtility.GetContentsFromUriAsyncWithRetry(goalStateUri, TimeSpan.FromMinutes(Constants.FabricOperationTimeoutInMinutes), CancellationToken.None);                
            }
            catch (FabricValidationException ex)
            {
                SFDeployerTrace.WriteError(StringResources.Error_SFGoalStateFileNotDownloaded, goalStateUri, ex.ToString());
            }

            return goalStateJson;
        }

        internal static GoalStateModel GetGoalStateModelFromJson(string json)
        {
            GoalStateModel model = null;
            try
            {
                model = JsonConvert.DeserializeObject<GoalStateModel>(json, new JsonSerializerSettings() { DefaultValueHandling = DefaultValueHandling.Populate, PreserveReferencesHandling = PreserveReferencesHandling.None });
            }
            catch (Exception ex)
            {
                SFDeployerTrace.WriteError(StringResources.Error_SFGoalStateFileNotDeserialize, ex.ToString());
            }

            return model;
        }
    }
}