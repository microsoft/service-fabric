// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    internal class BRSContinuationToken
    {
        public string IncomingContinuationToken { set; get; } = null;

        public string OutgoingContinuationToken { set; get; } = null;
    }
}
