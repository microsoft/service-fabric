// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using Newtonsoft.Json.Linq;
    using System;
    using System.Collections.Generic;
    using System.Text;

    /// <summary>
    ///  This class should be same as the one in 
    ///  RP branch
    /// </summary>
    public class StreamRequest
    {
        public string Method { get; set; }

        #region Content
        public string Content { get; set; }

        public string MediaType { get; set; }

        public string CharSet { get; set; }
        #endregion

        public string RedirectPathAndQuery { get; set; }

        public string ClusterQueryUri { get; set; }

        public IDictionary<string, string> Headers { get; set; }

        public string RequestId { get; set; }

        public DateTime RequestReceivedStamp { get; set; }

        public bool PingRequest { get; set; }

        #region ToString
        public override string ToString()
        {
            return $"StreamRequest: {JObject.FromObject(this).ToString()}";
        }
        #endregion

        public StreamRequest Replace(string host, int port)
        {
            var copy = (StreamRequest)this.MemberwiseClone();

            Uri queryUri = null;
            if (!Uri.TryCreate(copy.ClusterQueryUri, UriKind.Absolute, out queryUri))
            {
                throw new ArgumentException(copy.ClusterQueryUri);
            }
            
            var uriBuilder = new UriBuilder(queryUri.Scheme, host, port, queryUri.AbsolutePath);
            uriBuilder.Query = queryUri.Query.TrimStart('?');

            copy.ClusterQueryUri = uriBuilder.Uri.ToString();

            return copy;

        }
    }
}