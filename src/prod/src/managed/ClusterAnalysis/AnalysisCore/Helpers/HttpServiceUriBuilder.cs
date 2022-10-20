// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Helpers
{
    using System;
    using System.Globalization;
    using System.Linq;

    internal class HttpServiceUriBuilder
    {
        private const string Http = "http";

        private const string Fabric = "fabric";

        public HttpServiceUriBuilder()
        {
        }

        public HttpServiceUriBuilder(string uri)
            : this(new Uri(uri, UriKind.Absolute))
        {
        }

        public HttpServiceUriBuilder(Uri uri)
        {
            this.ServiceName = new Uri("fabric:" + uri.AbsolutePath.TrimEnd('/'));
            string path = uri.Fragment.Remove(0, 2);
            string[] segments = path.Split('/');
            this.ServiceHostNode = segments[0];
            this.ResourcePathAndQuery = string.Join("/", segments.Skip(1));
        }

        public string ServiceHostNode { get; private set; }

        public Uri ServiceName { get; private set; }

        public string ResourcePathAndQuery { get; private set; }

        public Uri Build()
        {
            if (this.ServiceName == null)
            {
                throw new UriFormatException("Service name is null.");
            }

            UriBuilder builder = new UriBuilder
                                     {
                                         Scheme = Http,
                                         Host = Fabric,
                                         Path = this.ServiceName.AbsolutePath.Trim('/') + '/',
                                         Fragment = string.Format(CultureInfo.InvariantCulture, "/{0}/{1}", this.ServiceHostNode, this.ResourcePathAndQuery)
                                     };

            return builder.Uri;
        }

        public HttpServiceUriBuilder SetsHost(string host)
        {
            this.ServiceHostNode = host.ToLowerInvariant();
            return this;
        }

        /// <summary>
        /// Fully-qualified service name URI: fabric:/name/of/service
        /// </summary>
        /// <param name="serviceName"></param>
        /// <returns></returns>
        public HttpServiceUriBuilder SetServiceName(string serviceName)
        {
            return this.SetServiceName(new Uri(serviceName, UriKind.Absolute));
        }

        /// <summary>
        /// Path to Resource and Query parameters.
        /// </summary>
        /// <param name="servicePathAndQuery"></param>
        /// <returns></returns>
        public HttpServiceUriBuilder SetServicePathAndQuery(string servicePathAndQuery)
        {
            this.ResourcePathAndQuery = servicePathAndQuery;
            return this;
        }

        /// <summary>
        /// Fully-qualified service name URI: fabric:/name/of/service
        /// </summary>
        /// <param name="serviceName"></param>
        /// <returns></returns>
        private HttpServiceUriBuilder SetServiceName(Uri serviceName)
        {
            if (serviceName != null)
            {
                if (!serviceName.IsAbsoluteUri)
                {
                    throw new UriFormatException("Service URI must be an absolute URI in the form 'fabric:/name/of/service");
                }

                if (!string.Equals(serviceName.Scheme, "fabric", StringComparison.OrdinalIgnoreCase))
                {
                    throw new UriFormatException("Scheme must be 'fabric'.");
                }
            }

            this.ServiceName = serviceName;

            return this;
        }
    }
}