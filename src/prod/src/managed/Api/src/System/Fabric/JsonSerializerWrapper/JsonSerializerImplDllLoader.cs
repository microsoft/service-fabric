// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerWrapper
{
    using System.IO;
    using System.Reflection;
    using System.Fabric.Common;
    using System.Linq;

    internal class JsonSerializerImplDllLoader
    {
        public const string JsonSerializerImplDllName = "System.Fabric.JsonSerializerImpl";
        public const string JsonSerializerImplDllFullName = JsonSerializerImplDllName + ".dll";
        public const string JsonSerialierImplTypeName = "JsonSerializerImpl";

        // No throw.
        // Returns an instance of JsonSerializerImpl or null
        public static IJsonSerializer TryGetJsonSerializerImpl()
        {
            IJsonSerializer jsonSerializerImpl = TryLoadLibraryAndCreateJsonSerializerImpl();
            return jsonSerializerImpl;
        }

        private static IJsonSerializer TryLoadLibraryAndCreateJsonSerializerImpl()
        {
            Assembly module = TryLoadDllForJsonSerializerImpl();

            if(module == null)
            {
                return null;
            }

            /// get type information from dll
            Type targetType = module.DefinedTypes.
                FirstOrDefault(t => t.Name == JsonSerialierImplTypeName);

            if(targetType == null)
            {
                return null;
            }

            // Create instance
            var instance = Activator.CreateInstance(targetType, nonPublic: true);

            return (IJsonSerializer) instance;
        }

        /// No throw method.
        /// Will return Assembly if found otherwise null.
        private static Assembly TryLoadDllForJsonSerializerImpl()
        { 
            /// Load assembly from "assemblyDirectory"
            Assembly targetModule = null;
            try
            {
                targetModule = Assembly.Load(new AssemblyName(JsonSerializerImplDllName));
            }
            catch (Exception)
            {
                /// Failed to load module
                targetModule = null;
            }

            return targetModule;
        }
    }
}