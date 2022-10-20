// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;

    internal sealed class JsonExpectedActions
    {
        public string Id { get; set; }
        public List<string> Actions { get; set; }

        public override string ToString()
        {
            var actions = Actions != null ? string.Join(",", Actions) : null;
            var text = "Id: {0}, Actions: {1}".ToString(Id, actions);
            return text;
        }
    }
}