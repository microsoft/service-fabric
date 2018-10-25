// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Hosting
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Runtime.InteropServices;

    internal sealed class FabricHostEntryPoint : IFabricHostEntryPoint, IDisposable
    {
        private readonly AssemblyResolver assemblyResolver = new AssemblyResolver();

        private readonly Dictionary<string, ActivatedCodePackage> activeCodePackages = new Dictionary<string, ActivatedCodePackage>();

        private bool disposed;

        #region IFabricHostEntryPoint Members

        public int ActiveCodePackageCount
        {
            get
            {
                this.ThrowIfDisposed();
                return this.activeCodePackages.Count;
            }
        }

        public void Start(string logDirectory, string workDirectory, string appDomainConfigFilePath, string hostEntryPointManagerUniqueId)
        {
            this.ThrowIfDisposed();
            
            AppTrace.TraceSource.WriteInfo("FabricEntryPoint.Start", "Start. LogDirectory {0}. WorkDirectory {1}", logDirectory, workDirectory);
            
            this.assemblyResolver.AddApplicationBinariesPath(workDirectory);
        }

        public void ActivateCodePackage(string activationContextId, string codePackageNameToActivate, IntPtr nativeCodePackageActivationContext, IntPtr nativeFabricRuntime)
        {
            this.ThrowIfDisposed();
            
            // thunk down to ActivateCodePackageInternal
            // this is for testing purposes only
            // all this method does is take the IntPtr codePackageActivationContext and unmarshal it
            AppTrace.TraceSource.WriteInfo("FabricEntryPoint.ActivateCodePackage", "Activating {0} with index {1}", activationContextId, codePackageNameToActivate);

            try
            {
                this.ActivateCodePackageInternal(
                    activationContextId,
                    codePackageNameToActivate,
                    CodePackageActivationContext.CreateFromNative((NativeRuntime.IFabricCodePackageActivationContext6)Marshal.GetObjectForIUnknown(nativeCodePackageActivationContext)),
                    (NativeRuntime.IFabricRuntime)Marshal.GetObjectForIUnknown(nativeFabricRuntime));
            }
            catch (Exception e)
            {
                AppTrace.TraceSource.WriteExceptionAsError("HostEntryPoint.ActivateCodePackage", e);
                throw;
            }
        }

        public void DeactivateCodePackage(string activationContextId)
        {
            this.ThrowIfDisposed();

            AppTrace.TraceSource.WriteInfo("FabricEntryPoint.DeactivateCodePackage", "ActivationContextId: {0}", activationContextId);
            Debug.Assert(!string.IsNullOrEmpty(activationContextId), "activationContextId cannot be null or empty");
            
            if (!this.activeCodePackages.ContainsKey(activationContextId))
            {
                AppTrace.TraceSource.WriteError("FabricEntryPoint.DeactivateCodePackage", "Cannot deactivate code package that was not activated: {0}", activationContextId);
                throw new InvalidOperationException(StringResources.Error_FabricHostEntryPoint_Package_Not_Activated);
            }

            ActivatedCodePackage activatedCodePackage = this.activeCodePackages[activationContextId];

            try
            {
                activatedCodePackage.EntryPoints.ForEach(ep => ep.InvokeDeactivate());
            }
            finally
            {
                this.activeCodePackages.Remove(activationContextId);
                activatedCodePackage.Dispose();
            }
        }

        public void Stop()
        {
            this.Dispose();
        }

        #endregion

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        // internal for test hook
        internal void ActivateCodePackageInternal(
            string activationContextId,
            string codePackageName,
            CodePackageActivationContext activationContext,
            NativeRuntime.IFabricRuntime fabricRuntimePointer)
        {
            this.ThrowIfDisposed();

            CodePackage codePackage;
            try
            {
                codePackage = activationContext.GetCodePackageObject(codePackageName);
            }
            catch (KeyNotFoundException)
            {
                AppTrace.TraceSource.WriteError("FabricHostEntryPoint.ActivateCodePackageInternal", "The code package {0} to activate was not found", codePackageName);
                throw new ArgumentException(StringResources.Error_CodePackageNotFound);
            }

            DllHostEntryPointDescription description = codePackage.Description.EntryPoint as DllHostEntryPointDescription;
            if (description == null)
            {
                AppTrace.TraceSource.WriteError("FabricHostEntryPoint.ActivateCodePackageInternal", "Code package {0} does not have a FabricHostEntryPoint", codePackageName);
                throw new ArgumentException(StringResources.Error_FabricHostEntryPoint_No_EntryPoint_Found);
            }

            AppTrace.TraceSource.WriteNoise("FabricHostEntryPoint.ActivateCodePackageInternal", "Adding assembly resolver path: {0}", codePackage.Path);
            this.assemblyResolver.AddApplicationBinariesPath(codePackage.Path);

            var workerEntryPoints = description.HostedDlls
                .Where(i => i.Kind == DllHostHostedDllKind.Managed)
                .Select(FabricWorkerEntryPoint.CreateFromAssemblyDescription)
                .ToArray();

            var fabricRuntime = new FabricRuntime(fabricRuntimePointer, activationContext);
            ActivatedCodePackage activatedCodePackage = new ActivatedCodePackage(fabricRuntime, workerEntryPoints);

            bool activateSucceeded = false;
            try
            {
                workerEntryPoints.ForEach(wep => wep.InvokeActivate(fabricRuntime, activationContext));

                this.activeCodePackages[activationContextId] = activatedCodePackage;
                activateSucceeded = true;
            }
            finally
            {
                if (!activateSucceeded)
                {
                    activatedCodePackage.Dispose();
                }
            }
        }

        private void ThrowIfDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("FabricHostEntryPoint");
            }
        }

        private void Dispose(bool disposing)
        {
            if (this.disposed && disposing)
            {
                foreach (var codePackage in this.activeCodePackages.Values)
                {
                    codePackage.EntryPoints.ForEach(ep => ep.InvokeDeactivate());
                }

                this.activeCodePackages.Values.ForEach(cp => cp.Dispose());

                this.assemblyResolver.Dispose();

                this.disposed = true;
            }
        }

        private sealed class ActivatedCodePackage : IDisposable
        {
            private readonly FabricWorkerEntryPoint[] entryPoints;
            
            private FabricRuntime runtime;
            private bool disposed = false;

            public ActivatedCodePackage(FabricRuntime runtime, FabricWorkerEntryPoint[] entryPoints)
            {
                this.runtime = runtime;
                this.entryPoints = entryPoints;
            }

            public FabricWorkerEntryPoint[] EntryPoints
            {
                get
                {
                    if (this.disposed)
                    {
                        throw new ObjectDisposedException("ActivatedCodePackage");
                    }

                    return this.entryPoints;
                }
            }

            public void Dispose()
            {
                this.Dispose(true);
                GC.SuppressFinalize(this);
            }

            private void Dispose(bool disposing)
            {
                if (!this.disposed && disposing)
                {
                    var disposable = this.runtime as IDisposable;
                    if (disposable != null)
                    {
                        disposable.Dispose();
                    }

                    this.runtime = null;
                    this.disposed = true;
                }
            }
        }
    }
}