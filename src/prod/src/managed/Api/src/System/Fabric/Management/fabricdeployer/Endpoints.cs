// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer
{
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;

    internal class Endpoints
    {
        public const string Metadata = "Role, ClientConnection, LeaseDriver, ClusterConnection, DefaultReplicator, NamingReplicator, FMReplicator, CMReplicator, ApplicationStart, ApplicationEnd";

        public string Role { get; set; }

        public string ClientConnection { get; set; }

        public string LeaseDriver { get; set; }

        public string ClusterConnection { get; set; }

        public string DefaultReplicator { get; set; }

        public string NamingReplicator { get; set; }

        public string FMReplicator { get; set; }

        public string CMReplicator { get; set; }

        public string ApplicationStart { get; set; }

        public string ApplicationEnd { get; set; }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static Endpoints Parse(string inputValue)
        {
            string[] input = inputValue.Split(',');
            Endpoints endpoint = new Endpoints
                {
                Role = input[0],
                ClientConnection = input[1],
                LeaseDriver = input[2],
                ClusterConnection = input[3],
                DefaultReplicator = input[4],
                NamingReplicator = input[5],
                FMReplicator = input[6],
                CMReplicator = input[7],
                ApplicationStart = input[8],
                ApplicationEnd = input[9]
            };
            return endpoint;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "{0},{1},{2},{3},{4},{5},{6},{7},{8},{9}",
                this.Role,
                this.ClientConnection,
                this.LeaseDriver,
                this.ClusterConnection,
                this.DefaultReplicator,
                this.NamingReplicator,
                this.FMReplicator,
                this.CMReplicator,
                this.ApplicationStart,
                this.ApplicationEnd);
        }
    }
}