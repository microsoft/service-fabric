// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Creates a particular correlation between services.</para>
    /// </summary>
    public sealed class ServiceCorrelationDescription
    {
        private Uri serviceName;
        private ServiceCorrelationScheme scheme;

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServiceCorrelationDescription" /> class.</para>
        /// </summary>
        public ServiceCorrelationDescription()
        {
            this.serviceName = null;
            this.scheme = ServiceCorrelationScheme.Invalid;
        }

        internal ServiceCorrelationDescription(ServiceCorrelationDescription other)
        {
            this.ServiceName = other.ServiceName;
            this.Scheme = other.Scheme;
        }

        /// <summary>
        /// <para>Gets or sets the name of the service that you want to establish the correlation relationship with.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service that you want to establish the correlation relationship with.</para>
        /// </value>
        /// <remarks>
        ///   <para />
        /// </remarks>
        public Uri ServiceName
        {
            get
            {
                return this.serviceName;
            }

            set
            {
                this.serviceName = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the <see cref="System.Fabric.Description.ServiceCorrelationScheme" /> which describes the relationship between this 
        /// service and the service specified via <see cref="System.Fabric.Description.ServiceCorrelationDescription.ServiceName" />.</para>
        /// </summary>
        /// <value>
        /// <para>The service correlation scheme.</para>
        /// </value>
        /// <remarks>
        ///   <para />
        /// </remarks>
        public ServiceCorrelationScheme Scheme
        {
            get
            {
                return this.scheme;
            }

            set
            {
                this.scheme = value;
            }
        }

        internal static void Validate(ServiceCorrelationDescription serviceCorrelationDescription)
        {
            Requires.Argument<ServiceCorrelationDescription>("serviceCorrelationDescription", serviceCorrelationDescription).NotNull();
            Requires.Argument<Uri>("serviceCorrelationDescription.Name", serviceCorrelationDescription.ServiceName).NotNullOrWhiteSpace();
        }

        internal static unsafe ServiceCorrelationDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, StringResources.Error_NullNativePointer);

            ServiceCorrelationDescription correlation = new ServiceCorrelationDescription();
            NativeTypes.FABRIC_SERVICE_CORRELATION_DESCRIPTION* casted = (NativeTypes.FABRIC_SERVICE_CORRELATION_DESCRIPTION*)nativePtr;

            correlation.ServiceName = NativeTypes.FromNativeUri(casted->ServiceName);

            switch (casted->Scheme)
            {
                case NativeTypes.FABRIC_SERVICE_CORRELATION_SCHEME.FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY:
                    correlation.Scheme = ServiceCorrelationScheme.Affinity;
                    break;
                case NativeTypes.FABRIC_SERVICE_CORRELATION_SCHEME.FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY:
                    correlation.Scheme = ServiceCorrelationScheme.AlignedAffinity;
                    break;
                case NativeTypes.FABRIC_SERVICE_CORRELATION_SCHEME.FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY:
                    correlation.Scheme = ServiceCorrelationScheme.NonAlignedAffinity;
                    break;
                default:
                    AppTrace.TraceSource.WriteError("ServiceCorrelationDescription.CreateFromNative", "Invalid service correlation scheme {0}", casted->Scheme);
                    ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidServiceCorrelationScheme_Formatted, casted->Scheme));
                    break;
            }

            return correlation;
        }

        internal unsafe void ToNative(PinCollection pin, ref NativeTypes.FABRIC_SERVICE_CORRELATION_DESCRIPTION description)
        {
            description.ServiceName = pin.AddObject(this.ServiceName);

            switch (this.Scheme)
            {
                case ServiceCorrelationScheme.Affinity:
                    description.Scheme = NativeTypes.FABRIC_SERVICE_CORRELATION_SCHEME.FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY;
                    break;
                case ServiceCorrelationScheme.AlignedAffinity:
                    description.Scheme = NativeTypes.FABRIC_SERVICE_CORRELATION_SCHEME.FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY;
                    break;
                case ServiceCorrelationScheme.NonAlignedAffinity:
                    description.Scheme = NativeTypes.FABRIC_SERVICE_CORRELATION_SCHEME.FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY;
                    break;
                default:
                    ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidServiceCorrelationScheme_Formatted, this.Scheme));
                    break;
            }
        }

        /// <summary>
        /// <para> 
        /// Returns a string of the ServiceCorrelationDescription in the form 'ServiceName', 'Scheme'
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the ServiceCorrelationDescription object.</para>
        /// </returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendFormat("{0},", this.ServiceName);
            sb.AppendFormat("{0}", this.Scheme);

            return sb.ToString();
        }
    }
}