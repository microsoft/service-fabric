// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class ApplicationUpgradeUpdateDescriptionInput
    {
        public ApplicationUpgradeUpdateDescriptionInput()
        {
            Name = "";
            UpgradeKind = null;
            UpdateDescription = null;
        }

        [DataMember]
        public string Name
        {
            get;
            set;
        }

        [DataMember]
        public System.Fabric.Description.UpgradeKind? UpgradeKind
        {
            get;
            set;
        }

        [DataMember]
        public RollingUpgradeUpdateDescriptionInput UpdateDescription
        {
            get;
            set;
        }
    }
}
