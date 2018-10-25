// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Net.Http;

    internal static class HttpRequestExtensions
    {
        public static string GetSchemeProperty(this HttpRequestMessage request)
        {
            return request.GetProperty<string>("url.Scheme");
        }

        public static void SetSchemeProperty(this HttpRequestMessage request, string scheme)
        {
            request.SetProperty("url.Scheme", scheme);
        }

        public static string GetHostProperty(this HttpRequestMessage request)
        {
            return request.GetProperty<string>("url.Host");
        }

        public static void SetHostProperty(this HttpRequestMessage request, string host)
        {
            request.SetProperty("url.Host", host);
        }

        public static int? GetPortProperty(this HttpRequestMessage request)
        {
            return request.GetProperty<int?>("url.Port");
        }

        public static void SetPortProperty(this HttpRequestMessage request, int? port)
        {
            request.SetProperty("url.Port", port);
        }

        public static string GetConnectionHostProperty(this HttpRequestMessage request)
        {
            return request.GetProperty<string>("url.ConnectionHost");
        }

        public static void SetConnectionHostProperty(this HttpRequestMessage request, string host)
        {
            request.SetProperty("url.ConnectionHost", host);
        }

        public static int? GetConnectionPortProperty(this HttpRequestMessage request)
        {
            return request.GetProperty<int?>("url.ConnectionPort");
        }

        public static void SetConnectionPortProperty(this HttpRequestMessage request, int? port)
        {
            request.SetProperty("url.ConnectionPort", port);
        }

        public static string GetPathAndQueryProperty(this HttpRequestMessage request)
        {
            return request.GetProperty<string>("url.PathAndQuery");
        }

        public static void SetPathAndQueryProperty(this HttpRequestMessage request, string pathAndQuery)
        {
            request.SetProperty("url.PathAndQuery", pathAndQuery);
        }

        public static string GetAddressLineProperty(this HttpRequestMessage request)
        {
            return request.GetProperty<string>("url.AddressLine");
        }

        public static void SetAddressLineProperty(this HttpRequestMessage request, string addressLine)
        {
            request.SetProperty("url.AddressLine", addressLine);
        }

        public static T GetProperty<T>(this HttpRequestMessage request, string key)
        {
            object obj;
            if (request.Properties.TryGetValue(key, out obj))
            {
                return (T)obj;
            }
            return default(T);
        }

        public static void SetProperty<T>(this HttpRequestMessage request, string key, T value)
        {
            request.Properties[key] = value;
        }
    }
}