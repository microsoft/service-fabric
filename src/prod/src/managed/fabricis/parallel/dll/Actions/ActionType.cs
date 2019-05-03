// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    [Flags]
    internal enum ActionType
    {
        None = 0x0,
        
        Prepare = 0x1,
        
        Execute = 0x2,
        
        Restore = 0x4,
        
        RequestRepair = 0x8,  
    }
}