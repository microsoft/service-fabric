// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.Globalization;

    [TestClass]
    public class SendApplicationHealthReportTests : TestHealthReportHelper
    {
        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            TestHealthReportHelper.ClassInitializeBase(context);
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            TestHealthReportHelper.ClassCleanupBase();
        }

        public static readonly Uri testApplicationUriDefault = new Uri("fabric:/testapp/myDefaultAppName");

        public Uri testApplicationUri
        {
            get;
            set;
        }

        public SendApplicationHealthReportTests()
        {
            SetDefaultValues();
        }

        /**
         * A typical test case should look like this:
         *
         * this.SetDefaultValues();                  // Reset to default values
         * [this.somefield = specificVal;]*          // optional
         * [this.setException(new someException());] // optional
         * this.invokeCmdletAndVerify();             // invoke cmdlet and verify
         * */

        /// <summary>
        /// Set MockClusterConnection and cmdlet arguments to default valid values.
        /// Then verifies that powershell command returns without any exception.
        /// </summary>
        [TestMethod]
        [Owner("vikkuma")]
        public void SendApplicationHealthTestPositionalParameterPositive()
        {
            /// Use only default arguments for the command.
            this.SetDefaultValues();

            /// No output validation as this powershell command's return type is void.
            this.InvokeCmdletAndVerify();
        }

        /// <summary>
        /// Set MockClusterConnection to send an Exception.
        /// Then verifies that powershell command returns the appropriate exception.
        /// </summary>
        [TestMethod]
        [Owner("vikkuma")]
        public void SendApplicationHealthTestExceptionNegative()
        {
            /// Set default values
            this.SetDefaultValues();

            /// Set Exception
            Exception expect = new ArgumentException(StringResources.Error_ArgumentInvalid);
            this.SetException(expect);
            this.InvokeCmdletAndVerify();
        }

        /// <summary>
        /// It will set optional arguments Description, TTL, RemoveWhenExpired.
        /// </summary>
        [TestMethod]
        [Owner("vikkuma")]
        public void SendApplicationHealthTestOptionalParameterPositive()
        {
            /// Set default values
            this.SetDefaultValues();

            /// Set some optional values.
            this.healthInformation.Description = "TestDescription for HealthInfo";
            this.healthInformation.TimeToLive = TimeSpan.FromSeconds(100);
            this.healthInformation.RemoveWhenExpired = true;

            /// No output validation as this powershell command's return type is void.
            this.InvokeCmdletAndVerify();
        }

        /// <summary>
        /// Creates and returns a new instance of MockClusterConnection with default settings.
        /// </summary>
        public MockClusterConnection CreateMockClusterConnection()
        {
            var mockClusterConnection = new MockClusterConnection
            {
                ApiNameArray = new[] { TestConstants.SendHealthReportApiName },
            };
            return mockClusterConnection;
        }

        /// <summary>
        /// Creates cmdlet-script using current fields.
        /// </summary>
        /// <returns>cmdlet-script</returns>
        public string CreateCmdletScript()
        {
            /// send-health-report-command, appname, HealthInformation's args
            string cmdScript = string.Format(
                CultureInfo.InvariantCulture,
                "{0} {1} {2} ", 
                TestConstants.SendApplicationHealthCommand,
                testApplicationUri,
                base.GetCmdletHealthInfoArguments());

            return cmdScript;
        }

        /// <summary>
        /// This method takes care of invoking current cmdlet-script and then validating the result.
        /// </summary>
        /// <returns>True if test-case passed.</returns>
        override public bool InvokeCmdletAndVerify()
        {
            /// Initialize mockcluster connection.
            this.mockClusterConnection = this.CreateMockClusterConnection();

            /// Set argument mockClusterConnection.healthReport to reflect any updated values.
            this.mockClusterConnection.healthReport =
                new ApplicationHealthReport(testApplicationUri, base.healthInformation);

            /// Set the cmdletScript
            this.cmdletScript = this.CreateCmdletScript();

            /// Then call super
            return base.InvokeCmdletAndVerify(this.mockClusterConnection, this.cmdletScript);
        }

        /// <summary>
        /// Sets class fields to default values.
        /// </summary>
        override public void SetDefaultValues()
        {
            base.SetDefaultValues();
            this.testApplicationUri = testApplicationUriDefault;
        }
    }
}