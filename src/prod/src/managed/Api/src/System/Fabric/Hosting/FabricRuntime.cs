// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Reflection;

    /// <summary>
    /// <para>Allows user created hosts to obtain their <see cref="System.Fabric.CodePackageActivationContext" />, well as to register the 
    /// necessary service factories [ <see cref="System.Fabric.IStatelessServiceFactory" />, <see cref="System.Fabric.IStatefulServiceFactory" />, or 
    /// <see cref="System.Fabric.ServiceGroupFactory" />] or service types directly.</para>
    /// </summary>
    public sealed class FabricRuntime : IDisposable
    {
        private static readonly TimeSpan DefaultFabricRuntimeCreationTimeout = TimeSpan.FromMilliseconds(10000);

        private readonly CodePackageActivationContext codePackageActivationContext;
        private readonly Action fabricExitCallback;

        private volatile bool disposed;
        private NativeRuntime.IFabricRuntime nativeRuntime;

        static FabricRuntime()
        {
            FabricRuntime.FabricRuntimeFactory = new FabricRuntime.NativeFabricRuntimeFactory();
        }

        /// <summary>
        /// Called by FabricRuntime.Create
        /// </summary>
        internal FabricRuntime(CodePackageActivationContext codePackageActivationContext, Action fabricExitCallback)
        {
            this.codePackageActivationContext = codePackageActivationContext;
            this.fabricExitCallback = fabricExitCallback;
        }

        // test hook
        internal FabricRuntime(NativeRuntime.IFabricRuntime fabricRuntime, CodePackageActivationContext codePackageActivationContext)
        {
            this.codePackageActivationContext = codePackageActivationContext;
            this.nativeRuntime = fabricRuntime;
        }

        internal static NativeFabricRuntimeFactory FabricRuntimeFactory
        {
            get;
            set;
        }

        // for test access
        internal CodePackageActivationContext CodePackageActivationContext
        {
            get
            {
                return this.codePackageActivationContext;
            }
        }

        internal NativeRuntime.IFabricRuntime NativeRuntimeObject
        {
            get
            {
                this.ThrowIfDisposed();
                return this.nativeRuntime;
            }

            set
            {
                this.ThrowIfDisposed();
                this.nativeRuntime = value;
            }
        }

        /// <summary>
        /// <para>Retrieves the current <see cref="System.Fabric.FabricRuntime" />’s <see cref="System.Fabric.CodePackageActivationContext" />.</para>
        /// </summary>
        /// <returns>
        /// <para>The activation context.</para>
        /// </returns>
        public static CodePackageActivationContext GetActivationContext()
        {
            //AppTrace.TraceSource.WriteInfo("FabricRuntime.GetActivationContext");

            return FabricRuntime.FabricRuntimeFactory.GetActivationContext();
        }

        /// <summary>
        /// <para>Retrieves the current <see cref="System.Fabric.FabricRuntime" />’s <see cref="System.Fabric.CodePackageActivationContext" /> asynchronously 
        /// with the specified <paramref name="timeout" /> and <paramref name="cancellationToken" />.</para>
        /// </summary>
        /// <param name="timeout">
        /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a TimeoutException</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.  It can be used to send a notification that 
        /// the operation should be canceled.  Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>The task representing the asynchronous operation.</para>
        /// </returns>
        public static Task<CodePackageActivationContext> GetActivationContextAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            //AppTrace.TraceSource.WriteInfo("FabricRuntime.GetActivationContextAsync");

            return FabricRuntime.FabricRuntimeFactory.GetActivationContextAsync(timeout, cancellationToken);
        }

        /// <summary>
        /// <para>Gets the Node Context object that contains information about Fabric Node. </para>
        /// </summary>
        /// <returns>
        /// <para>The node context.</para>
        /// </returns>
        public static NodeContext GetNodeContext()
        {
            //AppTrace.TraceSource.WriteInfo("FabricRuntime.GetNodeContext");

            return FabricRuntime.FabricRuntimeFactory.GetNodeContext();
        }

        /// <summary>
        /// <para>Gets Node Context from Fabric Node asynchronously with timeout and cancellation token.</para>
        /// </summary>
        /// <param name="timeout">
        /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a TimeoutException</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to send a notification that 
        /// the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>The task representing the asynchronous operation.</para>
        /// </returns>
        public static Task<NodeContext> GetNodeContextAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            //AppTrace.TraceSource.WriteInfo("FabricRuntime.GetNodeContextAsync");

            return FabricRuntime.FabricRuntimeFactory.GetNodeContextAsync(timeout, cancellationToken);
        }

        /// <summary>
        /// <para>Creates the <see cref="System.Fabric.FabricRuntime" /> object.</para>
        /// </summary>
        /// <returns>
        /// <para>A newly created <see cref="System.Fabric.FabricRuntime" /> object.</para>
        /// </returns>
        public static FabricRuntime Create()
        {
            return FabricRuntime.Create(null);
        }

        /// <summary>
        /// <para>Creates the <see cref="System.Fabric.FabricRuntime" /> object with a specified callback function which will be executed if the 
        /// underlying runtime terminates or exits for any reason.</para>
        /// </summary>
        /// <param name="fabricExitCallback">
        /// <para>The Action to be executed when the runtime exits or terminates.</para>
        /// </param>
        /// <returns>
        /// <para>A newly created <see cref="System.Fabric.FabricRuntime" />object.</para>
        /// </returns>
        public static FabricRuntime Create(Action fabricExitCallback)
        {
            //AppTrace.TraceSource.WriteInfo("FabricRuntime.Create");

            FabricRuntime fabricRuntime = null;
            Task<FabricRuntime> task = null;

            try
            {
                task = FabricRuntime.CreateAsyncHelper(fabricExitCallback, FabricRuntime.DefaultFabricRuntimeCreationTimeout, CancellationToken.None);
                task.Wait();
                fabricRuntime = task.Result;
            }
            catch (AggregateException ex)
            {
                throw ex.InnerException;
            }

            return fabricRuntime;
        }

        /// <summary>
        /// <para>Creates the <see cref="System.Fabric.FabricRuntime" /> object asynchronously with the specified <paramref name="timeout" /> and 
        /// <paramref name="cancellationToken" />.</para>
        /// </summary>
        /// <param name="timeout">
        /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a TimeoutException.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.  It can be used to send a notification that 
        /// the operation should be canceled.  Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>The task representing the asynchronous operation.</para>
        /// </returns>
        public static Task<FabricRuntime> CreateAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return FabricRuntime.CreateAsyncHelper(null, timeout, cancellationToken);
        }

        /// <summary>
        /// <para>Creates the <see cref="System.Fabric.FabricRuntime" /> object asynchronously with the specified callback function which will be executed 
        /// if the underlying runtime terminates or exits for any reason, <paramref name="timeout" />, and <paramref name="cancellationToken" />. </para>
        /// </summary>
        /// <param name="fabricExitCallback">
        /// <para>The Action to be executed when the runtime exits or terminates.</para>
        /// </param>
        /// <param name="timeout">
        /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a TimeoutException.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.  It can be used to send a notification that the 
        /// operation should be canceled.  Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>The task representing the asynchronous operation.</para>
        /// </returns>
        public static Task<FabricRuntime> CreateAsync(Action fabricExitCallback, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //AppTrace.TraceSource.WriteInfo("FabricRuntime.CreateAsync");
            return FabricRuntime.CreateAsyncHelper(fabricExitCallback, timeout, cancellationToken);
        }

        /// <summary>
        /// <para>Associates the specified <paramref name="serviceTypeName" /> with the actual managed Type that implements it. </para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The type name of the service type (as a string).  This should match the type of the service group type as specified in the manifests and/or 
        /// the CreateService command.</para>
        /// </param>
        /// <param name="serviceTypeImplementation">
        /// <para>The qualified service Type that implements the specified <paramref name="serviceTypeName" />.</para>
        /// </param>
        /// <remarks>
        /// <para>Note that this mechanism for service type registration does not require a custom <see cref="System.Fabric.IStatelessServiceFactory" /> or 
        /// <see cref="System.Fabric.IStatefulServiceFactory" /> to be provided at registration time.  Service Fabric will generate one at runtime and utilize 
        /// it automatically.  If there is a need for a custom implementation of the factory, you can implement <see cref="System.Fabric.IStatelessServiceFactory" /> 
        /// or <see cref="System.Fabric.IStatefulServiceFactory" /> and then provide those via the corresponding factory registration methods 
        /// (<see cref="System.Fabric.FabricRuntime.RegisterStatelessServiceFactoryAsync" /> or <see cref="System.Fabric.FabricRuntime.RegisterStatefulServiceFactoryAsync" />)</para>
        /// </remarks>
        public void RegisterServiceType(string serviceTypeName, Type serviceTypeImplementation)
        {
            this.ThrowIfDisposed();

            bool isStateful = FabricRuntime.ValidateParametersForRegisterServiceType(serviceTypeName, serviceTypeImplementation);

            Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterServiceTypeHelper(isStateful, serviceTypeName, serviceTypeImplementation), "FabricRuntime.RegisterServiceType");
        }

        /// <summary>
        /// <para>Registers the specified <see cref="System.Fabric.IStatelessServiceFactory" /> for the specified service type.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The type name of the service type (as a string).  This should match the type of the service group type as specified in the manifests and/or the 
        /// CreateService command.</para>
        /// </param>
        /// <param name="factory">
        /// <para>The <see cref="System.Fabric.IStatelessServiceFactory" /> which can create the specified service type.</para>
        /// </param>
        public void RegisterStatelessServiceFactory(string serviceTypeName, IStatelessServiceFactory factory)
        {
            this.ThrowIfDisposed();

            FabricRuntime.ValidateParametersForRegisterServiceFactory(serviceTypeName, factory);

            Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterServiceFactoryHelper(false, serviceTypeName, factory), "FabricRuntime.RegisterStatelessServiceFactory");
        }

        /// <summary>
        /// <para>Registers the specified <see cref="System.Fabric.IStatefulServiceFactory" /> for the specified service type.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The type name of the service type (as a string).  This should match the type of the service group type as specified in the manifests 
        /// and/or the CreateService command.</para>
        /// </param>
        /// <param name="factory">
        /// <para>The <see cref="System.Fabric.IStatefulServiceFactory" /> which can create the specified service type.</para>
        /// </param>
        public void RegisterStatefulServiceFactory(string serviceTypeName, IStatefulServiceFactory factory)
        {
            this.ThrowIfDisposed();

            FabricRuntime.ValidateParametersForRegisterServiceFactory(serviceTypeName, factory);

            Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterServiceFactoryHelper(true, serviceTypeName, factory), "FabricRuntime.RegisterStatefulServiceFactory");
        }

        /// <summary>
        /// <para>Registers the specified <see cref="System.Fabric.ServiceGroupFactory" /> for the specified type.</para>
        /// </summary>
        /// <param name="serviceGroupTypeName">
        /// <para>The type name of the ServiceGroup service type (as a string).  This should match the type of the service group type as specified in the 
        /// manifests and/or the CreateServiceGroup command.</para>
        /// </param>
        /// <param name="factory">
        /// <para>The <see cref="System.Fabric.ServiceGroupFactory" /> which can create the specified service group type.</para>
        /// </param>
        public void RegisterServiceGroupFactory(string serviceGroupTypeName, ServiceGroupFactory factory)
        {
            this.ThrowIfDisposed();

            FabricRuntime.ValidateParametersForRegisterServiceGroupFactory(serviceGroupTypeName, factory);

            Utility.WrapNativeSyncInvokeInMTA(() => this.InternalRegisterServiceGroupFactory(serviceGroupTypeName, factory), "FabricRuntime.RegisterServiceGroupFactory");
        }

        /// <summary>
        /// <para>Asynchronously associates the specified serviceTypeName with the actual managed Type that implements it, with the specified <paramref name="timeout" /> 
        /// and <paramref name="cancellationToken" /></para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The type name of the service type (as a string).  This should match the type of the service group type as specified in the manifests 
        /// and/or the CreateService command.</para>
        /// </param>
        /// <param name="serviceTypeImplementation">
        /// <para>The qualified service Type that implements the specified <paramref name="serviceTypeName" />.</para>
        /// </param>
        /// <param name="timeout">
        /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a TimeoutException.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.  It can be used to send a notification that the 
        /// operation should be canceled.  Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>The task representing the asynchronous operation.</para>
        /// </returns>
        public Task RegisterServiceTypeAsync(string serviceTypeName, Type serviceTypeImplementation, TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.ThrowIfDisposed();

            bool isStateful = FabricRuntime.ValidateParametersForRegisterServiceType(serviceTypeName, serviceTypeImplementation);

            ServiceFactoryBroker factory = new ServiceFactoryBroker(new DefaultServiceFactory(serviceTypeImplementation), this.codePackageActivationContext);
            return this.RegisterServiceFactoryAsyncHelper(isStateful, serviceTypeName, factory, (uint)timeout.TotalMilliseconds, cancellationToken);
        }

        /// <summary>
        /// <para>Asynchronously registers the specified <see cref="System.Fabric.IStatelessServiceFactory" /> for the specified service type, with the 
        /// specified <paramref name="timeout" /> and <paramref name="cancellationToken" /></para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The type name of the service type (as a string).  This should match the type of the service group type as specified in the manifests 
        /// and/or the CreateService command.</para>
        /// </param>
        /// <param name="factory">
        /// <para>The <see cref="System.Fabric.IStatelessServiceFactory" /> which can create the specified service type.</para>
        /// </param>
        /// <param name="timeout">
        /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a TimeoutException.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.  It can be used to send a notification that 
        /// the operation should be canceled.  Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>The task representing the asynchronous operation.</para>
        /// </returns>
        public Task RegisterStatelessServiceFactoryAsync(string serviceTypeName, IStatelessServiceFactory factory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.ThrowIfDisposed();

            FabricRuntime.ValidateParametersForRegisterServiceFactory(serviceTypeName, factory);

            ServiceFactoryBroker broker = new ServiceFactoryBroker(factory, this.codePackageActivationContext);
            return this.RegisterServiceFactoryAsyncHelper(false, serviceTypeName, broker, (uint)timeout.TotalMilliseconds, cancellationToken);
        }

        /// <summary>
        /// <para>Registers the specified <see cref="System.Fabric.IStatefulServiceFactory" /> for the specified service type with the specified 
        /// <paramref name="timeout" /> and <paramref name="cancellationToken" />.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The type name of the service type (as a string).  This should match the type of the service group type as specified in the manifests 
        /// and/or the CreateService command.</para>
        /// </param>
        /// <param name="factory">
        /// <para>The <see cref="System.Fabric.IStatefulServiceFactory" /> which can create the specified service type.</para>
        /// </param>
        /// <param name="timeout">
        /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a TimeoutException.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.  It can be used to send a notification that 
        /// the operation should be canceled.  Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>The representing the asynchronous operation.</para>
        /// </returns>
        public Task RegisterStatefulServiceFactoryAsync(string serviceTypeName, IStatefulServiceFactory factory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.ThrowIfDisposed();

            FabricRuntime.ValidateParametersForRegisterServiceFactory(serviceTypeName, factory);

            ServiceFactoryBroker broker = new ServiceFactoryBroker(factory, this.codePackageActivationContext);
            return this.RegisterServiceFactoryAsyncHelper(true, serviceTypeName, broker, (uint)timeout.TotalMilliseconds, cancellationToken);
        }

        /// <summary>
        /// <para>Asynchronously registers the specified <see cref="System.Fabric.ServiceGroupFactory" /> for the specified service group type with the 
        /// specified <paramref name="timeout" /> and <paramref name="cancellationToken" />.</para>
        /// </summary>
        /// <param name="serviceGroupTypeName">
        /// <para>The type name of the ServiceGroup service type (as a string).  This should match the type of the service group type as specified in 
        /// the manifests and/or the CreateServiceGroup command.</para>
        /// </param>
        /// <param name="factory">
        /// <para>The <see cref="System.Fabric.ServiceGroupFactory" /> which can create the specified service group type.</para>
        /// </param>
        /// <param name="timeout">
        /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a TimeoutException.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to send a notification that the 
        /// operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>The task representing the asynchronous operation.</para>
        /// </returns>
        public Task RegisterServiceGroupFactoryAsync(string serviceGroupTypeName, ServiceGroupFactory factory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.ThrowIfDisposed();

            FabricRuntime.ValidateParametersForRegisterServiceGroupFactory(serviceGroupTypeName, factory);

            return this.RegisterServiceGroupFactoryAsyncHelper(serviceGroupTypeName, factory, (uint)timeout.TotalMilliseconds, cancellationToken);
        }

        /// <summary>
        /// <para>Disposes of the <see cref="System.Fabric.FabricRuntime" />.</para>
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private static Task<FabricRuntime> CreateAsyncHelper(Action fabricExitCallback, TimeSpan timeout, CancellationToken cancellationToken)
        {
            CodePackageActivationContext activationContext = FabricRuntime.GetCodePackageActivationContextForRuntimeCreation();
            AppTrace.TraceSource.WriteNoise("FabricRuntime.CreateAsync", "Got activation context {0}", activationContext == null);

            FabricRuntime runtime = new FabricRuntime(activationContext, fabricExitCallback);
            Guid fabricRuntimeGuid = typeof(NativeRuntime.IFabricRuntime).GetTypeInfo().GUID;
            FabricExitNotificationSink sink = new FabricExitNotificationSink(runtime.OnFabricExited);

            return FabricRuntime.FabricRuntimeFactory
                .InitializeFabricRuntimeAsync(runtime, fabricRuntimeGuid, sink, (uint)timeout.TotalMilliseconds);
        }

        private static CodePackageActivationContext GetCodePackageActivationContextForRuntimeCreation()
        {
            CodePackageActivationContext activationContext = null;

            try
            {
                activationContext = FabricRuntime.GetActivationContext();
            }
            catch (InvalidOperationException)
            {
                AppTrace.TraceSource.WriteInfo("FabricRuntime.GetCodePackageActivationContextForRuntimeCreation", "Failed to get the activation context because we are not a normal hosted app");
            }

            return activationContext;
        }

        private static void ValidateParametersForRegisterServiceGroupFactory(string serviceGroupTypeName, ServiceGroupFactory serviceGroupFactory)
        {
            Requires.Argument<string>("serviceGroupTypeName", serviceGroupTypeName).NotNullOrWhiteSpace();
            Requires.Argument<ServiceGroupFactory>("serviceGroupFactory", serviceGroupFactory).NotNull();
        }

        private static void ValidateParametersForRegisterServiceFactory(string serviceTypeName, object serviceFactory)
        {
            Requires.Argument<string>("serviceTypeName", serviceTypeName).NotNullOrWhiteSpace();
            Requires.Argument<object>("serviceFactory", serviceFactory).NotNull();
        }

        private static bool ValidateParametersForRegisterServiceType(string serviceTypeName, Type serviceTypeImplementation)
        {
            Requires.Argument<string>("serviceTypeName", serviceTypeName).NotNullOrWhiteSpace();
            Requires.Argument<Type>("serviceTypeImplementation", serviceTypeImplementation).NotNull();

            bool? isStateful = Helpers.IsStatefulService(serviceTypeImplementation);

            if (!isStateful.HasValue)
            {
                AppTrace.TraceSource.WriteError("FabricRuntime.RegisterServiceType", "Service Type Implementation {0} is invalid", serviceTypeImplementation.FullName);
                throw new ArgumentException(StringResources.Error_InvalidServiceTypeImplementation);
            }

            return isStateful.Value;
        }

        private void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (this.nativeRuntime != null)
                {
                    Marshal.FinalReleaseComObject(this.nativeRuntime);
                    this.nativeRuntime = null;
                }

                if (disposing && this.codePackageActivationContext != null)
                {
                    codePackageActivationContext.Dispose();
                }

                this.nativeRuntime = null;

                this.disposed = true;
            }
        }

        private void ThrowIfDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("FabricRuntime");
            }
        }

        private void RegisterServiceTypeHelper(bool isStateful, string serviceTypeName, Type serviceTypeImplementation)
        {
            ServiceFactoryBroker factory = new ServiceFactoryBroker(new DefaultServiceFactory(serviceTypeImplementation), this.codePackageActivationContext);
            this.InternalRegisterServiceFactory(isStateful, serviceTypeName, factory);
        }

        private void RegisterServiceFactoryHelper(bool isStateful, string serviceTypeName, object factory)
        {
            ServiceFactoryBroker broker = new ServiceFactoryBroker(factory, this.codePackageActivationContext);
            this.InternalRegisterServiceFactory(isStateful, serviceTypeName, broker);
        }

        private void InternalRegisterServiceFactory(bool isStateful, string serviceType, ServiceFactoryBroker serviceFactoryBroker)
        {
            //// Calls native code, requires UnmanagedCode permission
            using (var pin = new PinBlittable(serviceType))
            {
                if (!isStateful)
                {
                    this.NativeRuntimeObject.RegisterStatelessServiceFactory(pin.AddrOfPinnedObject(), serviceFactoryBroker);
                }
                else
                {
                    this.NativeRuntimeObject.RegisterStatefulServiceFactory(pin.AddrOfPinnedObject(), serviceFactoryBroker);
                }
            }
        }

        private void InternalRegisterServiceGroupFactory(string serviceGroupType, ServiceGroupFactory serviceGroupFactory)
        {
            //// Calls native code, requires UnmanagedCode permission

            NativeRuntime.IFabricServiceGroupFactory nativeFactory = this.CreateServiceGroupFactoryHelper(serviceGroupFactory);

            using (var pin = new PinBlittable(serviceGroupType))
            {
                this.NativeRuntimeObject.RegisterServiceGroupFactory(pin.AddrOfPinnedObject(), nativeFactory);
            }
        }

        #region Register Wrapper Async

        private Task RegisterServiceFactoryAsyncHelper(bool isStateful, string serviceType, object serviceFactory, uint timeoutMs, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.RegisterServiceFactoryBeginWrapper(isStateful, serviceType, serviceFactory, timeoutMs, callback),
                (callback) => this.RegisterServiceFactoryEndWrapper(isStateful, callback),
                cancellationToken,
                "FabricRuntime.RegisterServiceFactory");
        }

        private NativeCommon.IFabricAsyncOperationContext RegisterServiceFactoryBeginWrapper(bool isStateful, string serviceType, object serviceFactory, uint timeoutMs, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            using (var pin = new PinBlittable(serviceType))
            {
                if (isStateful)
                {
                    return this.NativeRuntimeObject.BeginRegisterStatefulServiceFactory(pin.AddrOfPinnedObject(), serviceFactory as NativeRuntime.IFabricStatefulServiceFactory, timeoutMs, callback);
                }
                else
                {
                    return this.NativeRuntimeObject.BeginRegisterStatelessServiceFactory(pin.AddrOfPinnedObject(), serviceFactory as NativeRuntime.IFabricStatelessServiceFactory, timeoutMs, callback);
                }
            }
        }

        private void RegisterServiceFactoryEndWrapper(bool isStateful, NativeCommon.IFabricAsyncOperationContext context)
        {
            if (isStateful)
            {
                this.NativeRuntimeObject.EndRegisterStatefulServiceFactory(context);
            }
            else
            {
                this.NativeRuntimeObject.EndRegisterStatelessServiceFactory(context);
            }
        }

        private Task RegisterServiceGroupFactoryAsyncHelper(string serviceGroupTypeName, ServiceGroupFactory factory, uint timeoutMs, CancellationToken cancellationToken)
        {
            NativeRuntime.IFabricServiceGroupFactory serviceGroupFactory = this.CreateServiceGroupFactoryHelper(factory);

            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.RegisterServiceGroupFactoryBeginWrapper(serviceGroupTypeName, serviceGroupFactory, timeoutMs, callback),
                this.RegisterServiceGroupFactoryEndWrapper,
                cancellationToken,
                "FabricRuntime.RegisterServiceGroupFactory");
        }

        private NativeRuntime.IFabricServiceGroupFactory CreateServiceGroupFactoryHelper(ServiceGroupFactory factory)
        {
            NativeRuntime.IFabricServiceGroupFactoryBuilder nativeBuilder = this.NativeRuntimeObject.CreateServiceGroupFactoryBuilder();

            foreach (var namedFactory in factory.StatefulServiceFactories)
            {
                ServiceFactoryBroker broker = new ServiceFactoryBroker(namedFactory.Item2, this.codePackageActivationContext);
                using (var pin = new PinBlittable(namedFactory.Item1))
                {
                    nativeBuilder.AddStatefulServiceFactory(pin.AddrOfPinnedObject(), broker as NativeRuntime.IFabricStatefulServiceFactory);
                }
            }

            foreach (var namedFactory in factory.StatelessServiceFactories)
            {
                ServiceFactoryBroker broker = new ServiceFactoryBroker(namedFactory.Item2, this.codePackageActivationContext);
                using (var pin = new PinBlittable(namedFactory.Item1))
                {
                    nativeBuilder.AddStatelessServiceFactory(pin.AddrOfPinnedObject(), broker as NativeRuntime.IFabricStatelessServiceFactory);
                }
            }

            NativeRuntime.IFabricServiceGroupFactory serviceGroupFactory = nativeBuilder.ToServiceGroupFactory();
            return serviceGroupFactory;
        }

        private NativeCommon.IFabricAsyncOperationContext RegisterServiceGroupFactoryBeginWrapper(string serviceGroupTypeName, NativeRuntime.IFabricServiceGroupFactory factory, uint timeoutMs, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            using (var pin = new PinBlittable(serviceGroupTypeName))
            {
                return this.NativeRuntimeObject.BeginRegisterServiceGroupFactory(pin.AddrOfPinnedObject(), factory, timeoutMs, callback);
            }
        }

        private void RegisterServiceGroupFactoryEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.NativeRuntimeObject.EndRegisterServiceGroupFactory(context);
        }

        #endregion

        private void OnFabricExited()
        {
            this.ThrowIfDisposed();

            if (this.fabricExitCallback != null)
            {
                this.fabricExitCallback();
            }
        }

        internal class NativeFabricRuntimeFactory
        {
            // virtual so that tests can extend this
            public virtual Task<FabricRuntime> InitializeFabricRuntimeAsync(
                FabricRuntime runtime,
                Guid fabricRuntimeGuid,
                NativeRuntime.IFabricProcessExitHandler processExitHandler,
                uint timeout)
            {
                return this.InitializeFabricRuntimeAsyncHelper(runtime, fabricRuntimeGuid, processExitHandler, timeout);
            }

            public virtual CodePackageActivationContext GetActivationContext()
            {
                return Utility.WrapNativeSyncInvokeInMTA<CodePackageActivationContext>(this.GetCodePackageActivationContextHelper, "NativeFabricRuntime.GetCodePackageActivationContext");
            }

            public virtual Task<CodePackageActivationContext> GetActivationContextAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<CodePackageActivationContext>(
                   (callback) => this.GetCodePackageActivationContextBeginWrapper(timeout, callback),
                   this.GetCodePackageActivationContextEndWrapper,
                   cancellationToken,
                   "FabricRuntime.GetActivationContextAsync");
            }

            public Task<NodeContext> GetNodeContextAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<NodeContext>(
                  (callback) => this.GetNodeContextBeginWrapper(timeout, callback),
                  this.GetNodeContextEndWrapper,
                  cancellationToken,
                  "FabricRuntime.GetNodeContextAsync");
            }

            #region Initialize Fabric RT Async

            private Task<FabricRuntime> InitializeFabricRuntimeAsyncHelper(FabricRuntime runtime, Guid fabricRuntimeGuid, object exitHandler, uint timeout)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<FabricRuntime>(
                    (callback) => this.InitializeFabricRuntimeBeginWrapper(fabricRuntimeGuid, exitHandler, timeout, callback),
                    (callback) => this.InitializeFabricRuntimeEndWrapper(runtime, callback),
                    CancellationToken.None,
                    "FabricRuntime.InitializeFabricRuntime");
            }

            private NativeCommon.IFabricAsyncOperationContext InitializeFabricRuntimeBeginWrapper(Guid fabricRuntimeGuid, object exitHandler, uint timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                Guid localGuid = fabricRuntimeGuid;

                using (var pin = new PinCollection())
                {
                    return NativeRuntime.FabricBeginCreateRuntime(ref localGuid, exitHandler as NativeRuntime.IFabricProcessExitHandler, timeout, callback);
                }
            }

            private FabricRuntime InitializeFabricRuntimeEndWrapper(FabricRuntime runtime, NativeCommon.IFabricAsyncOperationContext context)
            {
                runtime.nativeRuntime = NativeRuntime.FabricEndCreateRuntime(context);
                return runtime;
            }

            #endregion

            #region GetCodePackageActivationContext Sync

            private CodePackageActivationContext GetCodePackageActivationContextHelper()
            {
                Guid iid = typeof(NativeRuntime.IFabricCodePackageActivationContext6).GetTypeInfo().GUID;
                return CodePackageActivationContext.CreateFromNative(NativeRuntime.FabricGetActivationContext(ref iid));
            }

            #endregion

            #region GetCodePackageActivationContext Async

            private NativeCommon.IFabricAsyncOperationContext GetCodePackageActivationContextBeginWrapper(TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                Guid iid = typeof(NativeRuntime.IFabricCodePackageActivationContext).GetTypeInfo().GUID;
                var timeoutMilliseconds = Utility.ToMilliseconds(timeout, "timeout");

                return NativeRuntime.FabricBeginGetActivationContext(
                    ref iid,
                    timeoutMilliseconds,
                    callback);
            }

            private CodePackageActivationContext GetCodePackageActivationContextEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return CodePackageActivationContext.CreateFromNative(NativeRuntime.FabricEndGetActivationContext(context));
            }

            #endregion

            private unsafe NodeContext GetNodeContextHelper()
            {
                NativeRuntime.IFabricNodeContextResult2 nodeContextResult = NativeRuntime.FabricGetNodeContext();
                return NodeContext.CreateFromNative(nodeContextResult);
            }

            public NodeContext GetNodeContext()
            {
                return Utility.WrapNativeSyncInvokeInMTA<NodeContext>(this.GetNodeContextHelper, "NativeFabricRuntime::GetFabricNodeContext");
            }
            
            #region GetNodeContext Async

            private NativeCommon.IFabricAsyncOperationContext GetNodeContextBeginWrapper(TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                var timeoutMilliseconds = Utility.ToMilliseconds(timeout, "timeout");

                return NativeRuntime.FabricBeginGetNodeContext(
                    timeoutMilliseconds,
                    callback);
            }

            private unsafe NodeContext GetNodeContextEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                NativeRuntime.IFabricNodeContextResult2 nodeContextResult = NativeRuntime.FabricEndGetNodeContext(context);
                return NodeContext.CreateFromNative(nodeContextResult);
            }

            #endregion

            private unsafe string GetCurrentRuntimeVersionHelper()
            {
                NativeCommon.IFabricStringResult result = NativeRuntimeInternal.FabricGetRuntimeDllVersion();
                return StringResult.FromNative(result);
            }

            internal string GetCurrentRuntimeVersion()
            {
                return Utility.WrapNativeSyncInvokeInMTA(this.GetCurrentRuntimeVersionHelper, "NativeFabricRuntimeInternal::FabricGetRuntimeDllVersion");
            }
        }

        // Separate SecurityCritical class that actually implements the native interface
        // This class simply forwards the notification to the FabricRuntime instance
        private sealed class FabricExitNotificationSink : NativeRuntime.IFabricProcessExitHandler
        {
            private Action fabricExitedHandler;

            public FabricExitNotificationSink(Action fabricExitedHandler)
            {
                Requires.Argument<Action>("fabricExitedHandler", fabricExitedHandler).NotNull();
                this.fabricExitedHandler = fabricExitedHandler;
            }

            #region IFabricProcessExitHandler Members

            public void FabricProcessExited()
            {
                this.fabricExitedHandler();
            }

            #endregion
        }
    }
}