// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.BackupRestoreExceptions;
using System.Fabric.Interop;
using Newtonsoft.Json;
using Newtonsoft.Json.Converters;
using Newtonsoft.Json.Linq;
using System.Runtime.Serialization;
using System.Web.Http;

namespace System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter
{
    [DataContract]
    internal class BackupStorageConverter : CustomCreationConverter<BackupStorage>
    {
        protected bool IsNullValueAllowed { get; set; }

        public BackupStorageConverter()
        {
            IsNullValueAllowed = false;
        }

        /// <summary>
        /// Create a Backup Storage JSON Converter
        /// </summary>
        /// <param name="objectType"> Null as Backup Storage Policy is abstract </param>
        /// <returns></returns>
        public override BackupStorage Create(Type objectType)
        {
            return null;
        }

        /// <summary>
        /// Reads JSON to create a particular Backup Storage Type
        /// </summary>
        /// <returns> Backup storage type Object from JSON</returns>
        public override object ReadJson(JsonReader reader,
                                   Type objectType,
                                    object existingValue,
                                    JsonSerializer serializer)
        {
            BackupStorage backupStorage;

            if (IsNullValueAllowed)
            {
                // Backup storage can be null
                if (reader.TokenType == JsonToken.Null) return null;
            }

            var backupStorageJObject = JObject.Load(reader);
            if (backupStorageJObject["StorageKind"] == null)
            {
                var fabricError = new FabricError
                {
                    Message = string.Format("StorageType is Required"),
                    Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                };

                throw new HttpResponseException(new FabricErrorError(fabricError).ToHttpResponseMessage());
            }

            switch(backupStorageJObject["StorageKind"].Value<string>())
            {
                case "FileShare":
                    backupStorage = new FileShareBackupStorageInfo();
                    break;
                case "AzureBlobStore":
                    backupStorage = new AzureBlobBackupStorageInfo();
                    break;
                case "DsmsAzureBlobStore":
                    backupStorage = new DsmsAzureBlobBackupStorageInfo();
                    break;
                default:
                    var fabricError = new FabricError
                    {
                        Message = string.Format(String.Format("Invalid StorageKind {0} ", backupStorageJObject["StorageKind"].Value<string>())),
                        Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                    };

                    throw new HttpResponseException(new FabricErrorError(fabricError).ToHttpResponseMessage());
            }

            serializer.Populate(backupStorageJObject.CreateReader(), backupStorage);
            return backupStorage;
        }
    }
}