//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin.Controllers
{
    using System.Fabric;
    using System.IO;
    using Microsoft.AspNetCore.Mvc;
    using Microsoft.ServiceFabric.Diagnostics.Tracing;
    using Newtonsoft.Json;
    using AzureFilesVolumePlugin.Models;


    [Route("/")]
    public class VolumePluginController : Controller
    {
        private const string TraceType = "AzureFilesVolController";
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
            var body = this.Deserialize<VolumeCreateRequest>(Request.Body);
            TraceWriter.WriteInfoWithId(
                TraceType,
                this.serviceContext.TraceId,
                $"VolumeCreate : {body.Name}, {body.Opts.Count}");
            return this.Serialize(this.volumeMap.CreateVolume(body));
        }

        [HttpPost("/VolumeDriver.Remove")]
        public string VolumeRemove()
        {
            var body = this.Deserialize<VolumeName>(Request.Body);
            TraceWriter.WriteInfoWithId(
                TraceType,
                this.serviceContext.TraceId,
                $"VolumeRemove : {body.Name}");
            return this.Serialize(this.volumeMap.RemoveVolume(body));
        }

        [HttpPost("/VolumeDriver.Mount")]
        public string VolumeMount()
        {
            var body = this.Deserialize<VolumeMountRequest>(Request.Body);
            TraceWriter.WriteInfoWithId(
                TraceType,
                this.serviceContext.TraceId,
                $"VolumeMount : {body.Name}, {body.ID}");
            return this.Serialize(this.volumeMap.MountVolume(body));
        }

        [HttpPost("/VolumeDriver.Path")]
        public string VolumePath()
        {
            var body = this.Deserialize<VolumeName>(Request.Body);
            TraceWriter.WriteInfoWithId(
                TraceType,
                this.serviceContext.TraceId,
                $"VolumePath : {body.Name}");
            return this.Serialize(this.volumeMap.GetVolumeMountPoint(body));
        }

        [HttpPost("/VolumeDriver.Unmount")]
        public string VolumeUnmount()
        {
            var body = this.Deserialize<VolumeUnmountRequest>(Request.Body);
            TraceWriter.WriteInfoWithId(
                TraceType,
                this.serviceContext.TraceId,
                $"VolumeUnmount : {body.Name}, {body.ID}");
            return this.Serialize(this.volumeMap.UnmountVolume(body));
        }

        [HttpPost("/VolumeDriver.Get")]
        public string VolumeGet()
        {
            var body = this.Deserialize<VolumeName>(Request.Body);
            TraceWriter.WriteInfoWithId(
                TraceType,
                this.serviceContext.TraceId,
                $"VolumeGet : {body.Name}");
            return this.Serialize(this.volumeMap.GetVolume(body));
        }

        [HttpPost("/VolumeDriver.List")]
        public string VolumeList()
        {
            var response = this.volumeMap.ListVolumes();
            var count = ((VolumeListResponse)response).Volumes.Count;
            TraceWriter.WriteInfoWithId(
                TraceType,
                this.serviceContext.TraceId,
                $"VolumeList: returning {count} volumes");
            return this.Serialize(response);
        }

        [HttpPost("/VolumeDriver.Capabilities")]
        public string VolumeCapabilities()
        {
            TraceWriter.WriteInfoWithId(
                TraceType,
                this.serviceContext.TraceId,
                "VolumeCapabilities");
            return volumeCapabilities;
        }

        [HttpPost("/Plugin.Activate")]
        public string Activate()
        {
            TraceWriter.WriteInfoWithId(
                TraceType,
                this.serviceContext.TraceId,
                "Activate");
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
    }
}

