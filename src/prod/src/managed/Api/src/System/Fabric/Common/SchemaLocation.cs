// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Reflection;

    class SchemaLocation
    {
        private const string WindowsFabricSchemaFileName = "ServiceFabricServiceModel.xsd";

        public static string GetWindowsFabricSchemaLocation()
        {
            string fabricCodePath = null;
            try
            {
                fabricCodePath = System.Fabric.Common.FabricEnvironment.GetCodePath();
            }
            catch (FabricException) { }
            if (fabricCodePath != null)
            {
                string schemaLocation = System.IO.Path.Combine(fabricCodePath, WindowsFabricSchemaFileName);
                if (System.IO.File.Exists(schemaLocation))
                {
                    return schemaLocation;
                }
            }

#if !DotNetCoreClr
            var entryAssembly = System.Reflection.Assembly.GetEntryAssembly();
            if (entryAssembly != null)
            {
                string entryPointLocation = System.IO.Path.GetDirectoryName(entryAssembly.Location);
                if (entryPointLocation != null)
                {
                    string schemaLocation = System.IO.Path.Combine(entryPointLocation, WindowsFabricSchemaFileName);
                    if (System.IO.File.Exists(schemaLocation))
                    {
                        return schemaLocation;
                    }
                }
            }
#endif

            var currentAssembly = typeof(SchemaLocation).GetTypeInfo().Assembly;
            if (currentAssembly != null)
            {
                string currentAssemblyLocation = System.IO.Path.GetDirectoryName(currentAssembly.Location);
                if (currentAssemblyLocation != null)
                {
                    string schemaLocation = System.IO.Path.Combine(currentAssemblyLocation, WindowsFabricSchemaFileName);
                    if (System.IO.File.Exists(schemaLocation))
                    {
                        return schemaLocation;
                    }
                }
            }

            throw new System.IO.FileNotFoundException(WindowsFabricSchemaFileName);
        }
    }
}