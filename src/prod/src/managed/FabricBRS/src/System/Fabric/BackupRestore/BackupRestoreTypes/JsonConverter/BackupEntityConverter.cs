// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter
{
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using Newtonsoft.Json.Linq;
    using System.Fabric.Interop;
    using System.Web.Http;

    [DataContract]
    internal class BackupEntityConverter : CustomCreationConverter<BackupEntity>
    {
        /// <summary>
        /// Create a BackupEntity JSON Converter
        /// </summary>
        /// <param name="objectType"> Null as BackupEntity is abstract  </param>
        /// <returns></returns>
        public override BackupEntity Create(Type objectType)
        {
            return null;
        }

        /// <summary>
        /// Reads JSON to create a particular Retention policy Type
        /// </summary>
        /// <returns>Retention Type Object created from JSON</returns>
        public override object ReadJson(JsonReader reader,
                                   Type objectType,
                                    object existingValue,
                                    JsonSerializer serializer)
        {
            BackupEntity backupEntity = null;
            JObject retentionPolicyJObject = JObject.Load(reader);
            if (retentionPolicyJObject["EntityKind"] == null)
            {
                var fabricError = new FabricError
                {
                    Message = string.Format("EntityKind is Required"),
                    Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                };

                throw new HttpResponseException(new FabricErrorError(fabricError).ToHttpResponseMessage());
            }
            if ("Application".Equals(retentionPolicyJObject["EntityKind"].Value<string>()))
            {
                backupEntity = new ApplicationBackupEntity();
            }
            else if ("Service".Equals(retentionPolicyJObject["EntityKind"].Value<string>()))
            {
                backupEntity = new ServiceBackupEntity();
            }
            else if ("Partition".Equals(retentionPolicyJObject["EntityKind"].Value<string>()))
            {
                backupEntity = new PartitionBackupEntity();
            }
            else
            {
                var fabricError = new FabricError
                {
                    Message = string.Format("Invalid Backup Entitiy Type {0} ", retentionPolicyJObject["EntityKind"].Value<string>()),
                    Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                };

                throw new HttpResponseException(new FabricErrorError(fabricError).ToHttpResponseMessage());
            }
            serializer.Populate(retentionPolicyJObject.CreateReader(), backupEntity);
            return backupEntity;
        }
    }
}