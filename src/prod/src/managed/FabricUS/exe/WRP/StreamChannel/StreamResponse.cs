// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System;
    using System.Collections.Generic;
    using System.Net;
    using System.Text;

    public class StreamResponse
    {
        private string reasonPhrase;

        public string RedirectPathAndQuery { get; set; }

        public IDictionary<string, string> Headers { get; set; }

        #region Content
        public string Content { get; set; }

        public string MediaType { get; set; }

        public string CharSet { get; set; }

        #endregion

        public HttpStatusCode StatusCode { get; set; }

        public string ReasonPhrase
        {
            get { return this.reasonPhrase; }
            set { this.reasonPhrase = value == null ? null : value.Replace(Environment.NewLine, string.Empty); }
        }

        public string RequestId { get; set; }

        public DateTime RequestReceivedStamp { get; set; }

        public override string ToString()
        {
            var headers = new StringBuilder();
            if (this.Headers != null)
            {
                foreach (var headerKvp in this.Headers)
                {
                    headers.Append(string.Format(" {0}-{1} ", headerKvp.Key, headerKvp.Value));
                }
            }

            return string.Format(
                "StreamResponse: RedirectPathAndQuery:{0},Headers:{1},Content:{2},MediaType:{3},CharSet:{4},StatusCode:{5},ReasonPhrase:{6},RequestId:{7}",
                this.RedirectPathAndQuery,
                headers,
                this.Content,
                this.MediaType,
                this.CharSet,
                this.StatusCode,
                this.ReasonPhrase,
                this.RequestId);
        }
    }
}