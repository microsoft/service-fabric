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

    /// <summary>
    /// <para>Describes the endpoint resource.</para>
    /// </summary>
    public sealed class EndpointResourceDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.Description.EndpointResourceDescription"/>.</para>
        /// </summary>
        public EndpointResourceDescription()
        {
        }

        /// <summary>
        /// <para>Gets or sets the name of the endpoint.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the endpoint.</para>
        /// </value>
        public string Name { get; set; }

        /// <summary>
        /// <para>Gets the protocol used by this endpoint.</para>
        /// </summary>
        /// <value>
        /// <para>The protocol used by this endpoint.</para>
        /// </value>
        public EndpointProtocol Protocol { get; set; }

        /// <summary>
        /// <para>Gets the type of the endpoint.</para>
        /// </summary>
        /// <value>
        /// <para>The type of the endpoint.</para>
        /// </value>
        public EndpointType EndpointType { get; set; }

        /// <summary>
        /// <para>Do not use. This property is not supported.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string Certificate { get; set; }

        /// <summary>
        /// <para>Gets the port assigned for this endpoint.</para>
        /// </summary>
        /// <value>
        /// <para>The port assigned for this endpoint.</para>
        /// </value>
        public int Port { get; internal set; }

        /// <summary>
        /// <para>Gets the Uri scheme for the endpoint.</para>
        /// </summary>
        /// <value>
        /// <para>The uri scheme for endpoint like http, https ftp.</para>
        /// </value>
        public string UriScheme { get; set; }

        /// <summary>
        /// <para>Gets the Path suffix for the endpoint.</para>
        /// </summary>
        /// <value>
        /// <para>The path suffix for endpoint like /myapp1.</para>
        /// </value>
        public string PathSuffix { get; set; }

        /// <summary>
        /// <para>
        /// Gets the name of code package that was specified in the CodePackageRef attribute
        /// of endpoint resource in service manifest.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        /// <remarks>
        /// If no CodePackageRef attribute was specified in endpoint resource in service manifest,
        /// its value is empty string.
        /// </remarks>
        public string CodePackageName { get; set; }

        /// <summary>
        /// <para>
        /// Gets the IP address associated with this endpoint resource.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>Returns he IP address associated with this endpoint resource.</para>
        /// </value>
        /// <remarks>
        /// <para>
        /// If CodePackageRef attribute was specified in endpoint resource in service manifest and 
        /// referenced code package <see cref="CodePackageName"/> had specified network settings for
        /// explicit IP address assignment, its value is the IP address that was assigned to this
        /// code package by Service Fabric runtime. For all other cases, its value is the IP address
        /// (or FQDN) of the machine on which service is running.
        /// </para>
        /// </remarks>
        public string IpAddressOrFqdn { get; set; }

        internal static unsafe EndpointResourceDescription CreateFromNative(IntPtr native)
        {
            NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION nativeDescription = *(NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION*)native;

            string scheme = NativeTypes.FromNativeString(nativeDescription.Protocol);
            string name = NativeTypes.FromNativeString(nativeDescription.Name);
            string cert = NativeTypes.FromNativeString(nativeDescription.CertificateName);
            string type = NativeTypes.FromNativeString(nativeDescription.Type);
            EndpointProtocol protocol;
            EndpointType endpointType;
            string uriScheme = null;
            string pathSuffix = null;
            string codepackageName = null;
            string ipAddressOrFqdn = null;

            // validation
            if (string.IsNullOrEmpty(name))
            {
                AppTrace.TraceSource.WriteError("EndpointResourceDescription.CreateFromNative", "Name was null/empty");
                throw new ArgumentException(
                    StringResources.Error_EndpointNameNullOrEmpty,
                    "native");
            }

            if (string.IsNullOrEmpty(scheme))
            {
                AppTrace.TraceSource.WriteError("EndpointResourceDescription.CreateFromNative", "Protocol was null/empty for {0}", name);
                throw new ArgumentException(
                    StringResources.Error_EndpointSchemeNullOrEmpty,
                    "native");
            }

            AppTrace.TraceSource.WriteNoise("EndpointResourceDescription.CreateFromNative", "Found endpoint with name {0}. Protocol {1}. Port {2}", name, scheme, nativeDescription.Port);

            if (scheme == "http")
            {
                protocol = EndpointProtocol.Http;
            }
            else if (scheme == "https")
            {
                protocol = EndpointProtocol.Https;
            }
            else if (scheme == "tcp")
            {
                protocol = EndpointProtocol.Tcp;
            }
            else if (scheme == "udp")
            {
                protocol = EndpointProtocol.Udp;
            }
            else
            {
                AppTrace.TraceSource.WriteError("EndpointResourceDescription.CreateFromNative", "Unknown scheme: {0}", scheme);
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_EndpointNotSupported_Formatted,
                        scheme),
                    "native");
            }

            if (type == "Input")
            {
                endpointType = EndpointType.Input;
            }
            else if (type == "Internal")
            {
                endpointType = EndpointType.Internal;
            }
            else
            {
                AppTrace.TraceSource.WriteError("EndpointResourceDescription.CreateFromNative", "Unknown type: {0}", type);
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_EndpointNotSupported_Formatted,
                        type),
                    "native");
            }

            if (nativeDescription.Reserved != IntPtr.Zero)
            {
                var endpointDescriptionEx1 = (NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX1*) (nativeDescription.Reserved);
                uriScheme = NativeTypes.FromNativeString(endpointDescriptionEx1->UriScheme);
                pathSuffix = NativeTypes.FromNativeString(endpointDescriptionEx1->PathSuffix);

                if (endpointDescriptionEx1->Reserved != IntPtr.Zero)
                {
                    var endpointDescriptionEx2 = (NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX2*)(endpointDescriptionEx1->Reserved);

                    codepackageName = NativeTypes.FromNativeString(endpointDescriptionEx2->CodePackageName);
                    ipAddressOrFqdn = NativeTypes.FromNativeString(endpointDescriptionEx2->IpAddressOrFqdn);
                }

            }
                 return new EndpointResourceDescription
                {
                    Certificate = cert,
                    EndpointType = endpointType,
                    Name = name,
                    Protocol = protocol,
                    Port = (int) nativeDescription.Port,
                    UriScheme = uriScheme,
                    PathSuffix = pathSuffix,
                    CodePackageName = codepackageName,
                    IpAddressOrFqdn = ipAddressOrFqdn
                };
        }
    }
}