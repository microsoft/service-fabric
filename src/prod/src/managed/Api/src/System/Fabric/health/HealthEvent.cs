// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>Represents health information reported on a health entity, such as cluster, application or node,
    /// with additional metadata added by the Health Manager.</para>
    /// </summary>
    /// <remarks>Health events are returned by health queries such as
    /// <see cref="System.Fabric.FabricClient.HealthClient.GetClusterHealthAsync(System.Fabric.Description.ClusterHealthQueryDescription)"/>.
    /// They contain <see cref="System.Fabric.Health.HealthInformation"/> sent to Health Manager in a <see cref="System.Fabric.Health.HealthReport"/>.</remarks>
    public sealed class HealthEvent
    {
        internal HealthEvent()
        {
        }

        /// <summary>
        /// <para>Gets the date and time when the health report was sent by the source.</para>
        /// </summary>
        /// <value>
        /// <para>The date and time when the health report was sent by the source.</para>
        /// </value>
        public DateTime SourceUtcTimestamp { get; internal set; }

        /// <summary>
        /// <para>Gets the date and time when the health report was last modified by the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The date and time when the health report was last modified by the health store.</para>
        /// </value>
        public DateTime LastModifiedUtcTimestamp { get; internal set; }

        /// <summary>
        /// <para>Gets a value that indicates whether the health event has expired.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the health event has expired;
        ///     <languageKeyword>false</languageKeyword> if the health event was not expired at the time the health store evaluated the query.</para>
        /// </value>
        /// <remarks>
        /// <para>An event can be expired only if RemoveWhenExpired is false.
        /// Otherwise, the event is not returned by query and is removed from the store.
        /// </para>
        /// </remarks>
        public bool IsExpired { get; internal set; }

        /// <summary>
        /// <para>If the current <see cref="System.Fabric.Health.HealthInformation.HealthState" /> is <see cref="System.Fabric.Health.HealthState.Ok" />, 
        /// returns the time at which the health report was first reported with <see cref="System.Fabric.Health.HealthState.Ok" />. For periodic reporting, 
        /// many reports with the same state may have been generated.</para>
        /// <para>If the current <see cref="System.Fabric.Health.HealthInformation.HealthState" /> is <see cref="System.Fabric.Health.HealthState.Error" /> 
        /// or <see cref="System.Fabric.Health.HealthState.Warning" />, returns the time at which the health state was last in 
        /// <see cref="System.Fabric.Health.HealthState.Ok" />, before transitioning to a different state. If the <see cref="System.Fabric.Health.HealthInformation.HealthState" /> 
        /// has never been <see cref="System.Fabric.Health.HealthState.Ok" />, the value will be System.DateTime.FromFileTimeUtc(0).</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.DateTime" /> representing the last transition time (UTC) involving <see cref="System.Fabric.Health.HealthState.Ok" />.</para>
        /// </value>
        /// <remarks>
        /// <para>The transition fields, <see cref="System.Fabric.Health.HealthEvent.LastOkTransitionAt"/>, <see cref="System.Fabric.Health.HealthEvent.LastWarningTransitionAt"/>, <see cref="System.Fabric.Health.HealthEvent.LastErrorTransitionAt"/> give the history of the health state transitions for the event.
        /// They can be used for smarter alerts or "historical" health event information. They enable scenarios such as:
        /// <list type="bullet">
        ///     <item>
        ///         <para>Alert when a property has been at warning/error for more than X minutes.
        ///         This avoids alerts on temporary conditions. For example, an alert if the health state has been warning for more than five minutes can be translated into 
        ///         (HealthState == Warning and Now - LastWarningTransitionTime > 5 minutes).</para>
        ///     </item>
        ///     <item>
        ///         <para>Alert only on conditions that have changed in the last X minutes.
        ///         If a report was already at error before the specified time, it can be ignored because it was already signaled previously.</para>
        ///     </item>
        ///     <item>
        ///         <para>If a property is toggling between warning and error, determine how long it has been unhealthy (i.e. not OK). 
        ///         For example, an alert if the property hasn't been healthy for more than five minutes can be translated into (HealthState != Ok and Now - LastOkTransitionTime > 5 minutes).</para>
        ///     </item>
        /// </list>
        /// </para>
        /// </remarks>
        public DateTime LastOkTransitionAt { get; internal set; }

        /// <summary>
        /// <para>If the current <see cref="System.Fabric.Health.HealthInformation.HealthState" /> is <see cref="System.Fabric.Health.HealthState.Warning" />, 
        /// returns the time at which the health report was first reported with <see cref="System.Fabric.Health.HealthState.Warning" />. For periodic reporting, 
        /// many reports with the same state may have been generated.</para>
        /// <para>If the current <see cref="System.Fabric.Health.HealthInformation.HealthState" /> is <see cref="System.Fabric.Health.HealthState.Ok" /> or 
        /// <see cref="System.Fabric.Health.HealthState.Error" />, returns the time at which the health state was last in <see cref="System.Fabric.Health.HealthState.Warning" />, 
        /// before transitioning to a different state. If the <see cref="System.Fabric.Health.HealthInformation.HealthState" /> has never been 
        /// <see cref="System.Fabric.Health.HealthState.Warning" />, the value will be System.DateTime.FromFileTimeUtc(0).</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.DateTime" /> representing the last transition time (UTC) involving <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
        /// </value>
        /// <remarks>
        /// <para>The transition fields, <see cref="System.Fabric.Health.HealthEvent.LastOkTransitionAt"/>, <see cref="System.Fabric.Health.HealthEvent.LastWarningTransitionAt"/>, <see cref="System.Fabric.Health.HealthEvent.LastErrorTransitionAt"/> give the history of the health state transitions for the event.
        /// They can be used for smarter alerts or "historical" health event information. They enable scenarios such as:
        /// <list type="bullet">
        ///     <item>
        ///         <para>Alert when a property has been at warning/error for more than X minutes.
        ///         This avoids alerts on temporary conditions. For example, an alert if the health state has been warning for more than five minutes can be translated into 
        ///         (HealthState == Warning and Now - LastWarningTransitionTime > 5 minutes).</para>
        ///     </item>
        ///     <item>
        ///         <para>Alert only on conditions that have changed in the last X minutes.
        ///         If a report was already at error before the specified time, it can be ignored because it was already signaled previously.</para>
        ///     </item>
        ///     <item>
        ///         <para>If a property is toggling between warning and error, determine how long it has been unhealthy (i.e. not OK). 
        ///         For example, an alert if the property hasn't been healthy for more than five minutes can be translated into (HealthState != Ok and Now - LastOkTransitionTime > 5 minutes).</para>
        ///     </item>
        /// </list>
        /// </para>
        /// </remarks>
        public DateTime LastWarningTransitionAt { get; internal set; }

        /// <summary>
        /// <para>If the current <see cref="System.Fabric.Health.HealthInformation.HealthState" /> is <see cref="System.Fabric.Health.HealthState.Error" />, returns 
        /// the time at which the health report was first reported with <see cref="System.Fabric.Health.HealthState.Error" />. For periodic reporting, many reports 
        /// with the same state may have been generated.</para>
        /// <para>If the current <see cref="System.Fabric.Health.HealthInformation.HealthState" /> is <see cref="System.Fabric.Health.HealthState.Ok" /> or 
        /// <see cref="System.Fabric.Health.HealthState.Warning" />, returns the time at which the health state was last in <see cref="System.Fabric.Health.HealthState.Error" />, 
        /// before transitioning to a different state. If the <see cref="System.Fabric.Health.HealthInformation.HealthState" /> has never been 
        /// <see cref="System.Fabric.Health.HealthState.Error" />, the value will be System.DateTime.FromFileTimeUtc(0).</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.DateTime" /> representing the last transition time (UTC) involving <see cref="System.Fabric.Health.HealthState.Error" />.</para>
        /// </value>
        /// <remarks>
        /// <para>The transition fields, <see cref="System.Fabric.Health.HealthEvent.LastOkTransitionAt"/>, <see cref="System.Fabric.Health.HealthEvent.LastWarningTransitionAt"/>, <see cref="System.Fabric.Health.HealthEvent.LastErrorTransitionAt"/> give the history of the health state transitions for the event.
        /// They can be used for smarter alerts or "historical" health event information. They enable scenarios such as:
        /// <list type="bullet">
        ///     <item>
        ///         <para>Alert when a property has been at warning/error for more than X minutes.
        ///         This avoids alerts on temporary conditions. For example, an alert if the health state has been warning for more than five minutes can be translated into 
        ///         (HealthState == Warning and Now - LastWarningTransitionTime > 5 minutes).</para>
        ///     </item>
        ///     <item>
        ///         <para>Alert only on conditions that have changed in the last X minutes.
        ///         If a report was already at error before the specified time, it can be ignored because it was already signaled previously.</para>
        ///     </item>
        ///     <item>
        ///         <para>If a property is toggling between warning and error, determine how long it has been unhealthy (i.e. not OK). 
        ///         For example, an alert if the property hasn't been healthy for more than five minutes can be translated into (HealthState != Ok and Now - LastOkTransitionTime > 5 minutes).</para>
        ///     </item>
        /// </list>
        /// </para>
        /// </remarks>
        public DateTime LastErrorTransitionAt { get; internal set; }

        /// <summary>
        /// <para>Gets the health information that was sent to health store in a <see cref="System.Fabric.Health.HealthReport"/>.</para>
        /// </summary>
        /// <value>
        /// <para>The health information that was sent to health store in a <see cref="System.Fabric.Health.HealthReport"/>.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public HealthInformation HealthInformation { get; internal set; }

        /// <summary>
        /// Gets a string representation of the health event.
        /// </summary>
        /// <returns>A string representation of the health event.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}: IsExpired {1}",
                this.HealthInformation.ToString(),
                this.IsExpired);
        }

        internal static unsafe HealthEvent FromNative(NativeTypes.FABRIC_HEALTH_EVENT nativeHealthEvent)
        {
            var healthEvent = new HealthEvent();

            healthEvent.HealthInformation = HealthInformation.FromNative(nativeHealthEvent.HealthInformation);
            healthEvent.SourceUtcTimestamp = NativeTypes.FromNativeFILETIME(nativeHealthEvent.SourceUtcTimestamp);
            healthEvent.LastModifiedUtcTimestamp = NativeTypes.FromNativeFILETIME(nativeHealthEvent.LastModifiedUtcTimestamp);
            healthEvent.IsExpired = NativeTypes.FromBOOLEAN(nativeHealthEvent.IsExpired);

            if (nativeHealthEvent.Reserved != IntPtr.Zero)
            {
                var nativeHealthEventEx1 = (NativeTypes.FABRIC_HEALTH_EVENT_EX1*)(nativeHealthEvent.Reserved);
                healthEvent.LastOkTransitionAt = NativeTypes.FromNativeFILETIME(nativeHealthEventEx1->LastOkTransitionAt);
                healthEvent.LastWarningTransitionAt = NativeTypes.FromNativeFILETIME(nativeHealthEventEx1->LastWarningTransitionAt);
                healthEvent.LastErrorTransitionAt = NativeTypes.FromNativeFILETIME(nativeHealthEventEx1->LastErrorTransitionAt);
            }

            return healthEvent;
        }

        internal static unsafe List<HealthEvent> FromNativeList(IntPtr nativeHealthEventsPtr)
        {
            List<HealthEvent> events = new List<HealthEvent>();
            if (nativeHealthEventsPtr != IntPtr.Zero)
            {
                NativeTypes.FABRIC_HEALTH_EVENT_LIST* nativeEvents = (NativeTypes.FABRIC_HEALTH_EVENT_LIST*)nativeHealthEventsPtr;
                for (int i = 0; i < nativeEvents->Count; i++)
                {
                    var nativeEvent = (NativeTypes.FABRIC_HEALTH_EVENT*)((ulong)nativeEvents->Items + (ulong)(i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_HEALTH_EVENT))));
                    events.Add(HealthEvent.FromNative(*nativeEvent));
                }
            }

            return events;
        }

        internal static bool AreEqual(HealthEvent a, HealthEvent b)
        {
            if (a == b)
            {
                // References are equal/ both are null
                return true;
            }

            if (a == null || b == null)
            {
                // One is null and one is non-null
                return false;
            }

            bool result =
                a.IsExpired == b.IsExpired
                && a.SourceUtcTimestamp == b.SourceUtcTimestamp
                && a.LastModifiedUtcTimestamp == b.LastModifiedUtcTimestamp
                && a.LastOkTransitionAt == b.LastOkTransitionAt
                && a.LastWarningTransitionAt == b.LastWarningTransitionAt
                && a.LastErrorTransitionAt == b.LastErrorTransitionAt;

            result &= HealthInformation.AreEqual(a.HealthInformation, b.HealthInformation);

            return result;
        }
    }
}