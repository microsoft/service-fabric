// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Security;
    using System.Globalization;
    using System.IO;
    using System.Collections.Generic;
    using System.Fabric.Dca.Validator;

    // This class implements validation for Ltt producer settings in the cluster
    // manifest
    public class LttProducerValidator : ClusterManifestSettingsValidator, IDcaSettingsValidator
    {
        protected override string isEnabledParamName
        {
            get
            {
                return LttProducerConstants.EnabledParamName;
            }
        }

        public LttProducerValidator()
        {
            this.settingHandlers = new Dictionary<string, Handler>()
                                   {
                                       {
                                           LttProducerConstants.LttReadIntervalParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLttReadIntervalSetting
                                           }
                                       },
                                       {
                                           LttProducerConstants.ServiceFabricLttTypeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLttTypeSetting
                                           }
                                       },
                                   };
            this.encryptedSettingHandlers = new Dictionary<string, Handler>();
            return;
        }
        
        private bool ValidateLttReadIntervalSetting()
        {
            string lttReadIntervalAsString = this.parameters[LttProducerConstants.LttReadIntervalParamName];
            
            int lttReadInterval;
            if (false == this.ValidateNonNegativeIntegerValue(
                             LttProducerConstants.LttReadIntervalParamName,
                             lttReadIntervalAsString, 
                             out lttReadInterval))
            {
                return false;
            }

            if ((0 != lttReadInterval) && 
                (LttProducerConstants.minRecommendedLttReadInterval > lttReadInterval))
            {
                this.WriteWarningInVSOutputFormat(
                    this.logSourceId,
                    "The value for setting {0} in section {1} is currently specified as {2} minutes. The minimum recommended value {3} minutes.",
                    LttProducerConstants.LttReadIntervalParamName,
                    this.sectionName,
                    lttReadInterval,
                    LttProducerConstants.minRecommendedLttReadInterval);
            }

            return true;
        }

		private bool ValidateLttTypeSetting()
        {
            string lttTypeAsString;
            string lttParameterName;
            if(this.parameters.ContainsKey(LttProducerConstants.ServiceFabricLttTypeParamName)){
                lttTypeAsString = this.parameters[LttProducerConstants.ServiceFabricLttTypeParamName];
                lttParameterName = LttProducerConstants.ServiceFabricLttTypeParamName;
            }
            else
            {
                lttTypeAsString = this.parameters[LttProducerConstants.ServiceFabricLttTypeParamName];
                lttParameterName = LttProducerConstants.ServiceFabricLttTypeParamName;
            }
            
            if (lttTypeAsString.Equals(
                    LttProducerConstants.QueryLtt,
                    StringComparison.Ordinal))
            {
                return true;
            }
            else
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The Ltt type '{0}' specified in section {1}, parameter {2} is not supported.",
                    lttTypeAsString,
                    this.sectionName,
                    lttParameterName);
                return false;
            }
        }
        
        public override bool Validate(
                                 ITraceWriter traceEventSource,
                                 string sectionName,
                                 Dictionary<string, string> parameters, 
                                 Dictionary<string, SecureString> encryptedParameters)
        {
            bool success = base.Validate(
                               traceEventSource, 
                               sectionName, 
                               parameters, 
                               encryptedParameters);

            return success;
        }
    }
}