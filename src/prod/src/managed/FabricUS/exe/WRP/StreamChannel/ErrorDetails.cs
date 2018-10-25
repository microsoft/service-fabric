// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    public class ErrorDetails
    {
        public Error Error { get; set; }

        public override string ToString()
        {
            if (this.Error != null)
            {
                return string.Format("Error Code:{0}, Message:{1}", Error.Code, Error.Message);
            }
            else
            {
                return string.Format("Error {0}", "null");
            }
        }
    }
}