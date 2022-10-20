// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;

    /// <summary>
    /// Structure of the json .output file.
    /// </summary>
    internal sealed class JsonExpectedTestOutput
    {
        public List<JsonExpectedActions> Items { get; set; }

        public override string ToString()
        {
            var text = "Items: {0}".ToString(Items != null ? Items.Count.ToString() : "<null>");
            return text;
        }
    }
}