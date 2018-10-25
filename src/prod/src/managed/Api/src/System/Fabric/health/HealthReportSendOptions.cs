// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Represents the send options that are applied when sending a <see cref="System.Fabric.Health.HealthReport"/>.</para>
    /// </summary>
    /// <remarks>
    /// <para>The report can be sent to the health store using
    /// <see cref="System.Fabric.FabricClient.HealthClient.ReportHealth(System.Fabric.Health.HealthReport)" />.</para>
    /// </remarks>
    public sealed class HealthReportSendOptions
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.HealthReportSendOptions" /> class.</para>
        /// </summary>
        /// <remarks>By default, there are no send options set. The fabric client settings are used to determine when to send the report and when to retry send on failure.</remarks>
        public HealthReportSendOptions()
        {
        }

        /// <summary>
        /// Gets or sets the flag which indicates whether the report should be sent immediately.
        /// Defaults to false, in which case the report is sent per the fabric client health report related settings.
        /// </summary>
        /// <value>A flag which indicates whether the report should be sent immediately.</value>
        /// <remarks>
        /// <para>
        /// If <languageKeyword>true</languageKeyword>, the report is sent immediately, regardless of the
        /// <see cref="System.Fabric.FabricClientSettings.HealthReportSendInterval"/> configuration set on the health client.
        /// This is useful for critical reports that should be sent as soon as possible.
        /// Another scenario where this may be useful is if the client needs to be closed, for example because the host process is going down, and you need to increase the chances of the report being sent.
        /// Depending on timing and other conditions, sending the report may still fail, either because the client doesn't have time to send it before shutdown,
        /// or because the message is lost and the health client went down before it can retry.
        /// </para>
        /// <para>
        /// If <languageKeyword>false</languageKeyword>, the report is sent based on the health client settings, especially the
        /// <see cref="System.Fabric.FabricClientSettings.HealthReportSendInterval"/> configuration.
        /// </para>
        /// <para>
        /// By default, reports are not sent immediately.
        /// This is the recommended setting because it allows the health client to optimize health reporting messages to health store as well as health report processing. 
        /// </para>
        /// </remarks>
        public bool Immediate
        {
            get;
            set;
        }
        
        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeSendOptions = new NativeTypes.FABRIC_HEALTH_REPORT_SEND_OPTIONS();
            nativeSendOptions.Immediate = NativeTypes.ToBOOLEAN(this.Immediate);
            
            return pinCollection.AddBlittable(nativeSendOptions);
        }
    }
}