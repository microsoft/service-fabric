// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Common;
using System.IO;
using System.Linq;
using System.Reflection;

namespace System.Fabric.BackupRestore
{
    internal class BackupRestoreManagerFactory
    {
        private const string SystemFabricBackupRestoreAssemblyName = "System.Fabric.BackupRestore";

        // we should only load the DLLs that we want to load dynamically and not anything else
        private static readonly string[] KnownDlls =
        {
            SystemFabricBackupRestoreAssemblyName,
        };

        static BackupRestoreManagerFactory()
        {
            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(LoadFromFabricCodePath);
        }

        static Assembly LoadFromFabricCodePath(object sender, ResolveEventArgs args)
        {
            try
            {
                var assemblyToLoad = new AssemblyName(args.Name);

                if (KnownDlls.Contains(assemblyToLoad.Name))
                {
                    var folderPath = FabricEnvironment.GetCodePath();
#if DotNetCoreClr
                    // .NetStandard compatible Data.Impl and its dependencies are located in "NS' subfolder under Fabric.Code folder.
                    folderPath = Path.Combine(folderPath, "NS");
#endif
                    var assemblyPath = Path.Combine(folderPath, assemblyToLoad.Name + ".dll");

                    if (File.Exists(assemblyPath))
                    {
                        return Assembly.LoadFrom(assemblyPath);
                    }
                }
            }
            catch (Exception)
            {
                // Supress any Exception so that we can continue to
                // load the assembly through other means
            }

            return null;
        }

        internal static IBackupRestoreManager GetBackupRestoreManager(IBackupRestoreReplica replica)
        {
            var currentAssembly = typeof(BackupRestoreManagerFactory).GetTypeInfo().Assembly;

            var systemFabricBackupRestoreAssembly = new AssemblyName
            {
                Name = SystemFabricBackupRestoreAssemblyName,
                Version = currentAssembly.GetName().Version,
#if !DotNetCoreClr
                CultureInfo = currentAssembly.GetName().CultureInfo,
#endif
                ProcessorArchitecture = currentAssembly.GetName().ProcessorArchitecture
            };

            systemFabricBackupRestoreAssembly.SetPublicKeyToken(currentAssembly.GetName().GetPublicKeyToken());

            var backupRestoreManagerTypeName = Helpers.CreateQualifiedNameForAssembly(
                systemFabricBackupRestoreAssembly.FullName,
                "System.Fabric.BackupRestore.BackupRestoreManager");

            var backupRestoreManagerType = Type.GetType(backupRestoreManagerTypeName, true);
            var backupRestoreManager = (IBackupRestoreManager)Activator.CreateInstance(backupRestoreManagerType, replica);

            return backupRestoreManager;
        }
    }
}