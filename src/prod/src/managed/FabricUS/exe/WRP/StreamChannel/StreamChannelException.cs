// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    public class StreamChannelException : Exception
    {
        public StreamChannelException(ErrorDetails errorDetails)
        {
            this.ErrorDetails = errorDetails;
        }

        public ErrorDetails ErrorDetails { get; set; }
    }
}