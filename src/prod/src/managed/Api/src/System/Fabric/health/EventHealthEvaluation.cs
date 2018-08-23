// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Represents health evaluation of a <see cref="System.Fabric.Health.HealthEvent" />. 
    /// Can be returned when evaluating health of an 
    /// entity returns <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
    /// </summary>
    public sealed class EventHealthEvaluation : HealthEvaluation
    {
        internal EventHealthEvaluation()
            : base(HealthEvaluationKind.Event)
        {
        }

        /// <summary>
        /// <para>Gets the unhealthy event details.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.HealthEvent" /> that led to current aggregated health state.</para>
        /// </value>
        public HealthEvent UnhealthyEvent
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets <see cref="System.Boolean" /> that indicates whether warnings are treated 
        /// with the same severity as errors. The field is 
        /// specified in the health policy used to evaluate the entity.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if warnings are treated as errors; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool ConsiderWarningAsError
        {
            get;
            internal set;
        }

        internal static unsafe EventHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_EVENT_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new EventHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;

            ReleaseAssert.AssertIf(nativeHealthEvaluation.UnhealthyEvent == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "UnhealthyEvent"));
            var nativeHealthEvent = (NativeTypes.FABRIC_HEALTH_EVENT*)nativeHealthEvaluation.UnhealthyEvent;
            managedHealthEvaluation.UnhealthyEvent = HealthEvent.FromNative(*nativeHealthEvent);

            managedHealthEvaluation.ConsiderWarningAsError = NativeTypes.FromBOOLEAN(nativeHealthEvaluation.ConsiderWarningAsError);

            return managedHealthEvaluation;
        }
    }
}