// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Fabric.Health;
    using WEX.Logging.Interop;

    /// Base class of all {something}ReportHealthHelper classes.
    /// Helps testing cmdlets of form "Send-{something}HealthReport"
    /// It has constants which are present in any healthreport to create HealthReports.
    [TestClass]
    public class TestHealthReportHelper : TestHelper
    {
        /// This attribute does not automatically work in the base class of a test-class.
        /// Each derived test class should call this method explicitly.
        [ClassInitialize]
        public static void ClassInitializeBase(TestContext context)
        {
            LogController.InitializeLogging();
            TestUtility.OpenPowerShell();
        }

        [ClassCleanup]
        public static void ClassCleanupBase()
        {
            TestUtility.ClosePowerShell();
            LogController.FinalizeLogging();
        }

        public const HealthState testAppHealthStateDefault = HealthState.Ok;
        public const string testHealthReportSourceIdDefault = "sourceId_123";
        public const string testHealthReportPropertyDefault = "property_123";

        public HealthState testAppHealthState
        {
            get;
            set;
        }

        public string testHealthReportSourceId
        {
            get;
            set;
        }

        public string testHealthReportProperty
        {
            get;
            set;
        }

        /// Nullable types for optional fields.
        public string testHealthReportDescription
        {
            get;
            set;
        }

        public bool? testRemoveWhenExpired
        {
            get;
            set;
        }

        public TimeSpan? testTimeToLive
        {
            get;
            set;
        }

        // Constructor
        public TestHealthReportHelper()
        {
            this.SetDefaultValues();
        }
        
        /// <summary>
        /// This will be set and updated when there is a change in health information fields.
        /// </summary>
        public HealthInformation healthInformation
        {
            get
            {
                return this.UpdatedHealthInformation();
            }
        }

        /// <summary>
        /// Creates an instance of HealthInformation using default values.
        /// </summary>
        /// <returns> Default HealthInformation </returns>
        public static HealthInformation DefaultHealthInformation()
        {
            return new HealthInformation(
                       testHealthReportSourceIdDefault,
                       testHealthReportPropertyDefault,
                       testAppHealthStateDefault);
        }

        public HealthInformation UpdatedHealthInformation()
        {
            var healthInfo = new HealthInformation(
                testHealthReportSourceId,
                testHealthReportProperty,
                testAppHealthState);

            if (testHealthReportDescription != null)
            {
                healthInfo.Description = testHealthReportDescription;
            }

            if (testTimeToLive.HasValue)
            {
                healthInfo.TimeToLive = testTimeToLive.Value;
            }

            if (testRemoveWhenExpired.HasValue)
            {
                healthInfo.RemoveWhenExpired = testRemoveWhenExpired.Value;
            }
            
            return healthInfo;
        }

        /// <summary>
        /// This will create a string of command line arguments using the current test-values.
        /// Will be used to create a cmdlet-script.
        /// </summary>
        /// <returns>string which space delimited arguments </returns>
        public string GetCmdletHealthInfoArguments()
        {
            /// Required parameters: HeathStatus, SourceId, HealthProperty,
            /// Optional parameters: Description, TimeToLive, RemoveWhenExpired
            string healthInfoArgs =
                " -HealthState " + this.testAppHealthState.ToString() +
                " -SourceId " + this.testHealthReportSourceId +
                " -HealthProperty " + this.testHealthReportProperty;

            /// add optional values if user has set it.
            if (testHealthReportDescription != null)
            {
                healthInfoArgs += " -Description " + testHealthReportDescription;
            }

            if (testTimeToLive.HasValue)
            {
                healthInfoArgs = healthInfoArgs + " -TimeToLiveSec " + testTimeToLive.Value.Seconds.ToString();
            }

            /// Adding '$' as in powershell we can provide boolean as $True/$False
            if (testRemoveWhenExpired.HasValue) 
            {
                healthInfoArgs = healthInfoArgs + " -RemoveWhenExpired $" + testRemoveWhenExpired.Value.ToString();
            }

            return healthInfoArgs;
        }

        override public void SetDefaultValues()
        {
            base.SetDefaultValues();
            this.testAppHealthState = testAppHealthStateDefault;
            this.testHealthReportSourceId = testHealthReportSourceIdDefault;
            this.testHealthReportProperty = testHealthReportPropertyDefault;
            this.testHealthReportDescription = null;
        }
    }
}