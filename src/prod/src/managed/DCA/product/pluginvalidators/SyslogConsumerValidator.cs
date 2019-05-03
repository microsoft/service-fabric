// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca.Validator;
    using System.Security;

    // This class implements validation for the SyslogConsumer settings.
    public class SyslogConsumerValidator : ClusterManifestBlobSettingsValidator, IDcaAzureSettingsValidator
    {
        protected override string isEnabledParamName
        {
            get
            {
                return SyslogConstant.EnabledParamName;
            }
        }

        public SyslogConsumerValidator()
        {
            this.settingHandlers = new Dictionary<string, Handler>
            {
                {
                    SyslogConstant.FacilityParamName,
                    new Handler
                    {
                        Validator = SyslogFacilityHandler,
                        AbsenceHandler = SyslogFacilityAbsenceHandler
                    }
                },
                {
                    SyslogConstant.IdentityParamName,
                    new Handler
                    {
                        Validator = SyslogIdentityHandler,
                        AbsenceHandler = SyslogIdentityAbsenceHandler
                    }
                },
            };

            this.encryptedSettingHandlers = new Dictionary<string, Handler>();
        }

        public override bool Validate(
                                 ITraceWriter traceEventSource,
                                 string sectionName,
                                 Dictionary<string, string> parameters,
                                 Dictionary<string, SecureString> encryptedParameters)
        {
            return base.Validate(traceEventSource, sectionName, parameters, encryptedParameters);
        }

        private bool SyslogFacilityHandler()
        {
            var facility = this.parameters[SyslogConstant.FacilityParamName];

            SyslogFacility facilityEnum;
            var success = Enum.TryParse(facility, true, out facilityEnum);
            if(!success)
            {
                traceSource.WriteError(
                    this.logSourceId,
                    "The value for parameter {0} in section {1} is an Invalid Syslog facility",
                    SyslogConstant.FacilityParamName,
                    this.sectionName);
            }

            return success;
        }

        private bool SyslogFacilityAbsenceHandler()
        {
            this.traceSource.WriteInfo(
                    this.logSourceId,
                    "The value for parameter {0} in section {1} is not specified. Default value will be used",
                    SyslogConstant.FacilityParamName,
                    this.sectionName);

            return true;
        }

        private bool SyslogIdentityHandler()
        {
            var identity = this.parameters[SyslogConstant.IdentityParamName];

            if (string.IsNullOrWhiteSpace(identity))
            {
                traceSource.WriteError(
                    this.logSourceId,
                    "The value for parameter {0} in section {1} is an Null/empty",
                    SyslogConstant.IdentityParamName,
                    this.sectionName);

                return false;
            }

            return true;
        }

        private bool SyslogIdentityAbsenceHandler()
        {
            this.traceSource.WriteInfo(
                    this.logSourceId,
                    "The value for parameter {0} in section {1} is not specified. Default value will be used",
                    SyslogConstant.IdentityParamName,
                    this.sectionName);

            return true;
        }

    }
}