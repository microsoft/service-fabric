// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace BSDockerVolumePlugin.Controllers
{
    using System;
    using System.Text;
    using System.IO;
    using Microsoft.AspNetCore.Mvc;
    using Newtonsoft.Json;
    using BSDockerVolumePlugin.Models;
    using System.Fabric;
    using Microsoft.ServiceFabric.Diagnostics.Tracing;

    [Route("/")]
    public class VolumePluginController : Controller
    {
        private const string activationString = "{ \"Implements\" : [\"VolumeDriver\"] }";
        private const string volumeCapabilities = "{\"Capabilities\": {\"Scope\": \"global\"}}";
        private VolumeMap volumeMap;
        private StatelessServiceContext serviceContext;

        public VolumePluginController(VolumeMap volumeMappings, StatelessServiceContext context)
        {
            this.volumeMap = volumeMappings;
            this.serviceContext = context;
        }

        [HttpPost("/VolumeDriver.Create")]
        public string VolumeCreate()
        {
            string traceId = GetTraceId("VolumeCreate");
            
            var body = this.Deserialize<VolumeCreateRequest>(Request.Body);
            TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"VolumeCreate : {body.Name}");
            
            foreach (var opt in body.Opts)
            {
                TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"VolumeCreate : {body.Name} Option {opt.Key}:{opt.Value}");
            }

            return this.Serialize(this.volumeMap.CreateVolume(body, traceId));
        }

        [HttpPost("/VolumeDriver.Remove")]
        public string VolumeRemove()
        {
            string traceId = GetTraceId("VolumeRemove");
            var body = this.Deserialize<VolumeName>(Request.Body);
            
            TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"VolumeRemove : {body.Name}");

            return this.Serialize(this.volumeMap.RemoveVolume(body, traceId));
        }

        [HttpPost("/VolumeDriver.Mount")]
        public string VolumeMount()
        {
            string traceId = GetTraceId("VolumeMount");
            var body = this.Deserialize<VolumeMountRequest>(Request.Body);

            TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"VolumeMount : {body.Name}, {body.ID}");
            
            return this.Serialize(this.volumeMap.MountVolume(body, traceId));
        }

        // Get the (ServicePartitionID, Port) and store in the map
        [HttpPost("/VolumeDriver.BlockStoreRegister")]
        public string BlockStoreRegister()
        {
            bool registerSuccessful = false;
            string traceId = GetTraceId("BlockStoreRegister");
            string blockStoreServicePartitionId = "";
            string blockStoreServicePort = "";

            try
            {
                // Catch the low memory exception
                byte[] buffer = new byte[(int)Request.ContentLength];
                Request.Body.Read(buffer, 0, (int)Request.ContentLength);

                // Format: "<ServicePartitionId> <Port>"
                string registerRequest = System.Text.Encoding.Default.GetString(buffer);
                string[] arrData = registerRequest.Split(new string[] { " " }, StringSplitOptions.None);
                if (arrData.Length == 2)
                {
                    blockStoreServicePartitionId = arrData[0];
                    blockStoreServicePort = arrData[1];

                    int port = 0;
                    if (Int32.TryParse(blockStoreServicePort, out port))
                    {
                        // ServicePartitionID could exist, (for instance, SF restart), then just update the map
                        // Need to make sure different Block Store Service never has same ServicePartitionID
                        registerSuccessful = this.volumeMap.UpdateServiceMappings(blockStoreServicePartitionId, port);
                    }
                    else
                    {
                        TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"Unable to parse registration request: {registerRequest}.");
                        registerSuccessful = false;
                    }
                }
                else
                {
                    TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"Malformed registration request: {registerRequest}.");
                    registerSuccessful = false;
                }
            }
            catch (Exception ex)
            {
                TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"Unexpected exception : {ex.Message}");
                registerSuccessful = false;
            }

            if (registerSuccessful)
            {
                TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"{blockStoreServicePartitionId}:{blockStoreServicePort} registered."); 
                return "RegisterSuccessful";
            }
            else
            {
                TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"Service registration failed.");
                return "RegisterFailed";
            }
        }
        
        [HttpPost("/VolumeDriver.Path")]
        public string VolumePath()
        {
            string traceId = GetTraceId("VolumePath");
            var body = this.Deserialize<VolumeName>(Request.Body);
            string response = this.Serialize(this.volumeMap.GetVolumeMountPoint(body, traceId));
            TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"VolumePath : {body.Name}, {response}");
            return response;
        }

        [HttpPost("/VolumeDriver.Unmount")]
        public string VolumeUnmount()
        {
            string traceId = GetTraceId("VolumeUnmount");
            var body = this.Deserialize<VolumeUnmountRequest>(Request.Body);
            string response = this.Serialize(this.volumeMap.UnmountVolume(body, traceId));
            TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"VolumeUnmount : {body.Name}, {body.ID}, {response}");
            return response;
        }

        [HttpPost("/VolumeDriver.Get")]
        public string VolumeGet()
        {
            string traceId = GetTraceId("VolumeGet");
            var body = this.Deserialize<VolumeName>(Request.Body);
            string response = this.Serialize(this.volumeMap.GetVolume(body, traceId));
            TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"VolumeGet : {body.Name}, {response}");
            return response;
        }

        [HttpPost("/VolumeDriver.List")]
        public string VolumeList()
        {
            string traceId = GetTraceId("VolumeList");
            var response = this.Serialize(this.volumeMap.ListVolumes());
            TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, $"VolumeList: {response}");
            return response;
        }

        [HttpPost("/VolumeDriver.Capabilities")]
        public string VolumeCapabilities()
        {
            string traceId = GetTraceId("VolumeCapabilities");
            TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, "VolumeCapabilities");
            return volumeCapabilities;
        }

        [HttpPost("/Plugin.Activate")]
        public string Activate()
        {
            string traceId = GetTraceId("Activate");
            TraceWriter.WriteInfoWithId(Constants.TraceSource, traceId, "Activate");
            return activationString;
        }

        private T Deserialize<T>(Stream request)
        {
            var serializer = new JsonSerializer();
            using (var reader = new StreamReader(request))
            using (var jsonTextReader = new JsonTextReader(reader))
            {
                return serializer.Deserialize<T>(jsonTextReader);
            }
        }

        private string Serialize<T>(T response)
        {
            return JsonConvert.SerializeObject(response, Formatting.None);
        }

        private string GetTraceId(string functionName)
        {
            if (TraceWriter.writeToConsole)
            {
                // Return an ID generated by us if we are writing traces to console
                return "[" + Guid.NewGuid().ToString() + "][" + functionName + "]";
            }
            else
            {
                return this.serviceContext.TraceId;
            }
        }
    }
}

