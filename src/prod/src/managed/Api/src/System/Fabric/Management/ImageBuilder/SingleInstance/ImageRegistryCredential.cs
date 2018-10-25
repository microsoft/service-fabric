// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class ImageRegistryCredential
    {
        public ImageRegistryCredential()
        {
            this.server = null;
            this.username = null;
            this.password = null;
        }

        public string server;
        public string username;
        public string password;

        public bool IsServerNameSpecified()
        {
            return !String.IsNullOrEmpty(server);
        }

        public bool IsCredentialsSpecified()
        {
            return !String.IsNullOrEmpty(username);
        }
    };
}
