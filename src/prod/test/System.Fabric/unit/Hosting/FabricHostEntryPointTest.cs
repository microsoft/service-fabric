// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Hosting
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Hosting;
    using System.Fabric.Interop;
    using System.Fabric.Test.Stubs;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class FabricHostEntryPointTest
    {
        private static string BinariesDir;

        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            FabricHostEntryPointTest.BinariesDir = context.TestDeploymentDir;
        }

        #region Constructor and initial properties

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void HEP_Constructor_InitialActiveCodePackageCountIsZero()
        {
            var hep = new FabricHostEntryPoint();
            Assert.AreEqual(0, hep.ActiveCodePackageCount);
        }

        #endregion

        #region Start

        #endregion


        #region Deactivation

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void HEP_Deactivate_DeactivatingUnknownCodePackageFails()
        {
            var hep = this.CreateAndInvokeStartWithDefaults();

            try
            {
                hep.DeactivateCodePackage("hi");
                Assert.Fail("Expected exception");
            }
            catch (InvalidOperationException)
            {
            }
        }


        #endregion


        private EntryPointInfo CreateEntryPointInfo(string assembly, NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND type = NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED)
        {
            return new EntryPointInfo
            {
                Description = new DllHostEntryPointInfo
                {
                    HostedDlls =
                    {
                        new DllHostItemInfo
                        {
                            Item = assembly,
                            Type = type
                        }   
                    }
                }.ToNative()

            };
        }

        private FabricHostEntryPoint CreateStartAndInvokeActivate(ActivationParameters activationParams, bool validate = true)
        {
            var hep = this.CreateAndInvokeStartWithDefaults();

            this.InvokeActivate(hep, activationParams, validate);

            return hep;
        }

        private void InvokeActivate(FabricHostEntryPoint hep, ActivationParameters activationParams, bool validate = true)
        {
            if (validate)
            {
                activationParams.Validate();
            }

            hep.ActivateCodePackageInternal(
                    activationParams.ActivationContextId,
                    activationParams.CodePackageName,
                    activationParams.CodePackageActivationContext,
                    activationParams.Stub);
        }

        private void InvokeStartWithDefaults(FabricHostEntryPoint hep)
        {
            hep.Start(
                Constants.DefaultLogDirectory,
                FabricHostEntryPointTest.BinariesDir,
                Constants.DefaultAppDomainConfigFilePath,
                Constants.DefaultHostEntryPointManagerUniqueId);
        }

        private FabricHostEntryPoint CreateAndInvokeStartWithDefaults()
        {
            var hep = new FabricHostEntryPoint();

            this.InvokeStartWithDefaults(hep);

            return hep;
        }


        public class Constants
        {
            public const string DefaultActivationContextId = "activation_context_id";
            public const string DefaultCodePackageName = "default_cp_name";

            public const string DefaultLogDirectory = "log_dir";
            public const string DefaultAppDomainConfigFilePath = "adcfgfp";
            public const string DefaultHostEntryPointManagerUniqueId = "hep_unique_id";

            public const string BasicStatelessServiceTypeName = "BasicStatelessService";
            public const string BasicStatefulServiceTypeName = "BasicStatefulService";
        }

        private class ActivationParameters
        {
            private CodePackageActivationContext codePackageActivationContext;

            public string ActivationContextId { get; set; }
            public string CodePackageName { get; set; }
            public List<CodePackageInfo> CodePackages { get; set; }
            public List<ServiceTypeInfo> ServiceTypes { get; set; }
            public Stubs.FabricRuntimeStubSupportingFactoryRegistration Stub { get; set; }

            public CodePackageActivationContext CodePackageActivationContext
            {
                get
                {
                    if (this.codePackageActivationContext == null)
                    {
                        this.codePackageActivationContext = this.CreateActivationContext();
                    }

                    return this.codePackageActivationContext;
                }
            }

            public ActivationParameters()
            {
                this.ServiceTypes = new List<ServiceTypeInfo>();
                this.CodePackages = new List<CodePackageInfo>();
            }

            public virtual void Validate()
            {
                // helper method to make sure any errors are caught here itself
                Assert.IsFalse(string.IsNullOrEmpty(this.ActivationContextId));
                Assert.IsFalse(string.IsNullOrEmpty(this.CodePackageName));
                Assert.AreNotEqual(0, this.ServiceTypes.Count);
                Assert.AreNotEqual(0, this.CodePackages.Count);
                Assert.IsTrue(this.CodePackages.Where(cp => cp.Name == this.CodePackageName).Count() != 0);
            }

            private CodePackageActivationContext CreateActivationContext()
            {
                ServiceManifestInfo manifestInfo = new ServiceManifestInfo
                {

                };

                foreach (var item in this.CodePackages)
                {
                    manifestInfo.CodePackages.Add(item);
                }

                foreach (var item in this.ServiceTypes)
                {
                    manifestInfo.ServiceTypes.Add(item);
                }

                return new CodePackageActivationContext(new Stubs.CodePackageActivationContextStub
                {
                    ServiceManifestInfo_Internal = manifestInfo,
                    ContextId_Internal = this.ActivationContextId,
                });
            }
        }
    }
}