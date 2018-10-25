// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    internal class ContainerRepositoryCredentials
    {
        public string RepositoryUserName;
        public string RepositoryPassword;
        public bool IsPasswordEncrypted;

        public ContainerRepositoryCredentials()
        {
            this.RepositoryUserName = "";
            this.RepositoryPassword = "";
            this.IsPasswordEncrypted = false;
        }

        public bool IsCredentialsSpecified()
        {
            return !string.IsNullOrEmpty(this.RepositoryUserName) && !string.IsNullOrEmpty(this.RepositoryPassword);
        }
    }
}