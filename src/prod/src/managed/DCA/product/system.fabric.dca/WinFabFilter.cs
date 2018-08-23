// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using System.Collections.Generic;

    internal static class WinFabFilter
    {
        internal static string CreateStringRepresentation(Dictionary<string, Dictionary<string, int>> filters)
        {
            string stringRepresentation = string.Empty;
            List<string> filterStrings = new List<string>();
            foreach (string taskName in filters.Keys)
            {
                foreach (string eventType in filters[taskName].Keys)
                {
                    string filterString = string.Format(
                                                "{0}.{1}:{2}",
                                                taskName,
                                                eventType,
                                                filters[taskName][eventType]);
                    filterStrings.Add(filterString);
                }
            }

            if (filterStrings.Count > 0)
            {
                stringRepresentation = string.Join(",", filterStrings.ToArray());
            }

            return stringRepresentation;
        }
    }
}