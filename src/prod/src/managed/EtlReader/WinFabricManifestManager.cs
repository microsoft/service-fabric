// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;

    public delegate void LoadWindowsFabricManifest(string manifestFileName);
    public delegate void UnloadWindowsFabricManifest(string manifestFileName);

    public class WinFabricManifestManager
    {
        public const string ManifestFileExtension = ".man";
        public const string FabricEtlPrefix = "fabric_traces_";
        public const string LeaseLayerEtlPrefix = "lease_traces_";
        public const string QueryEtlPrefix = "query_traces_";
        public const string OperationalEtlPrefix = "operational_traces_";

        private const string FabricWFManifestPrefix = "Microsoft-WindowsFabric-Events";
        private const string LeaseLayerWFManifestPrefix = "Microsoft-WindowsFabric-LeaseEvents";
        private const string KtlWFManifestPrefix = "Microsoft-WindowsFabric-KtlEvents";
        private const string FabricManifestPrefix = "Microsoft-ServiceFabric-Events";
        private const string LeaseLayerManifestPrefix = "Microsoft-ServiceFabric-LeaseEvents";
        private const string KtlManifestPrefix = "Microsoft-ServiceFabric-KtlEvents";
        private const string BlockStoreManifestPrefix = "Microsoft-ServiceFabric-BlockStoreEvents";

        private const string WindowsFabricV1FileVersion = "1.0.960.0";
        private const int VersionPartsCount = 4;
        private const int EtlFileSuffixPartsCountV1 = 2;
        private const int EtlFileSuffixPartsCountPostV1 = 3;

        private static string[] defaultManifests = new string[]
        {
            FabricManifestPrefix + ManifestFileExtension,
            LeaseLayerManifestPrefix + ManifestFileExtension,
            KtlManifestPrefix + ManifestFileExtension,
            BlockStoreManifestPrefix + ManifestFileExtension
        };

        private static string[] previousVersionManifestPrefixes = new string[]
        {
            String.Concat(FabricWFManifestPrefix, "_"),
            String.Concat(LeaseLayerWFManifestPrefix, "_"),
            String.Concat(KtlWFManifestPrefix, "_"),
            String.Concat(FabricManifestPrefix, "_"),
            String.Concat(LeaseLayerManifestPrefix, "_"),
            String.Concat(KtlManifestPrefix, "_")
        };

        public static IEnumerable<string> DefaultManifests
        {
            get
            {
                return defaultManifests;
            }
        }

        private Dictionary<long, string> fabricManifests;
        public IEnumerable<string> FabricManifests
        {
            get
            {
                return this.fabricManifests.Values;
            }
        }
        private Dictionary<long, string> leaseLayerManifests;
        public IEnumerable<string> LeaseLayerManifests
        {
            get
            {
                return this.leaseLayerManifests.Values;
            }
        }
        private Dictionary<long, string> ktlManifests;
        public IEnumerable<string> KtlManifests
        {
            get
            {
                return this.ktlManifests.Values;
            }
        }

        private List<long> fabricManifestVersions;
        private List<long> leaseLayerManifestVersions;
        private List<long> ktlManifestVersions;

        private readonly LoadWindowsFabricManifest loadManifest;
        private readonly UnloadWindowsFabricManifest unloadManifest;

        private string currentFabricManifest;
        private string currentLeaseLayerManifest;
        private string currentKtlManifest;

        public static bool IsPreviousVersionManifest(string manifestFile)
        {
            string manifestFileName = Path.GetFileName(manifestFile);
            return (null != previousVersionManifestPrefixes
                            .FirstOrDefault(
                                prefix =>
                                {
                                    return manifestFileName.StartsWith(
                                               prefix,
                                               StringComparison.OrdinalIgnoreCase);
                                }));
        }

        public static bool IsFabricManifestFileName(string manifestFileName)
        {
            return manifestFileName.StartsWith(WinFabricManifestManager.FabricWFManifestPrefix, StringComparison.OrdinalIgnoreCase) ||
                manifestFileName.StartsWith(WinFabricManifestManager.FabricManifestPrefix, StringComparison.OrdinalIgnoreCase);
        }

        public static bool IsLeaseLayerManifestFileName(string manifestFileName)
        {
            return manifestFileName.StartsWith(WinFabricManifestManager.LeaseLayerWFManifestPrefix, StringComparison.OrdinalIgnoreCase) ||
                manifestFileName.StartsWith(WinFabricManifestManager.LeaseLayerManifestPrefix, StringComparison.OrdinalIgnoreCase);
        }

        public static bool IsKtlManifestFileName(string manifestFileName)
        {
            return manifestFileName.StartsWith(WinFabricManifestManager.KtlWFManifestPrefix, StringComparison.OrdinalIgnoreCase) ||
                manifestFileName.StartsWith(WinFabricManifestManager.KtlManifestPrefix, StringComparison.OrdinalIgnoreCase);
        }

        public WinFabricManifestManager(IEnumerable<string> manifests, LoadWindowsFabricManifest loadManifestMethod, UnloadWindowsFabricManifest unloadManifestMethod)
        {
            this.fabricManifests = new Dictionary<long, string>();
            this.leaseLayerManifests = new Dictionary<long, string>();
            this.ktlManifests = new Dictionary<long, string>();
            this.fabricManifestVersions = new List<long>();
            this.leaseLayerManifestVersions = new List<long>();
            this.ktlManifestVersions = new List<long>();
            this.loadManifest = loadManifestMethod;
            this.unloadManifest = unloadManifestMethod;

            if (manifests != null)
            {
                foreach (string manifest in manifests)
                {
                    long version;
                    var manifestFileName = Path.GetFileName(manifest);
                    version = GetManifestVersion(manifestFileName);
                    
                    if (WinFabricManifestManager.IsFabricManifestFileName(manifestFileName))
                    {
                        AddManifest(manifest, version, this.fabricManifests, this.fabricManifestVersions);
                    }
                    else if (WinFabricManifestManager.IsLeaseLayerManifestFileName(manifestFileName))
                    {
                        AddManifest(manifest, version, this.leaseLayerManifests, this.leaseLayerManifestVersions);
                    }
                    else if (WinFabricManifestManager.IsKtlManifestFileName(manifestFileName))
                    {
                        AddManifest(manifest, version, this.ktlManifests, this.ktlManifestVersions);
                    }
                }
            }
        }

        public void EnsureCorrectWinFabManifestVersionLoaded(string etlFileName, out bool exactMatchFound)
        {
            List<string> manifestFiles = GetManifestsForEtl(Path.GetFileName(etlFileName), out exactMatchFound);

            foreach (string manifestFile in manifestFiles)
            {
                string manifestFileName = Path.GetFileName(manifestFile);
                if (WinFabricManifestManager.IsFabricManifestFileName(manifestFileName))
                {
                    this.EnsureCorrectWinFabManifestVersionLoaded(ref this.currentFabricManifest, manifestFile);
                }
                else if (WinFabricManifestManager.IsLeaseLayerManifestFileName(manifestFileName))
                {
                    this.EnsureCorrectWinFabManifestVersionLoaded(ref this.currentLeaseLayerManifest, manifestFile);
                }
                else if (WinFabricManifestManager.IsKtlManifestFileName(manifestFileName))
                {
                    this.EnsureCorrectWinFabManifestVersionLoaded(ref this.currentKtlManifest, manifestFile);
                }
                else
                {
                    Debug.Assert(false, "Unexpected manifest file.");
                    throw new InvalidDataException(
                                  String.Format(
                                      System.Globalization.CultureInfo.CurrentCulture,
                                      StringResources.ETLReaderError_UnexpectedManifestFileReturned_formatted,
                                      manifestFile,
                                      etlFileName));
                }
            }
        }

        private static long GetManifestVersion(string manifestFileName)
        {
            if (WinFabricManifestManager.DefaultManifests.Contains(manifestFileName))
            {
                // This is the default manifest. It has no version number.
                return Int64.MaxValue;
            }

            if (!manifestFileName.Contains('_'))
            {
                throw new ArgumentException(
                              String.Format(
                                  System.Globalization.CultureInfo.CurrentCulture,
                                  StringResources.ETLReaderError_InvalidManifestFileName_formatted,
                                  manifestFileName));
            }

            var manifestSuffix = Path.ChangeExtension(manifestFileName, null).Substring(manifestFileName.LastIndexOf('_') + 1);
            return ParseStringAsVersion(manifestFileName, manifestSuffix);
        }

        private static void AddManifest(string manifest, long version, Dictionary<long, string> manifests, List<long> manifestVersions)
        {
            manifests[version] = manifest;
            int index = manifestVersions.BinarySearch(version);
            if (index < 0)
            {
                index = ~index;
                manifestVersions.Insert(index, version);
            }
        }

        private static long ParseStringAsVersion(string fileName, string versionString)
        {
            string[] versionParts = versionString.Split('.');
            if (versionParts.Length != VersionPartsCount)
            {
                throw new ArgumentException(
                              String.Format(
                                  System.Globalization.CultureInfo.CurrentCulture,
                                  StringResources.ETLReaderError_IncorrectVersionComponentInFileName_formatted,
                                  fileName,
                                  VersionPartsCount));
            }

            long version = 0;
            for (int i = 0; i < VersionPartsCount; i++)
            {
                version = version << 16;

                UInt16 versionPart;
                if (false == UInt16.TryParse(
                                 versionParts[i],
                                 out versionPart))
                {
                    throw new ArgumentException(
                                  String.Format(
                                      System.Globalization.CultureInfo.CurrentCulture,
                                      StringResources.ETLReaderError_InvalidFileName_formatted,
                                      fileName,
                                      VersionPartsCount));
                }

                version = version | (long)versionPart;
            }

            return version;
        }

        private static long GetEtlVersion(string etlFileName, string etlFileNameSuffix)
        {
            string versionString;
#if !DotNetCoreClrLinux
            string[] suffixParts = etlFileNameSuffix.Split('_');

            if (suffixParts.Length == EtlFileSuffixPartsCountV1)
            {
                // The events in this file were written by V1 binaries
                versionString = WindowsFabricV1FileVersion;
            }
            else if (suffixParts.Length == EtlFileSuffixPartsCountPostV1)
            {
                versionString = suffixParts[0];
            }
            else
            {
                throw new ArgumentException(
                              String.Format(
                                  System.Globalization.CultureInfo.CurrentCulture,
                                  StringResources.ETLReaderError_UnexpectedETLFileFomat_formatted,
                                  etlFileName));
            }
#else
            // In Linux the etlFileName parameter should get here with only the version string
            versionString = etlFileNameSuffix;
#endif

            return ParseStringAsVersion(etlFileName, versionString);
        }

        private static string GetMatchingManifestForVersion(string etlFileName, long version, Dictionary<long, string> manifests, List<long> manifestVersions, out bool exactMatchFound)
        {
            exactMatchFound = false;
            if (manifestVersions.Count == 0)
            {
                return null;
            }

            int matchingIndex = manifestVersions.BinarySearch(version);
            if (matchingIndex < 0)
            {
                int nearestHigherIndex = ~matchingIndex;

                if (nearestHigherIndex == 0)
                {
                    // The ETL version is older than any version we have a match for.
                    // So just use the oldest version that we have.
                    matchingIndex = nearestHigherIndex;
                }
                else if (nearestHigherIndex == manifestVersions.Count)
                {
                    // The ETL version is newer than any version we have a match for.
                    // So just use the newest version that we have.
                    matchingIndex = manifestVersions.Count - 1;
                }
                else if (manifestVersions[nearestHigherIndex] == Int64.MaxValue)
                {
                    // The ETL version is higher than our most recently released
                    // version. So use the default manifest. The default manifest
                    // is not versioned and therefore has the version value
                    // Int64.MaxValue.
                    matchingIndex = nearestHigherIndex;

                    // The ETL version is higher than our most recently released
                    // version, then it means that we are working with a version
                    // that has not yet been released. Such pre-release versions
                    // always match against the default manifest, which is not
                    // versioned. We want to avoid logging a warning in this case,
                    // so we always consider it to be an exact match.
                    exactMatchFound = true;
                }
                else
                {
                    // The ETL version is in between two versions for which we
                    // have manifests available. Let's see which of those two
                    // versions would be a better match.
                    matchingIndex = GetBetterMatch(
                                        version,
                                        manifestVersions,
                                        nearestHigherIndex - 1,
                                        nearestHigherIndex);
                }
            }
            else
            {
                exactMatchFound = true;
            }
            long matchingVersion = manifestVersions[matchingIndex];
            return manifests[matchingVersion];
        }

        private static int GetBetterMatch(long version, List<long> manifestVersions, int index1, int index2)
        {
            UInt16[] versionParts = GetVersionParts(version);
            UInt16[] versionParts1 = GetVersionParts(manifestVersions[index1]);
            UInt16[] versionParts2 = GetVersionParts(manifestVersions[index2]);

            int closeness1 = GetCloseness(versionParts, versionParts1);
            int closeness2 = GetCloseness(versionParts, versionParts2);

            if (closeness1 > closeness2)
            {
                return index1;
            }
            else
            {
                return index2;
            }
        }

        private static UInt16[] GetVersionParts(long version)
        {
            UInt16[] parts = new UInt16[VersionPartsCount];
            for (int i = 0; i < parts.Length; i++)
            {
                parts[i] = (UInt16)(version & 0x000000000000FFFF);
                version = version >> 16;
            }
            return parts;
        }

        private static int GetCloseness(UInt16[] version1, UInt16[] version2)
        {
            int closeness = 0;
            for (int i = (VersionPartsCount-1); i >= 0; i--)
            {
                if (version1[i] != version2[i])
                {
                    break;
                }
                closeness++;
            }
            return closeness;
        }

        private List<string> GetManifestsForEtl(string etlFileName, out bool exactMatchFound)
        {
            exactMatchFound = true;

            bool exactMatchLatest;
            long version;
            string manifest;
            List<string> manifests = new List<string>();

            if (etlFileName.StartsWith(FabricEtlPrefix, StringComparison.OrdinalIgnoreCase))
            {
                // Fabric manifest
                version = GetEtlVersion(
                              etlFileName,
                              etlFileName.Remove(0, FabricEtlPrefix.Length));
                manifest = GetMatchingManifestForVersion(etlFileName, version, this.fabricManifests, this.fabricManifestVersions, out exactMatchLatest);
                if (null != manifest)
                {
                    manifests.Add(manifest);
                }

                exactMatchFound = exactMatchFound && exactMatchLatest;
            }
            else if (etlFileName.StartsWith(LeaseLayerEtlPrefix, StringComparison.OrdinalIgnoreCase))
            {
                // Lease layer manifest
                version = GetEtlVersion(
                              etlFileName,
                              etlFileName.Remove(0, LeaseLayerEtlPrefix.Length));
                manifest = GetMatchingManifestForVersion(etlFileName, version, this.leaseLayerManifests, this.leaseLayerManifestVersions, out exactMatchLatest);
                if (null != manifest)
                {
                    manifests.Add(manifest);
                }
                exactMatchFound = exactMatchFound && exactMatchLatest;

                // KTL manifest
                version = GetEtlVersion(
                              etlFileName,
                              etlFileName.Remove(0, LeaseLayerEtlPrefix.Length));
                manifest = GetMatchingManifestForVersion(etlFileName, version, this.ktlManifests, this.ktlManifestVersions, out exactMatchLatest);
                if (null != manifest)
                {
                    manifests.Add(manifest);
                }
                exactMatchFound = exactMatchFound && exactMatchLatest;
            }
            else if (etlFileName.StartsWith(QueryEtlPrefix, StringComparison.OrdinalIgnoreCase))
            {
                // ForQuery manifest
                version = GetEtlVersion(
                              etlFileName,
                              etlFileName.Remove(0, QueryEtlPrefix.Length));
                manifest = GetMatchingManifestForVersion(etlFileName, version, this.fabricManifests, this.fabricManifestVersions, out exactMatchLatest);
                if (null != manifest)
                {
                    manifests.Add(manifest);
                }
                
                exactMatchFound = exactMatchFound && exactMatchLatest;
            }
            else if (etlFileName.StartsWith(OperationalEtlPrefix, StringComparison.OrdinalIgnoreCase))
            {
                // Operational manifest
                version = GetEtlVersion(
                              etlFileName,
                              etlFileName.Remove(0, OperationalEtlPrefix.Length));
                manifest = GetMatchingManifestForVersion(etlFileName, version, this.fabricManifests, this.fabricManifestVersions, out exactMatchLatest);
                if (null != manifest)
                {
                    manifests.Add(manifest);
                }

                exactMatchFound = exactMatchFound && exactMatchLatest;
            }
            else
            {
                throw new ArgumentException(
                              String.Format(
                                  System.Globalization.CultureInfo.CurrentCulture,
                                  StringResources.ETLReaderError_MustBeginWithStandardPrefix_formatted,
                                  etlFileName));
            }
            return manifests;
        }

        private void EnsureCorrectWinFabManifestVersionLoaded(ref string currentManifestFile, string requiredManifestFile)
        {
            if (String.IsNullOrEmpty(currentManifestFile))
            {
                // No manifest is currently loaded. Load the one that we need.
                this.loadManifest(requiredManifestFile);
                currentManifestFile = requiredManifestFile;
            }
            else if (false == currentManifestFile.Equals(requiredManifestFile))
            {
                // The currently loaded manifest is not the one we need. Unload it
                // and load the correct manifest instead.
                this.unloadManifest(currentManifestFile);
                this.loadManifest(requiredManifestFile);
                currentManifestFile = requiredManifestFile;
            }
        }
    }
}