// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NetFwTypeLib
{
    using System.Security.Cryptography.X509Certificates;

    internal interface INetFwRule
    {
        NET_FW_ACTION_ Action { get; set; }

        string ApplicationName { get; set; }

        NET_FW_RULE_DIRECTION_ Direction { get; set; }

        bool Enabled { get; set; }

        string Grouping { get; set; }

        string LocalPorts { get; set; }

        string Name { get; set; }

        int Profiles { get; set; }

        int Protocol { get; set; }
        
        INetFwRule Clone();
    }
}