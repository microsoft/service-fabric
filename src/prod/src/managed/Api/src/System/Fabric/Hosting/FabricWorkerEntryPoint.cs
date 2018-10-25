// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Reflection;

    /// <summary>
    /// <para>Reserved for future use.</para>
    /// </summary>
    public abstract class FabricWorkerEntryPoint
    {
        private DllHostHostedManagedDllDescription assemblyDescription;
        private FabricRuntime runtime;
        private CodePackageActivationContext activationContext;

        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        protected FabricWorkerEntryPoint()
        {
        }

        internal static FabricWorkerEntryPoint CreateFromAssemblyDescription(DllHostHostedDllDescription hostedDllDescription)
        {
            Debug.Assert(hostedDllDescription.Kind == DllHostHostedDllKind.Managed, "Kind must be ManagedAssembly");

            var assemblyDescription = (DllHostHostedManagedDllDescription)hostedDllDescription;
            FabricWorkerEntryPoint entryPoint = FabricWorkerEntryPoint.CreateFromAssemblyName(assemblyDescription.AssemblyName);
            entryPoint.assemblyDescription = assemblyDescription;

            return entryPoint;
        }

        internal void InvokeActivate(FabricRuntime runtime, CodePackageActivationContext activationContext)
        {
            Debug.Assert(this.runtime == null, "Activate has already been called.");
            Debug.Assert(this.activationContext == null, "Activate has already been called.");

            this.runtime = runtime;
            this.activationContext = activationContext;

            try
            {
                this.Activate(this.runtime, this.activationContext);
            }
            catch (Exception e)
            {
                AppTrace.TraceSource.WriteExceptionAsError(
                    "FabricWorkerEntryPoint.Activate",
                    e,
                    "Failed to activate entrypoint {0} from assembly {1}",
                    this.GetType(),
                    this.assemblyDescription);
                throw;
            }
        }

        internal void InvokeDeactivate()
        {
            Debug.Assert(this.runtime != null, "Activate has not been called.");
            Debug.Assert(this.activationContext != null, "Activate has not been called.");

            try
            {
                this.Deactivate();
            }
            catch (Exception e)
            {
                AppTrace.TraceSource.WriteExceptionAsError(
                    "FabricWorkerEntryPoint.Deactivate",
                    e,
                    "Failed to deactivate entrypoint {0} from assembly {1}",
                    this.GetType(),
                    this.assemblyDescription);
                throw;
            }
        }

        /// <summary>
        /// <para> Reserved for future use.</para>
        /// </summary>
        /// <param name="runtime">
        /// <para>Reserved for future use.</para>
        /// </param>
        /// <param name="activationContext">
        /// <para>Reserved for future use.</para>
        /// </param>
        protected abstract void Activate(FabricRuntime runtime, CodePackageActivationContext activationContext);

        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        protected virtual void Deactivate()
        {
            // nothing
        }

        private static FabricWorkerEntryPoint CreateFromAssemblyName(string assemblyName)
        {
            Assembly assembly = null;
            try
            {
                assembly = Assembly.Load(new AssemblyName(assemblyName));
            }
            catch (Exception e)
            {
                AppTrace.TraceSource.WriteExceptionAsError("FabricWorkerEntryPoint.CreateFromAssemblyName", e, "Failed to load assembly {0}", assemblyName);
                throw;
            }

            Type entryPointType = null;
            try
            {
                entryPointType = FabricWorkerEntryPoint.GetEntryPointTypeFromAssembly(assembly);
            }
            catch (Exception e)
            {
                AppTrace.TraceSource.WriteExceptionAsError("FabricWorkerEntryPoint.CreateFromAssemblyName", e, "Did not find FabricWorkerEntryPoint in assembly {0}", assemblyName);
                throw;
            }

            try
            {
                return (FabricWorkerEntryPoint)Activator.CreateInstance(entryPointType);
            }
            catch (Exception e)
            {
                AppTrace.TraceSource.WriteExceptionAsError("FabricWorkerEntryPoint.CreateFromAssemblyName", e, "Did not find FabricWorkerEntryPoint in assembly {0}", assemblyName);
                throw;
            }
        }

        private static Type GetEntryPointTypeFromAssembly(Assembly assembly)
        {
            System.Collections.Generic.IEnumerable<TypeInfo> allDefinedTypes = assembly.DefinedTypes;
            Type[] allTypes = new Type[allDefinedTypes.Count()];
            allTypes = allDefinedTypes.Cast<TypeInfo>().ToArray();


            Type[] entryPointTypes = allTypes.Where(type => typeof(FabricWorkerEntryPoint).IsAssignableFrom(type) && type.GetTypeInfo().IsPublic).ToArray();

            if (entryPointTypes.Length == 0)
            {
                AppTrace.TraceSource.WriteError("FabricWorkerEntryPoint.GetEntryPointTypeFromAssembly", "Did not find FabricWorkerEntryPoint in assembly {0}", assembly);
                throw new InvalidOperationException(StringResources.Error_FabricWorkerEntryPoint_No_EntryPoint);
            }

            if (entryPointTypes.Length > 1)
            {
                AppTrace.TraceSource.WriteError("FabricWorkerEntryPoint.GetEntryPointTypeFromAssembly", "Found more than one FabricWorkerEntryPoint in assembly {0}", assembly);
                throw new InvalidOperationException(StringResources.Error_FabricWorkerEntryPoint_Extra_EntryPoint);
            }

            return entryPointTypes[0];
        }
    }
}