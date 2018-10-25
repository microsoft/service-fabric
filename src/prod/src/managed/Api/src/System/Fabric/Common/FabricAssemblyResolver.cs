// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.IO;
    using System.Linq;
    using System.Reflection;
    
    internal class FabricAssemblyResolver
    {
        public const string DataImplAssemblyName = "Microsoft.ServiceFabric.Data.Impl.dll";
        
        public static readonly string[] KnownDlls = { DataImplAssemblyName };

        public static Assembly ResolveAssembly(string assemblyName)
        {
            if (!KnownDlls.Contains(assemblyName))
            {
                throw new ArgumentException("Unknown fabric assembly name: " + assemblyName, "assemblyName");
            }

            string assemblyPath = Path.Combine(FabricEnvironment.GetCodePath(), assemblyName);
            return Assembly.LoadFrom(assemblyPath);
        }
    }
}