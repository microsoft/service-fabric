// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.BackupRestoreExceptions;
using System.Fabric.Interop;
using System.Web.Http;

namespace System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using Newtonsoft.Json.Linq;

    internal class BackupScheduleJsonConverter : CustomCreationConverter<BackupSchedule>
    {
        /// <summary>
        /// Create a Schedule Policy JSON Converter
        /// </summary>
        /// <param name="objectType"> Null as Schedule Policy is abstract </param>
        /// <returns></returns>
        public override BackupSchedule Create(Type objectType)
        {
            return null;
        }

        /// <summary>
        /// Reads JSON to create a particular Schedule policy Type
        /// </summary>
        /// <returns> Schedule Policy Type Object from JSON</returns>
        public override object ReadJson(JsonReader reader,
                                   Type objectType,
                                    object existingValue,
                                    JsonSerializer serializer)
        {
            BackupSchedule backupSchedule = null;
            JObject schedulePolicyJObject = JObject.Load(reader);
            if (schedulePolicyJObject["ScheduleKind"] == null)
            {
                var fabricError = new FabricError
                {
                    Message = " ScheduleKind is Required",
                    Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG
                };

                throw new HttpResponseException(new FabricErrorError(fabricError).ToHttpResponseMessage());

            }
            if ("TimeBased".Equals(schedulePolicyJObject["ScheduleKind"].Value<string>()))
            {
                backupSchedule = new TimeBasedBackupSchedule();
            }
            else if ("FrequencyBased".Equals(
                    schedulePolicyJObject["ScheduleKind"].Value<string>()))
            {
                backupSchedule = new FrequencyBasedBackupSchedule();
            }
            else
            {
                var fabricError = new FabricError
                {
                    Message = string.Format("Invalid Schedule Type {0}", schedulePolicyJObject["ScheduleKind"].Value<string>()),
                    Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG
                };

                throw new HttpResponseException(new FabricErrorError(fabricError).ToHttpResponseMessage());
            }
            serializer.Populate(schedulePolicyJObject.CreateReader(), backupSchedule);
            return backupSchedule;
        }
    }
}