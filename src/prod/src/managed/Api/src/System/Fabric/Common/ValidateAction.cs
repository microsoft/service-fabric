// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Reflection;

    internal abstract class ValidateAction : FabricTestAction
    {
        private static readonly HashSet<string> ResolvableAssemblyNames;
        private static Dictionary<string, Assembly> CachedAssemblies;

        protected ValidateAction()
            : base()
        {
        }

        static ValidateAction()
        {
            ResolvableAssemblyNames = new HashSet<string>();
            ResolvableAssemblyNames.Add(Constants.FabricManagementServiceModelAssemblyName);

            CachedAssemblies = new Dictionary<string, Assembly>();

            AppDomain.CurrentDomain.AssemblyResolve += LoadFromFabricCodePath;
        }

        public TimeSpan MaximumStabilizationTimeout { get; set; }

        public ValidationCheckFlag CheckFlag { get; set; }

        private static Assembly LoadFromFabricCodePath(object sender, ResolveEventArgs args)
        {
            string assemblyName = new AssemblyName(args.Name).Name;

            if (!ResolvableAssemblyNames.Contains(assemblyName))
            {
                return null;
            }

            if (CachedAssemblies.ContainsKey(assemblyName))
            {
                return CachedAssemblies[assemblyName];
            }

            try
            {
                string folderPath = FabricEnvironment.GetCodePath();
                string assemblyPath = Path.Combine(folderPath, assemblyName + ".dll");
                if (File.Exists(assemblyPath))
                {
                    CachedAssemblies[assemblyName] = Assembly.LoadFrom(assemblyPath);
                    return CachedAssemblies[assemblyName];
                }
            }
            catch (Exception)
            {
                // Supress any Exception so that we can continue to
                // load the assembly through other means
            }

            return null;
        }
    }
}