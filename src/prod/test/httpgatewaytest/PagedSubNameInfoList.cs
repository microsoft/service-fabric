// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;
using System.Fabric;

namespace System.Fabric.Test.HttpGatewayTest
{
    // Binary and Double values need their own class because they
    // are not sent as strings.

    [Serializable]
    [DataContract]
    class PagedSubNameInfoList
    {

        public PagedSubNameInfoList()
        {
            ContinuationToken = "";
            IsConsistent = false;
            SubNames = new List<Uri>();
        }

        [DataMember]
        public string ContinuationToken
        {
            get;
            set;
        }

        [DataMember]
        public bool IsConsistent
        {
            get;
            set;
        }

        [DataMember]
        public List<Uri> SubNames
        {
            get;
            set;
        }
    }
}