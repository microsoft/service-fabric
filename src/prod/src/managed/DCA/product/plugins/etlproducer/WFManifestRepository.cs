// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Dca;
    using System.IO;
    using System.Linq;
    using Tools.EtlReader;

    // Repository of Windows Fabric ETW manifests from various Windows Fabric 
    // versions. The ETL files on a machine can contain events from various
    // Windows Fabric versions because of upgrades that may be been performed
    // over the lifetime of the machine. Having a repository of Windows Fabric
    // ETW manifests enables events from these ETL files to be decoded. 
    internal class WFManifestRepository
    {
        // Constants
        private const string RepositoryFolderName = "WFEtwMan";

        // Directory that serves as a repository for Windows Fabric manifests
        private string winFabManifestRepository;

        internal string RepositoryPath
        {
            get
            {
                lock (this)
                {
                    // If we've already created a repository, then return it immediately.
                    if (false == string.IsNullOrEmpty(this.winFabManifestRepository))
                    {
                        return this.winFabManifestRepository;
                    }

                    // Compute the full path to the repository
                    string repositoryPath = Path.Combine(Utility.DcaWorkFolder, RepositoryFolderName);

                    // Compute the location of the currently running assembly
                    string assemblyLocation = Process.GetCurrentProcess().MainModule.FileName;
                    FileVersionInfo versionInfo = FileVersionInfo.GetVersionInfo(assemblyLocation);           

                    // Source location of the manifest files
                    string manifestFileSourceLocation = Path.GetDirectoryName(assemblyLocation);

                    // List of default manifests
                    IEnumerable<string> defaultManifests = FabricDirectory.GetFiles(manifestFileSourceLocation, "*.man")
                                                           .Where(path =>
                                                                  {
                                                                      string file = Path.GetFileName(path);
                                                                      return WinFabricManifestManager.DefaultManifests.Contains(file);
                                                                  });

                    // List of non-default manifests
                    IEnumerable<string> nonDefaultManifests = FabricDirectory.GetFiles(manifestFileSourceLocation, "*.man")
                                                              .Where(path =>
                                                                     {
                                                                         string file = Path.GetFileName(path);
                                                                         return !WinFabricManifestManager.DefaultManifests.Contains(file);
                                                                    });

                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            // Create the directory where the ETW manifest repository is located.
                            FabricDirectory.CreateDirectory(repositoryPath);

                            // Include the build version in each of the default manifest names
                            // and copy the manifest over to the repository
                            foreach (string defaultManifest in defaultManifests)
                            {
                                string defaultManifestWithoutExt = Path.GetFileNameWithoutExtension(
                                    defaultManifest);
                                string defaultManifestWithVersion = string.Format(
                                    "{0}_{1}.{2}.{3}.{4}{5}",
                                    defaultManifestWithoutExt,
                                    versionInfo.FileMajorPart,
                                    versionInfo.FileMinorPart,
                                    versionInfo.FileBuildPart,
                                    versionInfo.FilePrivatePart,
                                    WinFabricManifestManager.ManifestFileExtension);
                                string defaultManifestDestinationPath = Path.Combine(
                                    repositoryPath,
                                    defaultManifestWithVersion);
                                FabricFile.Copy(defaultManifest, defaultManifestDestinationPath, true);

                                // In case we roll back to Microsoft-WindowsFabric*.man based DCA version
                                // write out both styles of names.  This can be removed once we don't
                                // support rollback to 4.4 and earlier.
                                FabricFile.Copy(defaultManifest, defaultManifestDestinationPath.Replace("ServiceFabric", "WindowsFabric"), true);
                            }

                            // Copy the non-default manifest over to the repository
                            foreach (string nonDefaultManifest in nonDefaultManifests)
                            {
                                string nonDefaultManifestName = Path.GetFileName(nonDefaultManifest);

                                string nonDefaultManifestDestinationPath = Path.Combine(
                                    repositoryPath,
                                    nonDefaultManifestName);

                                FabricFile.Copy(nonDefaultManifest, nonDefaultManifestDestinationPath, true);
                            }
                        });

                    this.winFabManifestRepository = repositoryPath;
                    return this.winFabManifestRepository;
                }
            }
        }
    }
}