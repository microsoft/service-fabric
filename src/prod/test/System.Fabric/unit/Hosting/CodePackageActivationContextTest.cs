// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Hosting
{
    using System;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Test.Description;
    using System.Fabric.Test.Stubs;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    //
    // NOTE: These test cases leak memory. 
    [TestClass]
    public class CodePackageActivationContextTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContextTest_GetServiceManifestTest()
        {
            ServiceManifestInfo manifest = ServiceManifestTest.HelperInstance.CreateDefaultInfo();

            var context = CodePackageActivationContext.CreateFromNative(new CodePackageActivationContextStub()
            {
                ServiceManifestInfo_Internal = manifest,
                CodePackageName_Internal = manifest.CodePackages.ElementAt(0).Name
            });

            ServiceManifestTest.HelperInstance.Compare(ServiceManifestTest.HelperInstance.CreateDefaultDescription(), ServiceManifest.CreateFromCodePackageActivationContext(context));
        }


        #region Properties on the context object

        private static void SimplePropertiesOnContextObjectTest(
            Action<CodePackageActivationContextStub, string> setter,
            Func<CodePackageActivationContext, string> getter)
        {
            const string text = "abc";
            var stub = new CodePackageActivationContextStub();

            setter(stub, text);

            var context = CodePackageActivationContext.CreateFromNative(stub);

            Assert.AreEqual(text, getter(context));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContext_Parsing_LogDirectoryIsAssigned()
        {
            CodePackageActivationContextTest.SimplePropertiesOnContextObjectTest(
                (stub, text) => stub.LogDirectory_Internal = text,
                (context) => context.LogDirectory);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContext_Parsing_WorkDirectoryIsAssigned()
        {
            CodePackageActivationContextTest.SimplePropertiesOnContextObjectTest(
                (stub, text) => stub.WorkDirectory_Internal = text,
                (context) => context.WorkDirectory);
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContext_Parsing_ContextIdIsAssigned()
        {
            CodePackageActivationContextTest.SimplePropertiesOnContextObjectTest(
                (stub, text) => stub.ContextId_Internal = text,
                (context) => context.ContextId);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContext_Parsing_CodePackageNameIsAssigned()
        {
            CodePackageActivationContextTest.SimplePropertiesOnContextObjectTest(
                (stub, text) => stub.CodePackageName_Internal = text,
                (context) => context.CodePackageName);
        }

        #endregion

        #region GetPackage

        delegate TPackage PackageFetcher<TPackage>(ServiceManifest manifest, out string pkgName);

        private void GetPackageTestHelper<TPackage>(
            Action<TPackage, TPackage> comparer,
            PackageFetcher<TPackage> packageFetcher, 
            Func<CodePackageActivationContext, string, TPackage> contextPackageFetcher) 
            where TPackage : class
        {
            ServiceManifestInfo manifestInfo = ServiceManifestTest.HelperInstance.CreateDefaultInfo();

            var context = new CodePackageActivationContext(new CodePackageActivationContextStub() { ServiceManifestInfo_Internal = manifestInfo });

            ServiceManifest manifestDescription = ServiceManifestTest.HelperInstance.CreateDefaultDescription();

            string pkgName;
            var pkg = packageFetcher(manifestDescription, out pkgName);
            var actualPackageFromContext = contextPackageFetcher(context, pkgName);

            comparer(pkg, actualPackageFromContext);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContextTest_GetCodePackage()
        {
            this.GetPackageTestHelper(
                CodePackageTest.HelperInstance.Compare,
                (ServiceManifest manifest, out string pkgName) => { var pkg = manifest.CodePackages.First(); pkgName = pkg.Description.Name; return pkg; },
                (context, name) => context.GetCodePackageObject(name));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContextTest_GetConfigurationPackage()
        {
            this.GetPackageTestHelper(
                ConfigurationPackageTest.HelperInstance.Compare,
                (ServiceManifest manifest, out string pkgName) => { var pkg = manifest.ConfigurationPackages.First(); pkgName = pkg.Description.Name; return pkg; },
                (context, name) => context.GetConfigurationPackageObject(name));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContextTest_GetDataPackage()
        {
            this.GetPackageTestHelper(
                DataPackageTest.HelperInstance.Compare,
                (ServiceManifest manifest, out string pkgName) => { var pkg = manifest.DataPackages.First(); pkgName = pkg.Description.Name; return pkg; },
                (context, name) => context.GetDataPackageObject(name));
        }

        #endregion

        #region Package Change Notifications

        private void OneHandlerRegistrationTestHelper(Func<CodePackageActivationContextStub, int> handlerCountFunc)
        {
            var stub = new CodePackageActivationContextStub();

            var context = new CodePackageActivationContext(stub);

            Assert.AreEqual<int>(1, handlerCountFunc(stub));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContext_AssertThatOnlyOneHandlerIsRegistered()
        {
            this.OneHandlerRegistrationTestHelper((s) => s.CodePackageRegistrations.Count);
            this.OneHandlerRegistrationTestHelper((s) => s.ConfigPackageRegistrations.Count);
            this.OneHandlerRegistrationTestHelper((s) => s.DataPackageRegistrations.Count);
        }

        class ChangeNotificationTestData<TPackageInfo, TPackage>
            where TPackageInfo : IPackageInfo
            where TPackage : class
        {
            public DescriptionElementTestBase<TPackage, TPackageInfo> Helper { get; set; }

            public Action<CodePackageActivationContext, Action<object, EventArgs>> EventHookupFunction { get; set; }

            public Action<CodePackageActivationContextStub, TPackageInfo, TPackageInfo> EventInvoker { get; set; }
        }

        private void ChangeNotificationTestHelper<TPackageInfo, TPackage>(
            ChangeNotificationTestData<TPackageInfo, TPackage> data,
            TPackageInfo oldPackage,
            TPackageInfo newPackage,
            Action<EventArgs> validator)
            where TPackageInfo : IPackageInfo
            where TPackage : class
        {
            var stub = new CodePackageActivationContextStub();

            // create the context
            var context = new CodePackageActivationContext(stub);

            EventArgs packageChangedArgs = null;

            // create a delegate for handling the event
            Action<object, EventArgs> eventHandler = (sender, args) =>
            {
                Assert.AreSame(context, sender);

                packageChangedArgs = args;
            };

            // hookup the delegate as the event handler
            data.EventHookupFunction(context, eventHandler);

            // invoke the event
            data.EventInvoker(stub, oldPackage, newPackage);

            // validate that the event was invoked
            Assert.IsNotNull(packageChangedArgs);

            validator(packageChangedArgs);
        }

        private void UpgradeOfExistingPackageChangeNotificationTestHelper<TPackageInfo, TPackage>(ChangeNotificationTestData<TPackageInfo, TPackage> data)
            where TPackageInfo : IPackageInfo
            where TPackage : class
        {
            var initialPackage = data.Helper.CreateDefaultInfo();
            var upgradedPackage = data.Helper.CreateDefaultInfo();

            upgradedPackage.Version = upgradedPackage.Version + "a";

            this.ChangeNotificationTestHelper(
                data,
                initialPackage,
                upgradedPackage,
                (packageChangedArgs) =>
                {
                    var casted = packageChangedArgs as PackageModifiedEventArgs<TPackage>;
                    Assert.IsNotNull(casted);

                    data.Helper.Compare(data.Helper.Factory(initialPackage), casted.OldPackage);
                    data.Helper.Compare(data.Helper.Factory(upgradedPackage), casted.NewPackage);
                });
        }

        private void AddOfNewPackageChangeNotificationTestHelper<TPackageInfo, TPackage>(ChangeNotificationTestData<TPackageInfo, TPackage> data)
            where TPackageInfo : IPackageInfo
            where TPackage : class
        {
            TPackageInfo initialPackage = default(TPackageInfo);
            var upgradedPackage = data.Helper.CreateDefaultInfo();

            this.ChangeNotificationTestHelper(
                data,
                initialPackage,
                upgradedPackage,
                (packageChangedArgs) =>
                {
                    var casted = packageChangedArgs as PackageAddedEventArgs<TPackage>;
                    Assert.IsNotNull(casted);

                    data.Helper.Compare(data.Helper.Factory(upgradedPackage), casted.Package);
                });
        }

        private void DeleteOfExistingPackageChangeNotificationTestHelper<TPackageInfo, TPackage>(ChangeNotificationTestData<TPackageInfo, TPackage> data)
            where TPackageInfo : IPackageInfo
            where TPackage : class
        {
            var initialPackage = data.Helper.CreateDefaultInfo();
            var upgradedPackage = default(TPackageInfo);

            this.ChangeNotificationTestHelper(
                data,
                initialPackage,
                upgradedPackage,
                (packageChangedArgs) =>
                {
                    var casted = packageChangedArgs as PackageRemovedEventArgs<TPackage>;
                    Assert.IsNotNull(casted);

                    data.Helper.Compare(data.Helper.Factory(initialPackage), casted.Package);
                });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContext_PackageChangeNotificationsWorkForCodePackages()
        {
            {
                var testData = new ChangeNotificationTestData<CodePackageInfo, CodePackage>
                {
                    EventHookupFunction = (context, handler) => context.CodePackageAddedEvent += new EventHandler<PackageAddedEventArgs<CodePackage>>(handler),
                    EventInvoker = (stub, oldInfo, newInfo) => stub.CodePackageRegistrations[0].OnPackageAdded(null, newInfo),
                    Helper = CodePackageTest.HelperInstance
                };

                this.AddOfNewPackageChangeNotificationTestHelper(testData);
            }

            {
                var testData = new ChangeNotificationTestData<CodePackageInfo, CodePackage>
                {
                    EventHookupFunction = (context, handler) => context.CodePackageRemovedEvent += new EventHandler<PackageRemovedEventArgs<CodePackage>>(handler),
                    EventInvoker = (stub, oldInfo, newInfo) => stub.CodePackageRegistrations[0].OnPackageRemoved(null, oldInfo),
                    Helper = CodePackageTest.HelperInstance
                };

                this.DeleteOfExistingPackageChangeNotificationTestHelper(testData);
            }

            {
                var testData = new ChangeNotificationTestData<CodePackageInfo, CodePackage>
                {
                    EventHookupFunction = (context, handler) => context.CodePackageModifiedEvent += new EventHandler<PackageModifiedEventArgs<CodePackage>>(handler),
                    EventInvoker = (stub, oldInfo, newInfo) => stub.CodePackageRegistrations[0].OnPackageModified(null, oldInfo, newInfo),
                    Helper = CodePackageTest.HelperInstance
                };

                this.UpgradeOfExistingPackageChangeNotificationTestHelper(testData);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContext_PackageChangeNotificationsWorkForConfigPackages()
        {
            {
                var testData = new ChangeNotificationTestData<ConfigurationPackageInfo, ConfigurationPackage>
                {
                    EventHookupFunction = (context, handler) => context.ConfigurationPackageAddedEvent += new EventHandler<PackageAddedEventArgs<ConfigurationPackage>>(handler),
                    EventInvoker = (stub, oldInfo, newInfo) => stub.ConfigPackageRegistrations[0].OnPackageAdded(null, newInfo),
                    Helper = ConfigurationPackageTest.HelperInstance
                };

                this.AddOfNewPackageChangeNotificationTestHelper(testData);
            }

            {
                var testData = new ChangeNotificationTestData<ConfigurationPackageInfo, ConfigurationPackage>
                {
                    EventHookupFunction = (context, handler) => context.ConfigurationPackageRemovedEvent += new EventHandler<PackageRemovedEventArgs<ConfigurationPackage>>(handler),
                    EventInvoker = (stub, oldInfo, newInfo) => stub.ConfigPackageRegistrations[0].OnPackageRemoved(null, oldInfo),
                    Helper = ConfigurationPackageTest.HelperInstance
                };

                this.DeleteOfExistingPackageChangeNotificationTestHelper(testData);
            }

            {
                var testData = new ChangeNotificationTestData<ConfigurationPackageInfo, ConfigurationPackage>
                {
                    EventHookupFunction = (context, handler) => context.ConfigurationPackageModifiedEvent += new EventHandler<PackageModifiedEventArgs<ConfigurationPackage>>(handler),
                    EventInvoker = (stub, oldInfo, newInfo) => stub.ConfigPackageRegistrations[0].OnPackageModified(null, oldInfo, newInfo),
                    Helper = ConfigurationPackageTest.HelperInstance
                };

                this.UpgradeOfExistingPackageChangeNotificationTestHelper(testData);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageActivationContext_PackageChangeNotificationsWorkForDataPackages()
        {
            {
                var testData = new ChangeNotificationTestData<DataPackageInfo, DataPackage>
                {
                    EventHookupFunction = (context, handler) => context.DataPackageAddedEvent += new EventHandler<PackageAddedEventArgs<DataPackage>>(handler),
                    EventInvoker = (stub, oldInfo, newInfo) => stub.DataPackageRegistrations[0].OnPackageAdded(null, newInfo),
                    Helper = DataPackageTest.HelperInstance
                };

                this.AddOfNewPackageChangeNotificationTestHelper(testData);
            }

            {
                var testData = new ChangeNotificationTestData<DataPackageInfo, DataPackage>
                {
                    EventHookupFunction = (context, handler) => context.DataPackageRemovedEvent += new EventHandler<PackageRemovedEventArgs<DataPackage>>(handler),
                    EventInvoker = (stub, oldInfo, newInfo) => stub.DataPackageRegistrations[0].OnPackageRemoved(null, oldInfo),
                    Helper = DataPackageTest.HelperInstance
                };

                this.DeleteOfExistingPackageChangeNotificationTestHelper(testData);
            }

            {
                var testData = new ChangeNotificationTestData<DataPackageInfo, DataPackage>
                {
                    EventHookupFunction = (context, handler) => context.DataPackageModifiedEvent += new EventHandler<PackageModifiedEventArgs<DataPackage>>(handler),
                    EventInvoker = (stub, oldInfo, newInfo) => stub.DataPackageRegistrations[0].OnPackageModified(null, oldInfo, newInfo),
                    Helper = DataPackageTest.HelperInstance
                };

                this.UpgradeOfExistingPackageChangeNotificationTestHelper(testData);
            }
        }

        #endregion
    }
}