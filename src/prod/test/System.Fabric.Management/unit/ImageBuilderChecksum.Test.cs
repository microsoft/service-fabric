// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;

    [TestClass]
    public class ImageBuilderChecksumTest
    {
        private TestContext testContextInstance;
        private ImageBuilderTestContext imageBuilderTestContext;
        private ImageBuilder imageBuilder;
        private IImageStore imageStore;
        
        public TestContext TestContext
        {
            get { return testContextInstance; }
            set { testContextInstance = value; }
        }

        [TestInitialize]
        public void TestSetup()
        {
            this.imageBuilderTestContext = ImageBuilderTestContext.Create(this.TestContext.TestName);
            this.imageBuilder = imageBuilderTestContext.ImageBuilder;
            this.imageStore = imageBuilderTestContext.ImageStore;
        }

        [TestCleanup]
        public virtual void TestCleanup()
        {
            this.imageBuilderTestContext.TestCleanup();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationTypeWithConflictingCodePackage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";

            applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            ServiceManifestType serviceManifestType = buildLayoutInfo.ServiceManifestTypes[0];
            TestUtility.ModifyCodePackage(serviceManifestType.CodePackage[0].Name, serviceManifestType, buildLayoutInfo);

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationTypeWithConflictingCodePackage_IgnoreConflict()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";

            applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            ServiceManifestType serviceManifestType = buildLayoutInfo.ServiceManifestTypes[0];
            TestUtility.ModifyCodePackage(serviceManifestType.CodePackage[0].Name, serviceManifestType, buildLayoutInfo);

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue, null, null, true);            
        }
    }
}