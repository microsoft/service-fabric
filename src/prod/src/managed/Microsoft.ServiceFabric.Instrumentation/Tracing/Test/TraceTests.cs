// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Test
{
    using System;
    using System.IO;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class TraceTests
    {
        private readonly string testPathPrefix = @"%_NTTREE%\FabricUnitTests\Microsoft.ServiceFabric.Instrumentation.Tracing.Test";
        private TestContext testContextInstance;

        /// <summary>
        ///  Gets or sets the test context which provides
        ///  information about and functionality for the current test run.
        ///</summary>
        public TestContext TestContext
        {
            get { return testContextInstance; }
            set { testContextInstance = value; }
        }

        [TestMethod]
        public void TestTraceStructure()
        {
            try
            {    
                ValidateTraceRecords.Validate(Environment.ExpandEnvironmentVariables(this.testPathPrefix));
            }
            catch (Exception exp)
            {
                this.TestContext.WriteLine("Encountered Exception : {0}", exp);
                Assert.Fail(exp.ToString());
            }
        }
    }
}
