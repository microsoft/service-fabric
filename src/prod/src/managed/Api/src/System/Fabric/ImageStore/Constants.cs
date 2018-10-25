// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    /// <summary>
    /// Location for common constants;
    /// </summary>
    internal class ImageStoreConstants
    {        
        /// <summary>The FabricNode section name</summary>
        public const string FabricNodeSection = "FabricNode";

        /// <summary>The WorkingDir property name</summary>
        public const string WorkingDirKey = "WorkingDir";

        public const string NativeImageStoreConnectionString = "fabric:ImageStore";

        public const string AccountTypeKey = "AccountType=";
        public const string AccountNameKey = "AccountName=";
        public const string AccountPasswordKey = "AccountPassword=";

        public const string DomainUser_AccountType = "DomainUser";
        public const string ManagedServiceAccount_AccountType = "ManagedServiceAccount";
    }
}