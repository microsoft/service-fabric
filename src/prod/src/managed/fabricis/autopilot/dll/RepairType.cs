// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    // Refer to Autopilot RepairRequestType::c_requestTypeNames
    internal enum RepairType
    {
        Unknown,

        Recover,
        NoOp,
        SwitchService,
        ServiceRestart,
        SoftReboot,
        HardReboot,
        OSUpgrade,
        SoftImage,
        HardImage,
        SoftImageBurnin,
        HardImageBurnin,
        Triage,
        WipeAndRMA,
        TestNoOp1,
        TestNoOp2,
        ConfigurationProvision,
        DIPDeploy,
        BIOSUpgrade,
        SoftRebootRollout,
        DataRollout,
        FirmwareUpgrade,
        UpstreamRepair,
        RMA,
        SoftWipe,
        HardWipe,
        AcceptanceTest,
        None,
        Reassign,
        DestructiveUpdateMachine,
        ImpDestructive,
        ImpNonDestructive,
        AppRollout,
        AzureConfigurationUpdate,
        AzureApplicationUpdate,
        AzureMOS,
        AzureRMA,
        NonDestructiveUpdateMachine,
    }
}