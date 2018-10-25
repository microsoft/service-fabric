// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Globalization;
    using System.Security;
    using System.Collections.Generic;
    using System.Fabric.Dca.Validator;

    // This class implements validation for performance counter config reader 
    // settings in the cluster manifest
    public class PerfCounterConfigReaderValidator : ClusterManifestSettingsValidator, IDcaSettingsValidator
    {
        protected override string isEnabledParamName
        {
            get
            {
                return PerfCounterProducerConstants.EnabledParamName;
            }
        }

        public PerfCounterConfigReaderValidator()
        {
            this.settingHandlers = new Dictionary<string, Handler>();
            this.encryptedSettingHandlers = new Dictionary<string, Handler>();
            return;
        }
    }
}