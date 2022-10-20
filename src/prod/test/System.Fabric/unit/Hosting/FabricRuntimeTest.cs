// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Hosting
{
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class FabricRuntimeTest
    {
        private const string DefaultServiceName = "moo";

        private static readonly TimeSpan DefaultTimeout = TimeSpan.FromDays(1);
        private static readonly uint TimeoutForCreateSync = 10000;

        private static readonly CodePackageActivationContext DefaultCodePackageActivationContext = new CodePackageActivationContext(new Stubs.CodePackageActivationContextStub());

        [TestInitialize]
        public void TestInitialize()
        {
            FabricRuntimeTest.Cleanup();
        }

        [ClassCleanup]
        public static void Cleanup()
        {
            FabricRuntime.FabricRuntimeFactory = new FabricRuntime.NativeFabricRuntimeFactory();
        }

        #region Fabric Runtime Creation

        private void SuccessfulCreateTestHelper(Action<FabricRuntimeFactoryStub> setup, Func<FabricRuntimeFactoryStub, FabricRuntime> creationFunc, Action<FabricRuntime, FabricRuntimeFactoryStub> validation = null)
        {
            // setup the objects
            var stub = new FabricRuntimeFactoryStub();
            setup(stub);

            FabricRuntime.FabricRuntimeFactory = stub;

            // create should succeed
            FabricRuntime runtime = creationFunc(stub);

            Assert.IsNotNull(runtime);

            // the guid should match the FabricRuntime Guid
            Assert.AreEqual<Guid>(typeof(NativeRuntime.IFabricRuntime).GUID, stub.GuidReceived);

            if (validation != null)
            {
                validation(runtime, stub);
            }
        }

        private void SuccessfulSyncCreateTestHelper(Action<FabricRuntimeFactoryStub> setup, Action exitNotificationCallback, Action<FabricRuntime, FabricRuntimeFactoryStub> validation = null)
        {
            this.SuccessfulCreateTestHelper(
                setup,
                (stub) =>
                {
                    FabricRuntime runtime = null;

                    // start creating the runtime on a background task
                    var createBgTask = Task.Factory.StartNew(() => runtime = FabricRuntime.Create(exitNotificationCallback));

                    // this will call the async version of the api on the stub -> complete that so that it returns
                    stub.AsyncCompleteEvent.Set();

                    // the task should complete
                    Assert.IsTrue(createBgTask.Wait(this.GetDurationToWaitForTask()));

                    return runtime;
                },
                validation);
        }

        private TimeSpan GetDurationToWaitForTask()
        {
            return TimeSpan.FromMilliseconds(10000 * (Debugger.IsAttached ? 100 : 1));
        }

        private void SuccessfulAsyncCreateTestHelper(Action<FabricRuntimeFactoryStub> setup, Action exitNotificationCallback, Action<FabricRuntime, FabricRuntimeFactoryStub> validation = null)
        {
            this.SuccessfulCreateTestHelper(
                setup,
                (stub) =>
                {
                    // start creating the runtime on a background task
                    var createTask = FabricRuntime.CreateAsync(exitNotificationCallback, FabricRuntimeTest.DefaultTimeout, CancellationToken.None);

                    // this will call the async version of the api on the stub -> complete that so that it returns
                    stub.AsyncCompleteEvent.Set();

                    // the task should complete
                    Assert.IsTrue(createTask.Wait(this.GetDurationToWaitForTask()));

                    return createTask.Result;
                },
                validation);
        }

        private void CreateValidation(uint timeoutExpected, CodePackageActivationContext contextExpected, FabricRuntime runtime, FabricRuntimeFactoryStub stub)
        {
            Assert.AreEqual<uint>(timeoutExpected, stub.TimeoutReceived);
            Assert.AreSame(contextExpected, runtime.CodePackageActivationContext);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Create_ManagedFabricApp()
        {
            // an application whose life cycle is managed by fabric
            LogHelper.Log("CreateManaged_Sync");
            var context = FabricRuntimeTest.DefaultCodePackageActivationContext;
            this.SuccessfulSyncCreateTestHelper(
                (stub) => stub.CodePackageActivationContextToReturn = context,
                null,
                (rt, stub) => this.CreateValidation(FabricRuntimeTest.TimeoutForCreateSync, context, rt, stub));

            LogHelper.Log("CreateManaged_Async");
            this.SuccessfulAsyncCreateTestHelper(
                (stub) => stub.CodePackageActivationContextToReturn = context,
                null,
                (rt, stub) => this.CreateValidation((uint)FabricRuntimeTest.DefaultTimeout.TotalMilliseconds, context, rt, stub));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Create_Sync_CreateSucceedsIfSystemFabricIsBeginHostedInFWPOrAdHoc()
        {
            // an application whose life cycle is managed by fabric
            LogHelper.Log("Sync");
            this.SuccessfulSyncCreateTestHelper(
                (stub) => stub.ExceptionToThrowOnGetCodePackageActivationContext = new InvalidOperationException(),
                null,
                (rt, stub) => this.CreateValidation(FabricRuntimeTest.TimeoutForCreateSync, null, rt, stub));

            LogHelper.Log("Async");
            this.SuccessfulAsyncCreateTestHelper(
                (stub) => stub.ExceptionToThrowOnGetCodePackageActivationContext = new InvalidOperationException(),
                null,
                (rt, stub) => this.CreateValidation((uint)FabricRuntimeTest.DefaultTimeout.TotalMilliseconds, null, rt, stub));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_ExitEventHandlerValidationHelper()
        {
            bool wasExitEventInvoked = false;
            Action exitedHandler = () =>
            {
                wasExitEventInvoked = true;
            };

            Action<FabricRuntime, FabricRuntimeFactoryStub> validation = (runtime, stub) =>
                {
                    // invoke the event by firing the FabricExited on the IFabricProcessExitHandler interface passed in
                    stub.ExitProcessHandlerReceived.FabricProcessExited();

                    // the exited event should have been invoked
                    Assert.IsTrue(wasExitEventInvoked);
                };


            wasExitEventInvoked = false;
            LogHelper.Log("Sync");
            this.SuccessfulSyncCreateTestHelper((stub) => { ;}, exitedHandler, validation);

            wasExitEventInvoked = false;
            LogHelper.Log("Async");
            this.SuccessfulAsyncCreateTestHelper((stub) => { ;}, exitedHandler, validation);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_NullHandlerDoesNotCrash()
        {
            Action<FabricRuntime, FabricRuntimeFactoryStub> validation = (runtime, stub) =>
            {
                // invoke the event by firing the FabricExited on the IFabricProcessExitHandler interface passed in
                stub.ExitProcessHandlerReceived.FabricProcessExited();
            };


            LogHelper.Log("Sync");
            this.SuccessfulSyncCreateTestHelper((stub) => { ;}, null, validation);

            LogHelper.Log("Async");
            this.SuccessfulAsyncCreateTestHelper((stub) => { ;}, null, validation);
        }

        #endregion

        #region Factory Registration/Unregistration

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Register_TypeDerivedFromBothStatefulAndStatelessFails()
        {
            var runtime = FabricRuntimeTest.GetDefaultRuntime();

            AssertThrows<ArgumentException>(() => runtime.RegisterServiceType(FabricRuntimeTest.DefaultServiceName, typeof(Stubs.InvalidServiceStub)));
            AssertThrows<ArgumentException>(() => runtime.RegisterServiceTypeAsync(FabricRuntimeTest.DefaultServiceName, typeof(Stubs.InvalidServiceStub), FabricRuntimeTest.DefaultTimeout, CancellationToken.None));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Register_TypeDerivedFromNeitherStatefulNorStatelessFails()
        {
            var runtime = FabricRuntimeTest.GetDefaultRuntime();

            AssertThrows<ArgumentException>(() => runtime.RegisterServiceType(FabricRuntimeTest.DefaultServiceName, typeof(int)));
            AssertThrows<ArgumentException>(() => runtime.RegisterServiceTypeAsync(FabricRuntimeTest.DefaultServiceName, typeof(int), FabricRuntimeTest.DefaultTimeout, CancellationToken.None));
        }

	private void AssertThrows<T>(Action task)
	{
	    try
	    {
	        task();
	    }
	    catch (Exception ex)
	    {
	        Assert.IsInstanceOfType(ex, typeof(T), string.Format("Expected exception of type {0} but exception {1} was thrown.", typeof(T), ex));
	        return;
	    }

	    Assert.Fail(string.Format("Expected exception of type {0} but no exception was thrown.", typeof(T)));
	}

        private void RegisterTestHelper(CodePackageActivationContext context, bool isStateful, uint timeoutExpected, bool isSyncCallExpected, Action<string, FabricRuntime, Stubs.FabricRuntimeStubSupportingFactoryRegistration> registerFunc, Action<ServiceFactoryBroker> factoryBrokerValidator)
        {
            var runtime = new FabricRuntime(context, null);
            var stub = new Stubs.FabricRuntimeStubSupportingFactoryRegistration();
            runtime.NativeRuntimeObject = stub;

            registerFunc(FabricRuntimeTest.DefaultServiceName, runtime, stub);

            Assert.AreEqual(1, stub.ServiceFactoryRegistrations.Count);
            Assert.AreEqual(isStateful, stub.ServiceFactoryRegistrations[0].IsStateful);
            Assert.AreEqual(FabricRuntimeTest.DefaultServiceName, stub.ServiceFactoryRegistrations[0].ServiceType);
            Assert.AreEqual<bool>(isSyncCallExpected, stub.WasLastCallSynchronous);
            Assert.AreEqual<uint>(timeoutExpected, stub.ServiceFactoryRegistrations[0].Timeout);

            var registeredFactoryBroker = stub.ServiceFactoryRegistrations[0].ServiceFactoryBroker;
            Assert.IsNotNull(registeredFactoryBroker);
            Assert.AreSame(context, registeredFactoryBroker.CodePackageActivationContext);

            factoryBrokerValidator(registeredFactoryBroker);

        }

        private Action<ServiceFactoryBroker, Type> DefaultServiceFactoryRegistrationValidator = (registeredFactoryBroker, implementationType) =>
        {
            var defaultServiceFactory = registeredFactoryBroker.ServiceFactory as DefaultServiceFactory;
            Assert.AreSame(implementationType, defaultServiceFactory.ServiceImplementationType);
        };

        private void RegisterSyncTestHelper_DefaultServiceFactory(CodePackageActivationContext context, Type implementationType, bool isStateful)
        {
            this.RegisterTestHelper(
                context,
                isStateful,
                0,
                true,
                (s, rt, stub) => rt.RegisterServiceType(s, implementationType),
                (broker) => this.DefaultServiceFactoryRegistrationValidator(broker, implementationType));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Register_Sync_DefaultStatelessServiceFactory()
        {
            this.RegisterSyncTestHelper_DefaultServiceFactory(FabricRuntimeTest.DefaultCodePackageActivationContext, typeof(Stubs.StatelessServiceStubBase), false);
            this.RegisterSyncTestHelper_DefaultServiceFactory(null, typeof(Stubs.StatelessServiceStubBase), false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Register_Sync_DefaultStatefulServiceFactory()
        {
            this.RegisterSyncTestHelper_DefaultServiceFactory(FabricRuntimeTest.DefaultCodePackageActivationContext, typeof(Stubs.StatefulServiceReplicaStubBase), true);
            this.RegisterSyncTestHelper_DefaultServiceFactory(null, typeof(Stubs.StatefulServiceReplicaStubBase), true);
        }

        private void RegisterAsyncTestHelper_DefaultServiceFactory(CodePackageActivationContext context, Type implementationType, bool isStateful)
        {
            this.RegisterTestHelper(context, isStateful, (uint)FabricRuntimeTest.DefaultTimeout.TotalMilliseconds, false,
                (s, rt, stub) =>
                {
                    var registerTask = rt.RegisterServiceTypeAsync(s, implementationType, FabricRuntimeTest.DefaultTimeout, CancellationToken.None);

                    // set the event
                    stub.AsyncEvent.Set();

                    Assert.IsTrue(registerTask.Wait(1000));
                },
                (broker) => this.DefaultServiceFactoryRegistrationValidator(broker, implementationType));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Register_Async_DefaultStatelessServiceFactory()
        {
            this.RegisterAsyncTestHelper_DefaultServiceFactory(FabricRuntimeTest.DefaultCodePackageActivationContext, typeof(Stubs.StatelessServiceStubBase), false);
            this.RegisterAsyncTestHelper_DefaultServiceFactory(null, typeof(Stubs.StatelessServiceStubBase), false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Register_Async_DefaultStatefulServiceFactory()
        {
            this.RegisterAsyncTestHelper_DefaultServiceFactory(FabricRuntimeTest.DefaultCodePackageActivationContext, typeof(Stubs.StatefulServiceReplicaStubBase), true);
            this.RegisterAsyncTestHelper_DefaultServiceFactory(null, typeof(Stubs.StatefulServiceReplicaStubBase), true);
        }

        private void RegisterSyncTestHelper_CustomServiceFactory(CodePackageActivationContext context, Action<string, FabricRuntime, object> registerFunc, object serviceFactory, bool isStateful)
        {
            this.RegisterTestHelper(
                context,
                isStateful,
                0,
                true,
                (s, rt, stub) => registerFunc(s, rt, serviceFactory),
                (broker) => Assert.AreSame(serviceFactory, broker.ServiceFactory));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Register_Sync_CustomStatelessServiceFactory()
        {
            Action<string, FabricRuntime, object> registerFunc = (s, rt, factory) => rt.RegisterStatelessServiceFactory(s, factory as IStatelessServiceFactory);

            this.RegisterSyncTestHelper_CustomServiceFactory(FabricRuntimeTest.DefaultCodePackageActivationContext, registerFunc, new Stubs.StatelessServiceFactoryStubBase(), false);
            this.RegisterSyncTestHelper_CustomServiceFactory(null, registerFunc, new Stubs.StatelessServiceFactoryStubBase(), false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Register_Sync_CustomStatefulServiceFactory()
        {
            Action<string, FabricRuntime, object> registerFunc = (s, rt, factory) => rt.RegisterStatefulServiceFactory(s, factory as IStatefulServiceFactory);

            this.RegisterSyncTestHelper_CustomServiceFactory(FabricRuntimeTest.DefaultCodePackageActivationContext, registerFunc, new Stubs.StatefulServiceFactoryStubBase(), true);
            this.RegisterSyncTestHelper_CustomServiceFactory(null, registerFunc, new Stubs.StatefulServiceFactoryStubBase(), true);
        }

        private void RegisterAsyncTestHelper_CustomServiceFactory(
            CodePackageActivationContext context,
            object serviceFactory,
            bool isStateful,
            Func<string, FabricRuntime, object, TimeSpan, Task> registerFunc)
        {
            this.RegisterTestHelper(context, isStateful, (uint)FabricRuntimeTest.DefaultTimeout.TotalMilliseconds, false,
                (s, rt, stub) =>
                {
                    var registerTask = registerFunc(s, rt, serviceFactory, FabricRuntimeTest.DefaultTimeout);

                    // set the event
                    stub.AsyncEvent.Set();

                    Assert.IsTrue(registerTask.Wait(1000));
                },
                (broker) => Assert.AreSame(serviceFactory, broker.ServiceFactory));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Register_Async_CustomStatelessServiceFactory()
        {
            Func<string, FabricRuntime, object, TimeSpan, Task> registerFunc = (s, rt, f, timespan) => rt.RegisterStatelessServiceFactoryAsync(s, f as IStatelessServiceFactory, timespan, CancellationToken.None);

            this.RegisterAsyncTestHelper_CustomServiceFactory(FabricRuntimeTest.DefaultCodePackageActivationContext, new Stubs.StatelessServiceFactoryStubBase(), false, registerFunc);
            this.RegisterAsyncTestHelper_CustomServiceFactory(null, new Stubs.StatelessServiceFactoryStubBase(), false, registerFunc);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_Register_Async_CustomStatefulServiceFactory()
        {
            Func<string, FabricRuntime, object, TimeSpan, Task> registerFunc = (s, rt, f, timespan) => rt.RegisterStatefulServiceFactoryAsync(s, f as IStatefulServiceFactory, timespan, CancellationToken.None);

            this.RegisterAsyncTestHelper_CustomServiceFactory(FabricRuntimeTest.DefaultCodePackageActivationContext, new Stubs.StatefulServiceFactoryStubBase(), true, registerFunc);
            this.RegisterAsyncTestHelper_CustomServiceFactory(null, new Stubs.StatefulServiceFactoryStubBase(), true, registerFunc);
        }

        #endregion

        #region Code Package Activation Context

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_GetActivationContext_ReturnsCorrectValueFromNativeCode()
        {
            var stub = new FabricRuntimeFactoryStub
            {
                CodePackageActivationContextToReturn = FabricRuntimeTest.DefaultCodePackageActivationContext
            };

            FabricRuntime.FabricRuntimeFactory = stub;
            Assert.AreSame(stub.CodePackageActivationContextToReturn, FabricRuntime.GetActivationContext());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricRuntime_GetActivationContext()
        {
            var stub = new FabricRuntimeFactoryStub();

            var context = FabricRuntimeTest.DefaultCodePackageActivationContext;

            stub.CodePackageActivationContextToReturn = context;

            FabricRuntime.FabricRuntimeFactory = stub;
            Assert.AreSame(context, FabricRuntime.GetActivationContext());
        }

        #endregion

        private static FabricRuntime GetDefaultRuntime(CodePackageActivationContext context = null)
        {
            return new FabricRuntime(context == null ? FabricRuntimeTest.DefaultCodePackageActivationContext : context, null);
        }

        private class FabricRuntimeFactoryStub : FabricRuntime.NativeFabricRuntimeFactory
        {
            public NativeRuntime.IFabricProcessExitHandler ExitProcessHandlerReceived { get; set; }
            public Guid GuidReceived { get; set; }
            public uint TimeoutReceived { get; set; }
            public CodePackageActivationContext CodePackageActivationContextToReturn { get; set; }
            public Exception ExceptionToThrowOnGetCodePackageActivationContext { get; set; }
            public Exception ExceptionToThrowOnInitFabricRTAsync { get; set; }

            public ManualResetEventSlim AsyncCompleteEvent { get; private set; }

            public FabricRuntimeFactoryStub()
            {
                this.AsyncCompleteEvent = new ManualResetEventSlim();
                this.AsyncCompleteEvent.Reset();
            }

            public override Task<FabricRuntime> InitializeFabricRuntimeAsync(FabricRuntime runtime, Guid fabricRuntimeGuid, NativeRuntime.IFabricProcessExitHandler processExitHandler, uint timeout)
            {
                this.GuidReceived = fabricRuntimeGuid;
                this.ExitProcessHandlerReceived = processExitHandler;
                this.TimeoutReceived = timeout;
                runtime.NativeRuntimeObject = new Stubs.FabricRuntimeStubBase();

                return Task.Factory.StartNew<FabricRuntime>(
                    () =>
                    {
                        this.AsyncCompleteEvent.Wait();
                        if (this.ExceptionToThrowOnInitFabricRTAsync != null)
                        {
                            throw this.ExceptionToThrowOnInitFabricRTAsync;
                        }
                        return runtime;
                    });
            }

            public override CodePackageActivationContext GetActivationContext()
            {
                if (this.ExceptionToThrowOnGetCodePackageActivationContext != null)
                {
                    throw this.ExceptionToThrowOnGetCodePackageActivationContext;
                }

                return this.CodePackageActivationContextToReturn;
            }
        }
    }
}