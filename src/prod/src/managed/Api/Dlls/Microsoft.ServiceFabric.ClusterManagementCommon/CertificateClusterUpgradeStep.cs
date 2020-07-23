// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    using Newtonsoft.Json;

    internal sealed class CertificateClusterUpgradeStep
    {
        public CertificateClusterUpgradeStep(
            List<string> thumbprintWhiteList,
            CertificateDescription thumbprintLoadList,
            CertificateDescription thumbprintFileStoreSvcList,
            Dictionary<string, string> commonNameWhiteList,
            ServerCertificateCommonNames commonNameLoadList,
            ServerCertificateCommonNames commonNameFileStoreSvcList)
        {
            this.ThumbprintWhiteList = thumbprintWhiteList;
            this.ThumbprintLoadList = thumbprintLoadList;
            this.ThumbprintFileStoreSvcList = thumbprintFileStoreSvcList;

            this.CommonNameWhiteList = commonNameWhiteList;
            this.CommonNameLoadList = commonNameLoadList;
            this.CommonNameFileStoreSvcList = commonNameFileStoreSvcList;
        }

        public List<string> ThumbprintWhiteList { get; private set; }

        public CertificateDescription ThumbprintLoadList { get; private set; }

        public CertificateDescription ThumbprintFileStoreSvcList { get; private set; }

        public Dictionary<string, string> CommonNameWhiteList { get; private set; }

        public ServerCertificateCommonNames CommonNameLoadList { get; private set; }

        public ServerCertificateCommonNames CommonNameFileStoreSvcList { get; private set; }
    }
}