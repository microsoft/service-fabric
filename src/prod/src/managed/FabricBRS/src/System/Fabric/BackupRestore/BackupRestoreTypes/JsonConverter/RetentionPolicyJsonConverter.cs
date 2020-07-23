// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Interop;
using System.Web.Http;

namespace System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter
{
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using Newtonsoft.Json.Linq;

    [DataContract]
    internal class RetentionPolicyJsonConverter : CustomCreationConverter<RetentionPolicy>
    {
        /// <summary>
        /// Create a Retention Policy JSON Converter
        /// </summary>
        /// <param name="objectType"> Null as Retention Policy is abstract  </param>
        /// <returns></returns>
        public override RetentionPolicy Create(Type objectType)
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
            RetentionPolicy retentionPolicy = null;
            JObject retentionPolicyJObject = JObject.Load(reader);

            // RetentionPolicyType is optional as only Basic Type is there. 
            if ("Basic".Equals(retentionPolicyJObject["RetentionPolicyType"].Value<string>()))
            {
               
                retentionPolicy = new BasicRetentionPolicy(); 
            }
            else
            {
                var fabricError = new FabricError
                {
                    Message = string.Format("Invalid Retention policy Type {0} ", retentionPolicyJObject["RetentionPolicyType"].Value<string>()),
                    Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                };

                throw new HttpResponseException(new FabricErrorError(fabricError).ToHttpResponseMessage());
            }
            serializer.Populate(retentionPolicyJObject.CreateReader(), retentionPolicy);
            return retentionPolicy;
        }
    }
}