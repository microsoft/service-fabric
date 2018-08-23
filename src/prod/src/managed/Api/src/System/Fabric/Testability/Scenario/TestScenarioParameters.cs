// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Scenario
{
    using System;

    /// <summary>
    /// Base class for parameters passed into all the TestScenarios which defines all the common parameters.
    /// </summary>
    [Serializable]
    public abstract class TestScenarioParameters
    {
        /// <summary>
        /// Default value for WaitTimeBetweenFaults used if value not specified by user.
        /// </summary>
        public static readonly TimeSpan WaitTimeBetweenFaultsDefault = TimeSpan.FromSeconds(20);

        internal static readonly TimeSpan OperationTimeoutDefault = TimeSpan.FromMinutes(5);
        internal const double RequestTimeoutFactorDefault = 0.2;

        private double requestTimeoutFactor;

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="timetoRun">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        protected internal TestScenarioParameters(TimeSpan timetoRun)
        {
            this.TimeToRun = timetoRun;
            this.requestTimeoutFactor = RequestTimeoutFactorDefault;
            this.OperationTimeout = OperationTimeoutDefault;
            this.WaitTimeBetweenFaults = WaitTimeBetweenFaultsDefault;
        }

        /// <summary>
        /// Total time for which the scenario will run before ending.
        /// </summary>
        /// <value>
        /// Returns the max run-time of the scenario as TimeSpan
        /// </value>
        public TimeSpan TimeToRun { get; private set; }

        /// <summary>
        /// The maximum wait time between consecutive faults: the larger the value, the lower the concurrency (of faults).
        /// </summary>
        /// <value>
        /// Returns the maximum wait time between two consecutive faults as a TimeSpan
        /// </value>
        public TimeSpan WaitTimeBetweenFaults { get; set; }

        internal TimeSpan OperationTimeout { get; set; }

        internal double RequestTimeoutFactor
        {
            get
            {
                return this.requestTimeoutFactor <= 1 ? this.requestTimeoutFactor : RequestTimeoutFactorDefault;
            }

            set
            {
                this.requestTimeoutFactor = value;
            }
        }

        internal TimeSpan RequestTimeout
        {
            get
            {
                return TimeSpan.FromSeconds(this.requestTimeoutFactor * this.OperationTimeout.TotalSeconds);
            }
        }
    }
}