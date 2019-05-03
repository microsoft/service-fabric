// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EventStore.Service.ApiEndpoints.Common
{
    class UriConverters
    {
        public static string ConvertApiIdToFullUri(string entityId)
        {
            if (entityId.Contains("~"))
            {
                var segments = entityId.Split(
                    new[] { '~' },
                    StringSplitOptions.RemoveEmptyEntries);

                return string.Format("fabric:/{0}", string.Join("/", segments));
            }

            return string.Format("fabric:/{0}", entityId);
        }
    }
}
