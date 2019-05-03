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
    class RollingUpgradeUpdateDescriptionInput
    {
        public RollingUpgradeUpdateDescriptionInput()
        {
            RollingUpgradeMode = null;
        }

        [DataMember]
        public System.Fabric.RollingUpgradeMode? RollingUpgradeMode
        {
            get;
            set;
        }

        //
        // Do not include members not explicitly being used.
        // The JSON contract serializer converts null properties
        // into explicit {"prop":null} rather than just omitting them
        // and our HTTP gateway is currently not accepting such formats.
        //
    }
}