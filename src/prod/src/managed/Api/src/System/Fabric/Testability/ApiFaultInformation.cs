// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    using System.Collections.Generic;
    using System.Text;

    public class ApiFaultInformation
    {
        private List<ApiErrorInformation> apiErrorInformation;

        private List<ApiSlowInformation> apiSlowInformation;

        public ApiFaultInformation()
        {
            this.apiErrorInformation = new List<ApiErrorInformation>();

            this.apiSlowInformation = new List<ApiSlowInformation>();
        }

        public List<ApiErrorInformation> ApiErrorInformation
        {
            get
            {
                return apiErrorInformation;
            }
        }

        public List<ApiSlowInformation> ApiSlowInformation
        {
            get
            {
                return apiSlowInformation;
            }
        }

        internal void AddErrorInformation(ApiErrorInformation apiError)
        {
            this.apiErrorInformation.Add(apiError);
        }

        internal void AddSlowInformation(ApiSlowInformation apiSlow)
        {
            this.apiSlowInformation.Add(apiSlow);
        }

        public override string ToString()
        {
            var w = new StringBuilder();

            if(this.apiErrorInformation.Count > 0)
            {
                w.AppendFormat("Api Errors:\n");
                foreach(var apiError in this.apiErrorInformation)
                {
                    w.AppendFormat("{0}\n", apiError.ToString());
                }
            }

            if(this.apiSlowInformation.Count > 0)
            {
                w.AppendFormat("Api Slows:\n");
                foreach(var apiSlow in this.apiSlowInformation)
                {
                    w.AppendFormat("{0}\n", apiSlow.ToString());
                }
            }

            return w.ToString();
        }
    }
}

#pragma warning restore 1591