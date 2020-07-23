// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;

    internal class EndpointsList
    {
        private const string FileName = "EndpointsList.csv";

        protected EndpointsList(List<Endpoints> endpointsList)
        {
            this.List = endpointsList;
        }

        private EndpointsList()
        {
            this.List = new List<Endpoints>();
        }

        public List<Endpoints> List { get; private set; }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static EndpointsList ReadEndpointsListFromFile(string fileLocation)
        {
            EndpointsList list = new EndpointsList();
            string[] strEndponitsList = File.ReadAllLines(Path.Combine(fileLocation, FileName), Text.Encoding.ASCII);
            for (int i = 1; i < strEndponitsList.Length; i++)
            {
                list.List.Add(Endpoints.Parse(strEndponitsList[i]));
            }

            return list;
        }

        public void GenerateEndpointsListFile(string fileLocation)
        {
            List<string> endpointsList = new List<string>();
            endpointsList.Add(Endpoints.Metadata);
            foreach (var node in this.List)
            {
                endpointsList.Add(node.ToString());
            }

            File.WriteAllLines(Path.Combine(fileLocation, FileName), endpointsList.ToArray(), Text.Encoding.ASCII);
        }
    }
}