// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    // This interface needs to be implemented by DCA consumers
    // that are capable of handling upgrade from DCA v1
    internal interface IDcaUpgradableConsumer : IDcaConsumer
    {
        // Whether or not the current consumer instance should
        // handle upgrade from DCA v1
        bool HandleUpgradeFromV1 { set; }
    }
}