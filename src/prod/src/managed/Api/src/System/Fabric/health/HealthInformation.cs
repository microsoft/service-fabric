// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Represents common health report information.
    /// It is included in all health reports sent to the health store
    /// and in all health events returned by health queries.</para>
    /// </summary>
    public sealed class HealthInformation
    {
        /// <summary>
        /// Unknown sequence number, which is an invalid sequence number that is not accepted by the health store.
        /// </summary>
        public const long UnknownSequenceNumber = NativeTypes.FABRIC_INVALID_SEQUENCE_NUMBER;

        /// <summary>
        /// Auto sequence number, replaced with a valid sequence number by the health client.
        /// </summary>
        /// <remarks>When a health client receives a report with Auto sequence number,
        /// it replaces the auto sequence number with a valid sequence number.
        /// The sequence number is guaranteed to increase in the same process.</remarks>
        public const long AutoSequenceNumber = NativeTypes.FABRIC_AUTO_SEQUENCE_NUMBER;

        private TimeSpan timeToLive;

        private long sequenceNumber;

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.HealthInformation" /> class.</para>
        /// </summary>
        /// <param name="sourceId">
        /// <para>The source of the report. It cannot be <languageKeyword>null</languageKeyword> or empty.
        /// It can't start with "System.", which is reserved keyword for system components reporting.</para>
        /// </param>
        /// <param name="property">
        /// <para>The property of the report. It cannot be <languageKeyword>null</languageKeyword> or empty.</para>
        /// </param>
        /// <param name="healthState">
        /// <para>The health state of the report. Must be specified.
        /// Must be one of the <see cref="System.Fabric.Health.HealthState.Error" />, 
        /// <see cref="System.Fabric.Health.HealthState.Warning" /> or <see cref="System.Fabric.Health.HealthState.Ok" /> values.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>
        ///     <paramref name="sourceId" /> cannot be <languageKeyword>null</languageKeyword>.</para>
        /// </exception>
        /// <exception cref="System.ArgumentNullException">
        /// <para>
        ///     <paramref name="property" /> cannot be <languageKeyword>null</languageKeyword>.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <paramref name="sourceId" /> cannot be empty.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <paramref name="property" /> cannot be empty.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>Specified <paramref name="healthState" /> is not supported.</para>
        /// </exception>
        public HealthInformation(string sourceId, string property, HealthState healthState)
        {
            Requires.Argument<string>("sourceId", sourceId).NotNullOrEmpty();
            Requires.Argument<string>("property", property).NotNullOrEmpty();
            if ((healthState == HealthState.Invalid) || (healthState == HealthState.Unknown))
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidHealthState_Formatted, healthState), "healthState");
            }

            this.SourceId = sourceId;
            this.Property = property;
            this.HealthState = healthState;
            this.TimeToLive = TimeSpan.MaxValue;
            this.SequenceNumber = AutoSequenceNumber;
        }

        internal HealthInformation()
        {
            this.TimeToLive = TimeSpan.MaxValue;
            this.SequenceNumber = AutoSequenceNumber;
        }

        /// <summary>
        /// <para>Gets the source name which identifies the watchdog/system component 
        /// which generated the health information.</para>
        /// </summary>
        /// <value>
        /// <para>The source of the health report, which identifies the watchdog/system component that creates the report.</para>
        /// </value>
        public string SourceId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the property of the health report.</para>
        /// </summary>
        /// <value>
        /// <para>The property of the health report. 
        /// Together with the <see cref="System.Fabric.Health.HealthInformation.SourceId"/>, it uniquely 
        /// identifies the health information.</para>
        /// </value>
        /// <remarks>
        /// <para>
        /// The property is a <see cref="System.String"/> and not a fixed enumeration to allow the reporter
        /// flexibility to categorize the state condition that triggers the report.
        /// For example, a reporter with <see cref="System.Fabric.Health.HealthInformation.SourceId"/> "A" can monitor the state of the available disk on a node,
        /// so it can report "AvailableDisk" property on that node.
        /// A reporter with <see cref="System.Fabric.Health.HealthInformation.SourceId"/> "B" can monitor the node connectivity, so it can report
        /// a property "Connectivity" on the same node.
        /// In the health store, these reports are treated as separate health events for the specified node.
        /// </para>
        /// </remarks>
        public string Property
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the health state that describes the severity of the monitored condition used for reporting.</para>
        /// </summary>
        /// <value>
        /// <para>The health state that describes the severity of the monitored condition used for reporting.</para>
        /// </value>
        /// <remarks>
        /// <para>
        /// The accepted health states are <see cref="System.Fabric.Health.HealthState.Ok"/>,
        /// <see cref="System.Fabric.Health.HealthState.Warning"/> and <see cref="System.Fabric.Health.HealthState.Error"/>.
        /// </para>
        /// </remarks>
        public HealthState HealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets or sets the description of the health information. 
        /// It represents free text used to add human readable information about the monitored condition.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.String" /> which describes the health information in human readable form.</para>
        /// </value>
        /// <remarks>
        /// <para>The maximum string length for the description is 4096 characters.
        /// If the provided string is longer, it will be automatically truncated.
        /// When truncated, the last characters of the description contain a marker "[Truncated]", and total string size is 4096 characters.
        /// The presence of the marker indicates to users that truncation occurred.
        /// Note that when truncated, the description has less than 4096 characters from the original string.
        /// </para>
        /// </remarks>
        public string Description
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets how long the health report is valid. Must be larger than TimeSpan.Zero.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.TimeSpan" /> representing the time to live of the health report.</para>
        /// </value>
        /// <remarks><para>When clients report periodically, they should send reports 
        /// with higher frequency than time to live.
        /// If clients report on transition, they can set the time to live to infinite.</para>
        /// <para>When time to live expires, the health event that contains the health information
        /// is either removed from health store, if <see cref="System.Fabric.Health.HealthInformation.RemoveWhenExpired"/> is 
        /// <languageKeyword>true</languageKeyword> or evaluated at error, if <see cref="System.Fabric.Health.HealthInformation.RemoveWhenExpired"/> is <languageKeyword>false</languageKeyword>.
        /// </para></remarks>
        /// <exception cref="System.ArgumentException">
        /// <para>The specified value was invalid. Please provide a duration that is larger than 0.</para>
        /// </exception>
        [JsonCustomization(PropertyName = "TimeToLiveInMilliSeconds")]
        public TimeSpan TimeToLive
        {
            get
            {
                return this.timeToLive;
            }

            set
            {
                if (value <= TimeSpan.Zero)
                {
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidHealthTimeToLive_Formatted, value), "value");
                }

                this.timeToLive = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets a value that indicates whether the report is removed from health store when it expires. 
        /// If set to <languageKeyword>false</languageKeyword>, the 
        /// report is treated as an error when expired. <languageKeyword>false</languageKeyword> by default.</para>
        /// </summary>
        /// <value>
        /// <para>
        /// <languageKeyword>true</languageKeyword> if the report should be removed from health store when expired;
        /// otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        /// <remarks><para>When clients report periodically, they should set <see cref="System.Fabric.Health.HealthInformation.RemoveWhenExpired"/> <languageKeyword>false</languageKeyword> (default).
        /// This way, is the reporter has issues (eg. deadlock) and can't report, 
        /// the entity is evaluated at error when the health report
        /// expires, and this will flag the entity as <see cref="System.Fabric.Health.HealthState.Error"/>.
        /// Periodic health clients should send reports 
        /// with higher frequency than time to live to account for delays due to health client batching, 
        /// message transport delays over the wire and health store processing.</para></remarks>
        public bool RemoveWhenExpired
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the sequence number associated with the health information,
        /// used by the health store for staleness detection.
        /// Must be greater than <see cref="System.Fabric.Health.HealthInformation.UnknownSequenceNumber" />.</para>
        /// </summary>
        /// <value>
        /// <para>The report sequence number associated with the health information.</para>
        /// </value>
        /// <remarks>
        /// <para>The report sequence number is used by health store to detect stale reports.
        /// </para>
        /// <para>Most of the times, the reporter doesn't need to specify the sequence number. The default value
        /// <see cref="System.Fabric.Health.HealthInformation.AutoSequenceNumber"/> can be used instead. When a health client receives a report with Auto sequence number,
        /// it generates a valid sequence number, which is guaranteed to increase in the same process.</para>
        /// </remarks>
        public long SequenceNumber
        {
            get
            {
                return this.sequenceNumber;
            }

            set
            {
                if (value < AutoSequenceNumber)
                {
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidSequenceNumber_Formatted, value), "value");
                }

                this.sequenceNumber = value;
            }
        }

        /// <summary>
        /// Creates a string description of the health information.
        /// </summary>
        /// <returns>String description of the health information.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "SourceId: '{0}', Property: '{1}', {2}, \"{3}\", TimeToLive {4}, RemoveWhenExpired {5}, SequenceNumber {6}",
                this.SourceId,
                this.Property,
                this.HealthState,
                this.Description,
                this.TimeToLive,
                this.RemoveWhenExpired,
                this.SequenceNumber);
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeHealthInformation = new NativeTypes.FABRIC_HEALTH_INFORMATION();

            nativeHealthInformation.Property = pin.AddObject(this.Property);
            nativeHealthInformation.SourceId = pin.AddObject(this.SourceId);
            nativeHealthInformation.State = (NativeTypes.FABRIC_HEALTH_STATE)this.HealthState;
            nativeHealthInformation.Description = pin.AddObject(this.Description);

            if (this.TimeToLive == TimeSpan.MaxValue)
            {
                nativeHealthInformation.TimeToLiveSeconds = NativeTypes.FABRIC_HEALTH_REPORT_INFINITE_TTL;
            }
            else
            {
                nativeHealthInformation.TimeToLiveSeconds = (uint)Math.Ceiling(this.TimeToLive.TotalSeconds);
            }
            nativeHealthInformation.RemoveWhenExpired = NativeTypes.ToBOOLEAN(this.RemoveWhenExpired);
            nativeHealthInformation.SequenceNumber = this.SequenceNumber;

            return pin.AddBlittable(nativeHealthInformation);
        }

        internal static unsafe HealthInformation FromNative(IntPtr nativeHealthInformationPtr)
        {
            var nativeHealthInformation = *(NativeTypes.FABRIC_HEALTH_INFORMATION*)nativeHealthInformationPtr;
            return FromNative(nativeHealthInformation);
        }

        internal static unsafe HealthInformation FromNative(NativeTypes.FABRIC_HEALTH_INFORMATION nativeHealthInformation)
        {
            var healthInformation = new HealthInformation(
                    NativeTypes.FromNativeString(nativeHealthInformation.SourceId),
                    NativeTypes.FromNativeString(nativeHealthInformation.Property),
                    (HealthState)nativeHealthInformation.State);

            healthInformation.Description = NativeTypes.FromNativeString(nativeHealthInformation.Description);

            if (nativeHealthInformation.TimeToLiveSeconds == NativeTypes.FABRIC_HEALTH_REPORT_INFINITE_TTL)
            {
                healthInformation.TimeToLive = TimeSpan.MaxValue;
            }
            else
            {
                healthInformation.TimeToLive = TimeSpan.FromSeconds(nativeHealthInformation.TimeToLiveSeconds);
            }

            healthInformation.SequenceNumber = nativeHealthInformation.SequenceNumber;
            healthInformation.RemoveWhenExpired = NativeTypes.FromBOOLEAN(nativeHealthInformation.RemoveWhenExpired);

            return healthInformation;
        }

        internal static bool AreEqual(HealthInformation a, HealthInformation b)
        {
            if (a == b)
            {
                // References are equal or both are null
                return true;
            }

            if (a == null || b == null)
            {
                // One is null and one is non-null
                return false;
            }

            bool result =
                string.Equals(a.Description, b.Description)
                && a.HealthState == b.HealthState
                && string.Equals(a.Property, b.Property)
                && a.RemoveWhenExpired == b.RemoveWhenExpired
                && a.SequenceNumber == b.SequenceNumber
                && string.Equals(a.SourceId, b.SourceId)
                && a.TimeToLive == b.TimeToLive;

            return result;
        }
    }
}