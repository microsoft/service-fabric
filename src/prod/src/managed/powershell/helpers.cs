// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Health;
    using System.Globalization;
    using System.Management.Automation;

    internal static class Helpers
    {
        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Reviewed. Suppression is OK here for psInfo and psNote.")]
        internal static void AddToPsObject(PSObject psObject, IList<HealthEvaluation> unhealthyEvaluations)
        {
            if (unhealthyEvaluations != null && unhealthyEvaluations.Count > 0)
            {
                var unhealthyEvaluationsPropertyPSObj = new PSObject(unhealthyEvaluations);
                unhealthyEvaluationsPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                psObject.Properties.Add(
                    new PSNoteProperty(
                        Constants.UnhealthyEvaluationsPropertyName,
                        unhealthyEvaluationsPropertyPSObj));
            }
        }

        internal static void AddToPSObject(PSObject itemPSObj, HealthStatistics healthStats)
        {
            if (healthStats == null || healthStats.HealthStateCountList.Count == 0)
            {
                return;
            }

            var healthStatisticsPropertyPSObj = new PSObject(healthStats);
            healthStatisticsPropertyPSObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            itemPSObj.Properties.Add(
                new PSNoteProperty(
                    Constants.HealthStatisticsPropertyName,
                    healthStatisticsPropertyPSObj));
        }

        internal static bool ResultHasMorePages(Cmdlet cmdlet, string continuationToken, out string currentContinuationToken)
        {
            if (string.IsNullOrEmpty(continuationToken))
            {
                // All pages are received, done
                currentContinuationToken = null;
                return false;
            }
            else
            {
                cmdlet.WriteCommandDetail(GetContinuationTokenMessage(continuationToken));
                currentContinuationToken = continuationToken;
                return true;
            }
        }

        internal static string GetContinuationTokenMessage(string continuationToken)
        {
            return string.Format(CultureInfo.CurrentCulture, "{0}: {1}", Constants.ContinuationTokenPropertyName, continuationToken);
        }
    }
}