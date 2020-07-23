// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Globalization;

    public class ClusterManagementException : Exception
    {
        public ClusterManagementException(ClusterManagementErrorCode errorCode, params object[] errorMessageParams)
        : this(errorCode, errorMessageParams, null)
        {
        }

        public ClusterManagementException(ClusterManagementErrorCode errorCode, object[] errorMessageParams, Exception innerException)
        : base(GetMessage(CultureInfo.CurrentUICulture, errorCode, errorMessageParams), innerException)
        {
            this.Code = errorCode;
            this.ErrorMessageParams = errorMessageParams;
        }

        public ClusterManagementErrorCode Code { get; private set; }

        public object[] ErrorMessageParams { get; private set; }

        public string GetMessage(CultureInfo culture)
        {
            return GetMessage(culture, this.Code, this.ErrorMessageParams);
        }

        private static string GetMessage(CultureInfo culture, ClusterManagementErrorCode errorCode, params object[] errorMessageParams)
        {
            var resourceManager = new System.Resources.ResourceManager("System.Fabric.Strings.StringResources", typeof(System.Fabric.Strings.StringResources).Assembly);
            string errMsg = resourceManager.GetString(errorCode.ToString(), culture);
            if (errMsg == null)
            {
                throw new InvalidOperationException("StringResources.resx does not contain a string resource " + errorCode +
                                                    ". Requested culture: " + culture);
            }

            if (errorMessageParams != null && errorMessageParams.Length > 0)
            {
                errMsg = string.Format(CultureInfo.InvariantCulture, errMsg, errorMessageParams);
            }

            return errMsg;
        }
    }
}