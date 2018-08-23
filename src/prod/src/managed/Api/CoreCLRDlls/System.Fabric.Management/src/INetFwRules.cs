// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections;

namespace NetFwTypeLib
{
    internal interface INetFwRules : IEnumerable
    {
        int Count { get; }

        void Add(INetFwRule rule);
        
        INetFwRule Item(string name);

        void Remove(string name);

        void Update(INetFwRule rule);
    }
}