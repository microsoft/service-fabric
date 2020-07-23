// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Host
{
    using System.IO;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Threading;

    public class ManagedHost
    {
        private static string productBinariesPath;

        public static int Setup(string[] args)
        {
            if (args.Length < 2)
            {
                return -1;
            }

            ManagedHost.productBinariesPath = args[0];
            AppDomain.CurrentDomain.AssemblyResolve += CustomResolver;

            return 0;
        }

        public static int Run(string[] args)
        {
            using (var fabricRuntime = FabricRuntime.Create())
            {
                // Load the specific type that implements FabricWorkerEntryPoint
                var entryPointType = Type.GetType(args[1]);
                var entryPoint = (FabricWorkerEntryPoint) Activator.CreateInstance(entryPointType);

                entryPoint.InvokeActivate(fabricRuntime, fabricRuntime.CodePackageActivationContext);

                Thread.Sleep(Timeout.Infinite);
            }

            return 0;
        }

        private static Assembly CustomResolver(object sender, ResolveEventArgs args)
        {
            //This handler is called only when the common language runtime tries to bind to the assembly and fails.

            //Retrieve the list of referenced assemblies in an array of AssemblyName.
            Assembly myAssembly, executingAssembly;
            string tempAssemblyPath = "";

            executingAssembly = Assembly.GetExecutingAssembly();
            AssemblyName[] arrReferencedAssmbNames = executingAssembly.GetReferencedAssemblies();

            //Loop through the array of referenced assembly names.
            foreach (AssemblyName strAssmbName in arrReferencedAssmbNames)
            {
                //Check for the assembly names that have raised the "AssemblyResolve" event.
                if (strAssmbName.FullName.Substring(0, strAssmbName.FullName.IndexOf(",")) == args.Name.Substring(0, args.Name.IndexOf(",")))
                {
                    //Build the path of the assembly from where it has to be loaded.                
                    tempAssemblyPath = Path.Combine(productBinariesPath, args.Name.Substring(0, args.Name.IndexOf(",")) + ".dll");
                    break;
                }

            }

            //Load the assembly from the specified path.                    
            myAssembly = Assembly.LoadFrom(tempAssemblyPath);

            //Return the loaded assembly.
            return myAssembly;
        }
    }
}