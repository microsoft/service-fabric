// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;
    using Newtonsoft.Json;

    /// <summary>
    /// This class analyzes the delta between the current and target certs, and figures out:
    ///     a. how many steps is required to complete the cert upgrade.
    ///     b. what are the target white/ load/ fileStoreSvc lists for each step.
    /// </summary>
    internal static class CertificateClusterUpgradeFlow
    {
        private static UpgradeStepDelegate[] upgradeStepDelegateTable = new UpgradeStepDelegate[3]
            {
                CertificateClusterUpgradeFlow.TryGetStepOne,
                CertificateClusterUpgradeFlow.TryGetStepTwo,
                CertificateClusterUpgradeFlow.TryGetStepThree
            };

        internal delegate bool UpgradeStepDelegate(
            X509 currentCerts,
            X509 targetCerts,
            CertificateClusterUpgradeStep previousStep,
            out CertificateClusterUpgradeStep step);

        public static List<CertificateClusterUpgradeStep> GetUpgradeFlow(X509 currentCerts, X509 targetCerts)
        {
            List<CertificateClusterUpgradeStep> result = new List<CertificateClusterUpgradeStep>();
            CertificateClusterUpgradeStep previousStep = null;
            bool shouldContinue = true;

            for (int i = 0; i < CertificateClusterUpgradeFlow.upgradeStepDelegateTable.Length && shouldContinue; i++)
            {
                CertificateClusterUpgradeStep step;
                shouldContinue = CertificateClusterUpgradeFlow.upgradeStepDelegateTable[i](currentCerts, targetCerts, previousStep, out step);
                previousStep = step;
                if (step != null)
                {
                    result.Add(step);
                }
            }

            return result;
        }

        internal static bool TryGetStepOne(
            X509 currentCerts,
            X509 targetCerts,
            CertificateClusterUpgradeStep previousStep,
            out CertificateClusterUpgradeStep step)
        {
            // step 1 adds all newly added certs and issuers (if any) to the white list
            step = null;
            List<string> currentThumbprints = CertificateClusterUpgradeFlow.GetThumbprints(currentCerts.ClusterCertificate);
            List<string> addedThumbprints = CertificateClusterUpgradeFlow.GetAddedThumbprints(currentCerts.ClusterCertificate, targetCerts.ClusterCertificate);
            Dictionary<string, string> currentCns = CertificateClusterUpgradeFlow.GetCns(currentCerts.ClusterCertificateCommonNames);
            Dictionary<string, string> addedCnsAndIssuers = CertificateClusterUpgradeFlow.GetAddedCnsAndIssuers(currentCerts.ClusterCertificateCommonNames, targetCerts.ClusterCertificateCommonNames);

            if (addedThumbprints.Any() || addedCnsAndIssuers.Any())
            {
                step = new CertificateClusterUpgradeStep(
                    thumbprintWhiteList: currentThumbprints.Concat(addedThumbprints).ToList(),
                    thumbprintLoadList: currentCerts.ClusterCertificate,
                    thumbprintFileStoreSvcList: currentCerts.ClusterCertificate,
                    commonNameWhiteList: CertificateClusterUpgradeFlow.MergeCnsAndIssuers(currentCns, addedCnsAndIssuers),
                    commonNameLoadList: currentCerts.ClusterCertificateCommonNames,
                    commonNameFileStoreSvcList: currentCerts.ClusterCertificateCommonNames);
            }

            return true;
        }

        internal static bool TryGetStepTwo(
            X509 currentCerts,
            X509 targetCerts,
            CertificateClusterUpgradeStep previousStep,
            out CertificateClusterUpgradeStep step)
        {
            // step 2:
            // white list: inherit from the previous step
            // load list: replace all current certs with target certs
            // fss list: change if necessary
            int changedThumbprintCount = CertificateClusterUpgradeFlow.GetChangedThumbprintCount(currentCerts.ClusterCertificate, targetCerts.ClusterCertificate);
            int changedCnCount = CertificateClusterUpgradeFlow.GetChangedCnCount(currentCerts.ClusterCertificateCommonNames, targetCerts.ClusterCertificateCommonNames);

            List<string> removedThumbprints = CertificateClusterUpgradeFlow.GetAddedThumbprints(targetCerts.ClusterCertificate, currentCerts.ClusterCertificate);
            List<string> removedCns = CertificateClusterUpgradeFlow.GetAddedCns(targetCerts.ClusterCertificateCommonNames, currentCerts.ClusterCertificateCommonNames);

            CertificateDescription thumbprintFileStoreSvcCerts = null;
            ServerCertificateCommonNames commonNameFileStoreSvcCerts = null;
            bool shouldContinue = true;

            switch (changedThumbprintCount + changedCnCount)
            {
                case 1:
                    {
                        if (removedThumbprints.Any() || removedCns.Any())
                        {
                            // cert removal
                            thumbprintFileStoreSvcCerts = currentCerts.ClusterCertificate;
                            commonNameFileStoreSvcCerts = currentCerts.ClusterCertificateCommonNames;
                        }
                        else
                        {
                            // cert add
                            thumbprintFileStoreSvcCerts = targetCerts.ClusterCertificate;
                            commonNameFileStoreSvcCerts = targetCerts.ClusterCertificateCommonNames;
                            shouldContinue = false;
                        }
                        
                        break;
                    }

                case 2:
                    {
                        if (CertificateClusterUpgradeFlow.IsSwap(currentCerts.ClusterCertificate, targetCerts.ClusterCertificate))
                        {
                            thumbprintFileStoreSvcCerts = new CertificateDescription()
                            {
                                Thumbprint = currentCerts.ClusterCertificate.Thumbprint,
                                ThumbprintSecondary = currentCerts.ClusterCertificate.Thumbprint,
                                X509StoreName = currentCerts.ClusterCertificate.X509StoreName,
                            };
                        }
                        else
                        {
                            if (changedThumbprintCount == 2)
                            {
                                // thumbprint replace
                                thumbprintFileStoreSvcCerts = new CertificateDescription()
                                {
                                    Thumbprint = currentCerts.ClusterCertificate.Thumbprint,
                                    ThumbprintSecondary = targetCerts.ClusterCertificate.Thumbprint,
                                    X509StoreName = currentCerts.ClusterCertificate.X509StoreName,
                                };
                            }
                            else if (changedCnCount == 2)
                            {
                                // cn replace
                                commonNameFileStoreSvcCerts = new ServerCertificateCommonNames()
                                {
                                    CommonNames = new List<CertificateCommonNameBase>()
                                    {
                                        currentCerts.ClusterCertificateCommonNames.CommonNames[0],
                                        targetCerts.ClusterCertificateCommonNames.CommonNames[0]
                                    },
                                    X509StoreName = currentCerts.ClusterCertificateCommonNames.X509StoreName
                                };
                            }
                            else
                            {
                                // 1 thumbprint <-> 1 cn
                                CertificateClusterUpgradeFlow.GetFileStoreSvcListForCertTypeChange(
                                    currentCerts,
                                    targetCerts,
                                    out thumbprintFileStoreSvcCerts,
                                    out commonNameFileStoreSvcCerts);
                            }
                        }

                        break;
                    }

                case 3:
                case 4:
                    {
                        // 1 thumbprints <-> 2 cns, or 2 thumbprints <-> 1 cns, or 2 thumbprints <-> 2 cns
                        CertificateClusterUpgradeFlow.GetFileStoreSvcListForCertTypeChange(
                            currentCerts,
                            targetCerts,
                            out thumbprintFileStoreSvcCerts,
                            out commonNameFileStoreSvcCerts);
                        break;
                    }

                default:
                    throw new NotSupportedException(string.Format("It's not supported that {0} certificate thumbprints and {1} certificate common names have changed", changedThumbprintCount, changedCnCount));
            }

            step = new CertificateClusterUpgradeStep(
                thumbprintWhiteList: previousStep != null ? previousStep.ThumbprintWhiteList : CertificateClusterUpgradeFlow.GetThumbprints(currentCerts.ClusterCertificate),
                thumbprintLoadList: targetCerts.ClusterCertificate,
                thumbprintFileStoreSvcList: thumbprintFileStoreSvcCerts,
                commonNameWhiteList: previousStep != null ? previousStep.CommonNameWhiteList : CertificateClusterUpgradeFlow.GetCns(currentCerts.ClusterCertificateCommonNames),
                commonNameLoadList: targetCerts.ClusterCertificateCommonNames,
                commonNameFileStoreSvcList: commonNameFileStoreSvcCerts);

            return shouldContinue;
        }

        internal static bool TryGetStepThree(
            X509 currentCerts,
            X509 targetCerts,
            CertificateClusterUpgradeStep previousStep,
            out CertificateClusterUpgradeStep step)
        {
            Dictionary<string, string> commonNameWhiteList = targetCerts.ClusterCertificateCommonNames == null || !targetCerts.ClusterCertificateCommonNames.Any() ?
                    null : targetCerts.ClusterCertificateCommonNames.CommonNames.ToDictionary(p => p.CertificateCommonName, p => p.CertificateIssuerThumbprint);
            step = new CertificateClusterUpgradeStep(
                thumbprintWhiteList: targetCerts.ClusterCertificate == null ? null : targetCerts.ClusterCertificate.ToThumbprintList(),
                thumbprintLoadList: targetCerts.ClusterCertificate,
                thumbprintFileStoreSvcList: targetCerts.ClusterCertificate,
                commonNameWhiteList: commonNameWhiteList,
                commonNameLoadList: targetCerts.ClusterCertificateCommonNames,
                commonNameFileStoreSvcList: targetCerts.ClusterCertificateCommonNames);

            return true;
        }

        internal static List<string> GetAddedThumbprints(
            CertificateDescription originalList,
            CertificateDescription newList)
        {
            List<string> originalThumbprints = originalList == null ? new List<string>() : originalList.ToThumbprintList();
            List<string> newThumbprints = newList == null ? new List<string>() : newList.ToThumbprintList();
            return newThumbprints.Except(originalThumbprints, StringComparer.OrdinalIgnoreCase).ToList();
        }

        internal static List<string> GetAddedCns(
            ServerCertificateCommonNames originalList,
            ServerCertificateCommonNames newList)
        {
            List<string> currentCns = originalList == null || originalList.CommonNames == null ?
                new List<string>() : originalList.CommonNames.Select(p => p.CertificateCommonName).ToList();
            List<string> newCns = newList == null || newList.CommonNames == null ?
                new List<string>() : newList.CommonNames.Select(p => p.CertificateCommonName).ToList();

            return newCns.Except(currentCns).ToList();
        }

        internal static Dictionary<string, string> GetAddedCnsAndIssuers(
            ServerCertificateCommonNames originalList,
            ServerCertificateCommonNames newList)
        {
            Dictionary<string, string> currentCns = originalList == null || originalList.CommonNames == null ?
                new Dictionary<string, string>() : originalList.CommonNames.ToDictionary(p => p.CertificateCommonName, p => p.CertificateIssuerThumbprint);
            Dictionary<string, string> newCns = newList == null || newList.CommonNames == null ?
                new Dictionary<string, string>() : newList.CommonNames.ToDictionary(p => p.CertificateCommonName, p => p.CertificateIssuerThumbprint);

            Dictionary<string, string> result = new Dictionary<string, string>();

            foreach (var newCn in newCns)
            {
                string addedValue = null;
                bool isAddedFound = false;

                if (!currentCns.ContainsKey(newCn.Key))
                {
                    addedValue = newCn.Value;
                    isAddedFound = true;
                }
                else if (currentCns[newCn.Key] != newCn.Value)
                {
                    string[] currentThumbprints = currentCns[newCn.Key] == null ? new string[0] : currentCns[newCn.Key].Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
                    string[] newThumbprints = newCn.Value == null ? new string[0] : newCn.Value.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
                    IEnumerable<string> addedThumbprints = newThumbprints.Except(currentThumbprints, StringComparer.OrdinalIgnoreCase);
                    if (addedThumbprints.Any())
                    {
                        addedValue = string.Join(",", addedThumbprints);
                        isAddedFound = true;
                    }
                }

                if (isAddedFound)
                {
                    result[newCn.Key] = addedValue;
                }
            }
            
            return result;
        }

        internal static Dictionary<string, string> MergeCnsAndIssuers(Dictionary<string, string> originalCns, Dictionary<string, string> addedCns)
        {
            Dictionary<string, string> result = new Dictionary<string, string>(originalCns);

            foreach (var addedCn in addedCns)
            {
                string newValue = null;
                string key = addedCn.Key;

                if (!result.ContainsKey(key))
                {
                    newValue = addedCn.Value;
                }
                else
                {
                    string[] currentThumbprints = result[key] == null ? new string[0] : result[key].Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
                    string[] addedThumbprints = addedCn.Value == null ? new string[0] : addedCn.Value.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
                    newValue = string.Join(",", currentThumbprints.Union(addedThumbprints));
                }

                result[key] = newValue;
            }

            return result;
        }

        internal static List<string> GetThumbprints(CertificateDescription certs)
        {
            return certs == null ? new List<string>() : certs.ToThumbprintList();
        }

        internal static Dictionary<string, string> GetCns(ServerCertificateCommonNames certs)
        {
            return certs == null || certs.CommonNames == null ?
                            new Dictionary<string, string>() : certs.CommonNames.ToDictionary(p => p.CertificateCommonName, p => p.CertificateIssuerThumbprint);
        }

        internal static int GetChangedThumbprintCount(CertificateDescription currentCerts, CertificateDescription targetCerts)
        {
            List<string> currentThumbprints = CertificateClusterUpgradeFlow.GetThumbprints(currentCerts);
            List<string> targetThumbprints = CertificateClusterUpgradeFlow.GetThumbprints(targetCerts);

            return CertificateClusterUpgradeFlow.GetChangedCertCount(currentThumbprints, targetThumbprints, isThumbprint: true);
        }

        internal static int GetChangedCnCount(ServerCertificateCommonNames currentCerts, ServerCertificateCommonNames targetCerts)
        {
            List<string> currentCns = CertificateClusterUpgradeFlow.GetCns(currentCerts).Keys.ToList();
            List<string> targetCns = CertificateClusterUpgradeFlow.GetCns(targetCerts).Keys.ToList();

            return CertificateClusterUpgradeFlow.GetChangedCertCount(currentCns, targetCns, isThumbprint: false);
        }

        internal static int GetChangedCertCount(List<string> currentValues, List<string> targetValues, bool isThumbprint)
        {
            if (currentValues.Count == 0 || targetValues.Count == 0)
            {
                return Math.Max(currentValues.Count, targetValues.Count);
            }

            List<string> fssCurrentValues = (currentValues.Count > 1) ? currentValues : new List<string>() { currentValues[0], currentValues[0] };
            List<string> fssTargetValues = (targetValues.Count > 1) ? targetValues : new List<string>() { targetValues[0], targetValues[0] };

            int result = 0;
            StringComparison comparisonType = isThumbprint ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal;
            for (int i = 0; i < fssCurrentValues.Count; i++)
            {
                if (!string.Equals(fssCurrentValues[i], fssTargetValues[i], comparisonType))
                {
                    result++;
                }
            }

            return result;
        }

        internal static void GetFileStoreSvcListForCertTypeChange(
            X509 currentCerts,
            X509 targetCerts,
            out CertificateDescription thumbprintFileStoreSvcCerts,
            out ServerCertificateCommonNames commonNameFileStoreSvcCerts)
        {
            if (targetCerts.ClusterCertificate == null)
            {
                thumbprintFileStoreSvcCerts = currentCerts.ClusterCertificate;
                commonNameFileStoreSvcCerts = targetCerts.ClusterCertificateCommonNames;
            }
            else
            {
                thumbprintFileStoreSvcCerts = targetCerts.ClusterCertificate;
                commonNameFileStoreSvcCerts = currentCerts.ClusterCertificateCommonNames;
            }
        }

        internal static bool IsSwap(CertificateDescription currentCerts, CertificateDescription targetCerts)
        {
            if (currentCerts == null || targetCerts == null)
            {
                return false;
            }

            return currentCerts.Thumbprint.Equals(targetCerts.ThumbprintSecondary, StringComparison.OrdinalIgnoreCase)
                && targetCerts.Thumbprint.Equals(currentCerts.ThumbprintSecondary, StringComparison.OrdinalIgnoreCase)
                && !currentCerts.Thumbprint.Equals(currentCerts.ThumbprintSecondary, StringComparison.OrdinalIgnoreCase);
        }
    }
}