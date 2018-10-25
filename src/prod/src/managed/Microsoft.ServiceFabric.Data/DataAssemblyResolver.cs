// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Linq;
    using System.IO;
    using System.Reflection;
    using System.Fabric.Common;

    internal static class DataAssemblyResolver
    {

        // we should only load the DLLs that we know about and not anything else
        private static readonly string[] KnownDlls =
        {
            "Microsoft.ServiceFabric.Data.Impl",
#if DotNetCoreClrLinux
            // Need to update KnownDlls list in case dependent Dlls change due to change/update for MCG generated interop dll's dependency requirements.
            "Microsoft.ServiceFabric.Data.Impl.McgInterop",
            "System.Private.CompilerServices.ICastable",
            "System.Private.Interop"
#endif
        };

        internal static Assembly OnAssemblyResolve(object sender, ResolveEventArgs args)
        {
            var assemblyName = new AssemblyName(args.Name);
            if (KnownDlls.Contains(assemblyName.Name))
            {
                var assemblyToLoad = assemblyName.Name + ".dll";

                var assemblyPath = FabricEnvironment.GetCodePath();

                if (assemblyPath != null)
                {
#if DotNetCoreClr
                    // .NetStandard compatible Data.Impl and its dependencies are located in "NS' subfolder under Fabric.Code folder.
                    assemblyPath = Path.Combine(assemblyPath, "NS");
#endif
                    // the code below assumes that compatibility exists between the versions
                    // this is like assembly binding redirect
                    return Assembly.LoadFrom(Path.Combine(assemblyPath, assemblyToLoad));
                }
            }

            return null;
        }
    }
}