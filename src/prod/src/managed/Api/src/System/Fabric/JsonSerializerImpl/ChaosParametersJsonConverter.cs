// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Linq;

    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    public sealed class ChaosParametersJsonConverter : JsonConverter
    {
        private const string TraceSource = "ChaosParametersJsonConverter";

        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(ChaosParameters);
        }

        /// <inheritdoc />
        public override object ReadJson(
            JsonReader reader,
            Type objectType,
            object existingValue,
            JsonSerializer serializer)
        {
            ThrowIf.Null(reader, "reader");

            if (reader.TokenType == JsonToken.Null)
            {
                return null;
            }

            TimeSpan maxClusterStabilizationTimeout = ChaosConstants.DefaultClusterStabilizationTimeout;
            long maxConcurrentFaults = ChaosConstants.MaxConcurrentFaultsDefault;
            TimeSpan waitTimeBetweenIterations = ChaosConstants.WaitTimeBetweenIterationsDefault;
            TimeSpan waitTimeBetweenFaults = ChaosConstants.WaitTimeBetweenFaultsDefault;
            TimeSpan timeToRun = TimeSpan.FromSeconds(uint.MaxValue);
            bool enableMoveReplicaFaults = false;
            ClusterHealthPolicy healthPolicy = new ClusterHealthPolicy();
            Dictionary<string, string> context = null;
            ChaosTargetFilter chaosTargetFilter = null;

            var chaosParametersJObject = JObject.Load(reader);

            this.ReadTimePeriod(chaosParametersJObject, JsonSerializerImplConstants.MaxClusterStabilizationTimeoutInSeconds, ref maxClusterStabilizationTimeout);

            JToken maxConcurrentFaultsJToken = chaosParametersJObject[JsonSerializerImplConstants.MaxConcurrentFaults];
            if (maxConcurrentFaultsJToken != null)
            {
                maxConcurrentFaults = maxConcurrentFaultsJToken.Value<long>();
            }

            this.ReadTimePeriod(
                chaosParametersJObject,
                JsonSerializerImplConstants.WaitTimeBetweenIterationsInSeconds,
                ref waitTimeBetweenIterations);

            this.ReadTimePeriod(
                chaosParametersJObject,
                JsonSerializerImplConstants.WaitTimeBetweenFaultsInSeconds,
                ref waitTimeBetweenFaults);

            this.ReadTimePeriod(
                chaosParametersJObject,
                JsonSerializerImplConstants.TimeToRunInSeconds,
                ref timeToRun);

            JToken enableMoveJToken = chaosParametersJObject[JsonSerializerImplConstants.EnableMoveReplicaFaults];
            if (enableMoveJToken != null)
            {
                enableMoveReplicaFaults = enableMoveJToken.Value<bool>();
            }

            JToken policyJToken = chaosParametersJObject[JsonSerializerImplConstants.ClusterHealthPolicy];
            if (policyJToken != null)
            {
                healthPolicy = policyJToken.ToObject<ClusterHealthPolicy>(serializer);
            }

            JToken contextJToken = chaosParametersJObject[JsonSerializerImplConstants.Context];
            if (contextJToken != null)
            {
                var contextMap = contextJToken[JsonSerializerImplConstants.Map];
                if (contextMap != null)
                {
                    context = contextMap.ToObject<Dictionary<string, string>>(serializer);
                }
            }

            JToken entityFilterJToken = chaosParametersJObject[JsonSerializerImplConstants.ChaosTargetFilter];
            if (entityFilterJToken != null)
            {
                chaosTargetFilter = entityFilterJToken.ToObject<ChaosTargetFilter>(new JsonSerializer { NullValueHandling = NullValueHandling.Ignore });
            }

            return new ChaosParameters(
                                     maxClusterStabilizationTimeout,
                                     maxConcurrentFaults,
                                     enableMoveReplicaFaults,
                                     timeToRun,
                                     context,
                                     waitTimeBetweenIterations,
                                     waitTimeBetweenFaults,
                                     healthPolicy)
                                     {
                                         ChaosTargetFilter = chaosTargetFilter
                                     };
        }

        public override void WriteJson(
            JsonWriter writer,
            object value,
            JsonSerializer serializer)
        {
            ThrowIf.Null(writer, "writer");

            var chaosParameters = (ChaosParameters)value;

            JObject chaosParametersJObject = JObject.FromObject(chaosParameters, new JsonSerializer { NullValueHandling = NullValueHandling.Ignore });

            this.UpdateTimePeriod(
                chaosParametersJObject,
                JsonSerializerImplConstants.MaxClusterStabilizationTimeout,
                JsonSerializerImplConstants.MaxClusterStabilizationTimeoutInSeconds);

            this.UpdateTimePeriod(
                chaosParametersJObject,
                JsonSerializerImplConstants.WaitTimeBetweenIterations,
                JsonSerializerImplConstants.WaitTimeBetweenIterationsInSeconds);

            this.UpdateTimePeriod(
                chaosParametersJObject,
                JsonSerializerImplConstants.WaitTimeBetweenFaults,
                JsonSerializerImplConstants.WaitTimeBetweenFaultsInSeconds);

            this.UpdateTimePeriodAsString(
                chaosParametersJObject,
                JsonSerializerImplConstants.TimeToRun,
                JsonSerializerImplConstants.TimeToRunInSeconds);

            this.UpdateContext(chaosParametersJObject);

            this.UpdateClusterHealthPolicy(chaosParametersJObject);

            chaosParametersJObject.WriteTo(writer);
        }

        private void ReadTimePeriod(JObject chaosParametersJObject, string propertyName, ref TimeSpan timePeriod)
        {
            JToken timePeriodInSecondsJToken = chaosParametersJObject[propertyName];
            if (timePeriodInSecondsJToken != null)
            {
                uint timePeriodAsInt;

                if (!uint.TryParse(timePeriodInSecondsJToken.Value<string>(), out timePeriodAsInt))
                {
                    TimeSpan timePeriodAsTimeSpan;
                    if (TimeSpan.TryParse(timePeriodInSecondsJToken.Value<string>(), out timePeriodAsTimeSpan))
                    {
                        timePeriod = timePeriodAsTimeSpan;
                    }
                    else
                    {
                        throw new ArgumentException("'{0}' is in a bad format.", propertyName);
                    }
                }
                else
                {
                    timePeriod = TimeSpan.FromSeconds(timePeriodAsInt);
                }
            }
        }

        private void UpdateTimePeriod(JObject chaosParametersJObject, string propertyNameOld, string propertyNameNew)
        {
            JProperty timePeriodJProperty = chaosParametersJObject.Children<JProperty>()
                   .Where(p => p.Name == propertyNameOld)
                   .First();
            if (timePeriodJProperty != null && timePeriodJProperty.Value != null)
            {
                TimeSpan ts = TimeSpan.Parse(timePeriodJProperty.Value.ToString());
                JValue timePeriodInSecondsJValue = new JValue((uint)ts.TotalSeconds);

                chaosParametersJObject[propertyNameNew] = timePeriodInSecondsJValue;
                timePeriodJProperty.Remove();
            }
        }

        private void UpdateTimePeriodAsString(JObject chaosParametersJObject, string propertyNameOld, string propertyNameNew)
        {
            JProperty timePeriodJProperty = chaosParametersJObject.Children<JProperty>()
                   .Where(p => p.Name == propertyNameOld)
                   .First();
            if (timePeriodJProperty != null && timePeriodJProperty.Value != null)
            {
                TimeSpan ts = TimeSpan.Parse(timePeriodJProperty.Value.ToString());
                JValue timePeriodInSecondsJValue = new JValue((uint)ts.TotalSeconds);

                chaosParametersJObject[propertyNameNew] = timePeriodInSecondsJValue.ToString();
                timePeriodJProperty.Remove();
            }
        }

        private void UpdateContext(JObject chaosParametersJObject)
        {
            JProperty contextJProperty = chaosParametersJObject.Children<JProperty>()
                   .Where(p => p.Name == JsonSerializerImplConstants.Context)
                   .First();
            if (contextJProperty != null)
            {
                JProperty map = new JProperty(JsonSerializerImplConstants.Map, contextJProperty.Value);
                JObject mapObj = new JObject();
                mapObj.Add(map);
                JProperty context = new JProperty(JsonSerializerImplConstants.Context, mapObj);
                contextJProperty.Remove();
                chaosParametersJObject.Add(context);
            }
        }

        private void UpdateClusterHealthPolicy(JObject chaosParametersJObject)
        {
            JProperty policyJProperty = chaosParametersJObject.Children<JProperty>()
                               .Where(p => p.Name == JsonSerializerImplConstants.ClusterHealthPolicy)
                               .First();
            if (policyJProperty != null && policyJProperty.Value != null)
            {
                JProperty considerWarningAsErrorJProperty;
                JProperty maxPercentUnhealthyNodesJProperty;
                JProperty maxPercentUnhealthyApplicationsJProperty;
                JProperty applicationTypeHealthPolicyMapJProperty;
                JObject clusterPolicy = new JObject();

                // Update MaxPercentUnhealthyNodes
                JToken maxNodesJToken = policyJProperty.Value.SelectToken(JsonSerializerImplConstants.MaxPercentUnhealthyNodes);
                if (maxNodesJToken != null)
                {
                    maxPercentUnhealthyNodesJProperty = new JProperty(JsonSerializerImplConstants.MaxPercentUnhealthyNodes, maxNodesJToken.Value<int>());
                    clusterPolicy.Add(maxPercentUnhealthyNodesJProperty);
                }

                // Update ConsiderWarningAsError
                JToken considerJToken = policyJProperty.Value.SelectToken(JsonSerializerImplConstants.ConsiderWarningAsError);
                if (considerJToken != null)
                {
                    considerWarningAsErrorJProperty = new JProperty(JsonSerializerImplConstants.ConsiderWarningAsError, considerJToken.Value<bool>());
                    clusterPolicy.Add(considerWarningAsErrorJProperty);
                }

                // Update MaxPercentUnhealthyApplications
                JToken maxAppJToken = policyJProperty.Value.SelectToken(JsonSerializerImplConstants.MaxPercentUnhealthyApplications);
                if (maxAppJToken != null)
                {
                    maxPercentUnhealthyApplicationsJProperty = new JProperty(JsonSerializerImplConstants.MaxPercentUnhealthyApplications, maxAppJToken.Value<int>());
                    clusterPolicy.Add(maxPercentUnhealthyApplicationsJProperty);
                }

                // Update ApplicationTypeHealthPolicyMap
                JObject typeMapJObject = (JObject)policyJProperty.Value.SelectToken(JsonSerializerImplConstants.ApplicationTypeHealthPolicyMap);
                if (typeMapJObject != null)
                {
                    JArray applicationTypePolicyJArray = new JArray();

                    foreach (JProperty property in typeMapJObject.Properties())
                    {
                        JObject policyEntryJObject = new JObject();
                        policyEntryJObject[JsonSerializerImplConstants.Key] = property.Name;
                        policyEntryJObject[JsonSerializerImplConstants.Value] = property.Value;
                        applicationTypePolicyJArray.Add(policyEntryJObject);
                    }

                    if (applicationTypePolicyJArray.Any())
                    {
                        applicationTypeHealthPolicyMapJProperty = new JProperty(JsonSerializerImplConstants.ApplicationTypeHealthPolicyMap, applicationTypePolicyJArray);
                        clusterPolicy.Add(applicationTypeHealthPolicyMapJProperty);
                    }
                }

                policyJProperty.Remove();
                chaosParametersJObject[JsonSerializerImplConstants.ClusterHealthPolicy] = clusterPolicy;
            }
        }
    }
}