// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Stubs
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Base stub class
    /// Derive from this when creating a custom stub
    /// </summary>
    class FabricRuntimeStubBase : NativeRuntime.IFabricRuntime
    {

        #region IFabricRuntime Members

        public virtual NativeCommon.IFabricAsyncOperationContext BeginRegisterServiceGroupFactory(IntPtr groupServiceType, NativeRuntime.IFabricServiceGroupFactory factory, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            throw new NotImplementedException();
        }

        public virtual NativeCommon.IFabricAsyncOperationContext BeginRegisterStatefulServiceFactory(IntPtr serviceType, NativeRuntime.IFabricStatefulServiceFactory factory, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            throw new NotImplementedException();
        }

        public virtual NativeCommon.IFabricAsyncOperationContext BeginRegisterStatelessServiceFactory(IntPtr serviceType, NativeRuntime.IFabricStatelessServiceFactory factory, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            throw new NotImplementedException();
        }

        public virtual NativeRuntime.IFabricServiceGroupFactoryBuilder CreateServiceGroupFactoryBuilder()
        {
            throw new NotImplementedException();
        }

        public virtual void EndRegisterServiceGroupFactory(NativeCommon.IFabricAsyncOperationContext context)
        {
            throw new NotImplementedException();
        }

        public virtual void EndRegisterStatefulServiceFactory(NativeCommon.IFabricAsyncOperationContext context)
        {
            throw new NotImplementedException();
        }

        public virtual void EndRegisterStatelessServiceFactory(NativeCommon.IFabricAsyncOperationContext context)
        {
            throw new NotImplementedException();
        }

        public virtual void RegisterServiceGroupFactory(IntPtr groupServiceType, NativeRuntime.IFabricServiceGroupFactory factory)
        {
            throw new NotImplementedException();
        }

        public virtual void RegisterStatefulServiceFactory(IntPtr serviceType, NativeRuntime.IFabricStatefulServiceFactory factory)
        {
            throw new NotImplementedException();
        }

        public virtual void RegisterStatelessServiceFactory(IntPtr serviceType, NativeRuntime.IFabricStatelessServiceFactory factory)
        {
            throw new NotImplementedException();
        }

        #endregion
    }

    /// <summary>
    /// Stub that supports register/unregister service factory
    /// </summary>
    class FabricRuntimeStubSupportingFactoryRegistration : FabricRuntimeStubBase
    {
        public List<RegisterParameters> ServiceFactoryRegistrations { get; private set; }
        public List<Tuple<string, uint>> ServiceFactoryUnregistrations { get; private set; }
        public bool WasLastCallSynchronous { get; set; }

        public ManualResetEventSlim AsyncEvent { get; private set; }

        public FabricRuntimeStubSupportingFactoryRegistration()
        {
            this.ServiceFactoryRegistrations = new List<RegisterParameters>();
            this.ServiceFactoryUnregistrations = new List<Tuple<string, uint>>();
            this.WasLastCallSynchronous = false;
            this.AsyncEvent = new ManualResetEventSlim();
            this.AsyncEvent.Reset();
        }
        
        public override void RegisterStatefulServiceFactory(IntPtr serviceType, NativeRuntime.IFabricStatefulServiceFactory factory)
        {
            this.ServiceFactoryRegistrations.Add(new RegisterParameters(serviceType, true, factory as ServiceFactoryBroker));
            this.WasLastCallSynchronous = true;
        }

        public override void RegisterStatelessServiceFactory(IntPtr serviceType, NativeRuntime.IFabricStatelessServiceFactory factory)
        {
            this.ServiceFactoryRegistrations.Add(new RegisterParameters(serviceType, false, factory as ServiceFactoryBroker));
            this.WasLastCallSynchronous = true;
        }

        public override NativeCommon.IFabricAsyncOperationContext BeginRegisterStatefulServiceFactory(IntPtr serviceType, NativeRuntime.IFabricStatefulServiceFactory factory, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            this.ServiceFactoryRegistrations.Add(new RegisterParameters(serviceType, true, factory as ServiceFactoryBroker, timeoutMilliseconds));
            this.WasLastCallSynchronous = false;

            AsyncTaskCallInAdapter adapter = new AsyncTaskCallInAdapter(callback, Task.Factory.StartNew(() => this.AsyncEvent.Wait()), InteropApi.Default);
            return adapter;
        }

        public override void EndRegisterStatefulServiceFactory(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public override NativeCommon.IFabricAsyncOperationContext BeginRegisterStatelessServiceFactory(IntPtr serviceType, NativeRuntime.IFabricStatelessServiceFactory factory, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            this.ServiceFactoryRegistrations.Add(new RegisterParameters(serviceType, false, factory as ServiceFactoryBroker, timeoutMilliseconds));
            this.WasLastCallSynchronous = false;

            AsyncTaskCallInAdapter adapter = new AsyncTaskCallInAdapter(callback, Task.Factory.StartNew(() => this.AsyncEvent.Wait()), InteropApi.Default);
            return adapter;
        }

        public override void EndRegisterStatelessServiceFactory(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public class RegisterParameters
        {
            public string ServiceType { get; private set; }
            public bool IsStateful { get; set; }
            public ServiceFactoryBroker ServiceFactoryBroker { get; private set; }
            public uint Timeout { get; private set; }

            public RegisterParameters(IntPtr type, bool isStateful, ServiceFactoryBroker factoryBroker)
                : this(type, isStateful, factoryBroker, 0)
            {
            }

            public RegisterParameters(IntPtr type, bool isStateful, ServiceFactoryBroker factoryBroker, uint timeout)
            {                
                this.ServiceType = NativeTypes.FromNativeString(type);
                this.ServiceFactoryBroker = factoryBroker;
                this.IsStateful = isStateful;
                this.Timeout = timeout;
            }
        }
    }

    /// <summary>
    /// Stub that throws a specific exception on Register
    /// </summary>
    class FabricRuntimeFailingRegisterStub : FabricRuntimeStubBase
    {
        public override void RegisterStatelessServiceFactory(IntPtr serviceType, NativeRuntime.IFabricStatelessServiceFactory factory)
        {
            throw new ApplicationException("some exception");
        }
    }

}