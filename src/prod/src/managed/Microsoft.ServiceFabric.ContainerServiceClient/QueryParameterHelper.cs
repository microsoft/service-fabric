// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    //using System.Linq;
    using System.Net;
    using System.Net.Http;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;
    using Microsoft.ServiceFabric.ContainerServiceClient.Response;

    internal static class QueryParameterHelper
    {
        internal static string BuildQueryString(ContainerEventsConfig eventsConfig)
        {
            // TODO: use string.join.

            var sb = new StringBuilder();

            if(!string.IsNullOrEmpty(eventsConfig.Since))
            {
                sb.Append($"since={eventsConfig.Since}");
            }

            if (!string.IsNullOrEmpty(eventsConfig.Until))
            {
                if(sb.Length > 0)
                {
                    sb.Append("&");
                }

                sb.Append($"until={eventsConfig.Until}");
            }

            if (eventsConfig.Filters != null && eventsConfig.Filters.Count > 0)
            {
                if (sb.Length > 0)
                {
                    sb.Append("&");
                }

                var filterJsonStr = ContainerServiceClient.JsonSerializer.SerializeObject(eventsConfig.Filters);

                filterJsonStr = Uri.EscapeDataString(filterJsonStr);

                sb.Append($"filters={filterJsonStr}");
            }

            return sb.ToString();
        }

        internal static string BuildQueryString(NetworksListConfig listConfig)
        {
            return BuildFilterParam(listConfig.Filters);
        }

        internal static string BuildQueryString(ContainersPruneConfig pruneConfig)
        {
            return BuildFilterParam(pruneConfig.Filters);
        }

        private static string BuildFilterParam(IDictionary<string, IList<string>> filters)
        {
            if(filters == null || filters.Count == 0)
            {
                return null;
            }

            var filterJsonStr = ContainerServiceClient.JsonSerializer.SerializeObject(filters);

            filterJsonStr = Uri.EscapeDataString(filterJsonStr);

            return $"filters={filterJsonStr}";
        }
    }
}