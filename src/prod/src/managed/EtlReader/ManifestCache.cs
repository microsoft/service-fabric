// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Threading;
    using System.Xml;

    public class ManifestCache
    {
        public const string ProviderAlreadyLoadedMessage = "An ETW provider with the same GUID = {0} is already loaded.";
        private const ushort EventHeaderFlagStringOnly = 0x4;        
        private const int DefaultLoadManifestRetryCount = 3;
        private const string DynamicManifestFolderName = "WFDynMan";
        private const string ManifestFilePattern = "*.man";
        private static readonly TimeSpan DefaultLoadManifestRetryInterval = TimeSpan.FromSeconds(5);

        private readonly Dictionary<Guid, ProviderDefinition> providers;
        private readonly Dictionary<string, List<Guid>> guids;
        private readonly PartialManifestInfo partialManifest;
        private readonly string dynamicManifestFolder;

        /// <summary>
        /// Initializes an instance of Manifest Cache
        /// </summary>
        /// <param name="workFolder">Folder where read/write permission is available for manifest related operations</param>
        public ManifestCache(string workFolder) : this(string.Empty, workFolder, new List<ProviderDefinitionDescription>())
        {
        }

        public ManifestCache(string filename, string workFolder, ICollection<ProviderDefinitionDescription> providers)
        {
            this.providers = new Dictionary<Guid, ProviderDefinition>();
            this.guids = new Dictionary<string, List<Guid>>();

            this.dynamicManifestFolder = Path.Combine(workFolder, DynamicManifestFolderName);
            Directory.CreateDirectory(this.dynamicManifestFolder);
            this.LoadDynamicManifests();

            this.AddProviders(filename, providers);
            this.partialManifest = new PartialManifestInfo();
        }

        internal ManifestCache(Dictionary<Guid, ProviderDefinition> providers, Dictionary<string, List<Guid>> guids)
        {
            this.providers = providers;
            this.guids = guids;
            this.partialManifest = new PartialManifestInfo();
        }

        public ManifestCache(IDictionary<Guid, ProviderDefinition> providers, IDictionary<string, List<Guid>> guids)
        {
            this.providers = new Dictionary<Guid, ProviderDefinition>();
            this.guids = new Dictionary<string, List<Guid>>();          

            foreach (KeyValuePair<Guid, ProviderDefinition> provider in providers)
            {
                this.providers.Add(provider.Key, new ProviderDefinition(provider.Value.Description));
            }

            foreach (KeyValuePair<string, List<Guid>> guid in guids)
            {
                this.guids.Add(guid.Key, new List<Guid>(guid.Value));
            }

            this.partialManifest = new PartialManifestInfo();
        }

        public IDictionary<Guid, ProviderDefinition> ProviderDefinitions
        {
            get { return this.providers; }
        }

        public IDictionary<string, List<Guid>> Guids
        {
            get { return this.guids; }
        }

        public void LoadManifest(string fileName)
        {
            var loader = new ManifestLoader();

            var providers = loader.LoadManifest(fileName).Providers;

            this.AddProviders(fileName, providers);
        }

        private void LoadDynamicManifests()
        {
            DirectoryInfo dirInfo = new DirectoryInfo(this.dynamicManifestFolder);
            FileInfo[] manifestFiles = dirInfo.GetFiles(ManifestFilePattern);

            // Load the manifests
            foreach (FileInfo manifestFile in manifestFiles)
            {
                var retries = DefaultLoadManifestRetryCount;
                while (retries >= 0)
                {
                    retries--;
                    try
                    {
                        this.LoadManifest(manifestFile.FullName);
                        break;
                    }
                    catch (XmlException)
                    {
                    }
                    catch (UnauthorizedAccessException)
                    {
                    }

                    Thread.Sleep(DefaultLoadManifestRetryInterval);
                }
            }
        }

        /// <summary>
        /// Loads manifest from stream and then writes the manifest contents onto a file
        /// This utility is used for Loading the in stream manifest
        /// </summary>
        /// <param name="manifestData">Memory stream containing cached contents of manifest</param>
        internal void LoadManifest(byte[] manifestData)
        {
            if (string.IsNullOrEmpty(this.dynamicManifestFolder))
            {
                return;
            }

            var loader = new ManifestLoader();
            ICollection<ProviderDefinitionDescription> providers;

            using (MemoryStream memStream = new MemoryStream(manifestData))
            {
                providers = loader.LoadManifest(memStream).Providers;
            }

            List<Guid> guidList = this.GetGuidsInManifest(providers);

            // EventSource Dynamic manifest would contain only 1 guid/provider per manifest
            FileInfo fileName =
                new FileInfo(Path.Combine(this.dynamicManifestFolder, string.Format("{0}.man", guidList[0])));
            this.UnloadManifest(fileName.FullName);
            this.WriteManifest(fileName.FullName, manifestData);
            this.AddProviders(fileName.FullName, providers);
        }

        private void WriteManifest(string manifestFileName, byte[] manifestContent)
        {
            // Don't share access until file is completely written.
            try
            {
                using (var fs = new FileStream(manifestFileName, FileMode.Create, FileAccess.Write, FileShare.None))
                {
                    fs.Write(manifestContent, 0, manifestContent.Length);
                }
            }
            catch (UnauthorizedAccessException)
            { }
        }

        private void AddProviders(string fileName, ICollection<ProviderDefinitionDescription> providers)
        {
            this.guids[fileName] = GetGuidsInManifest(providers, true);
        }

        private List<Guid> GetGuidsInManifest(ICollection<ProviderDefinitionDescription> providers, bool addProvider = false)
        {
            List<Guid> guidsInManifest = new List<Guid>();
            foreach (var provider in providers)
            {
                ProviderDefinition providerDef = new ProviderDefinition(provider);

                try
                {
                    if (addProvider)
                    {
                        this.providers.Add(providerDef.Guid, providerDef);
                    }
                }
                catch (ArgumentException e)
                {
                    throw new ArgumentException(string.Format(ProviderAlreadyLoadedMessage, providerDef.Guid), e);
                }

                guidsInManifest.Add(providerDef.Guid);
            }

            return guidsInManifest;
        }

        public void UnloadManifest(string fileName)
        {
            if (this.guids.ContainsKey(fileName))
            {
                List<Guid> guidsInManifest = this.guids[fileName];
                foreach (Guid guid in guidsInManifest)
                {
                    this.providers.Remove(guid);
                }
                this.guids.Remove(fileName);
            }
        }

        public string FormatEvent(EventRecord eventRecord)
        {
            string unusedEventType;
            string unusedEventText;

            return this.FormatEvent(eventRecord, out unusedEventType, out unusedEventText);
        }

        /// <summary>
        /// Processes an EventSource event embedded in eventstream.
        /// </summary>
        /// <param name="eventRecord"></param>
        /// <returns>true if its an eventsource event</returns>
        private bool ProcessEventSourceEvent(EventRecord eventRecord)
        {
            if (WellKnownProviderList.IsDynamicProvider(eventRecord.EventHeader.ProviderId))
            {
                if (EventSourceHelper.CheckForDynamicManifest(eventRecord.EventHeader.EventDescriptor))
                {
                    ApplicationDataReader reader = new ApplicationDataReader(eventRecord.UserData,
                        eventRecord.UserDataLength);
                    byte[] rawData = reader.ReadBytes(eventRecord.UserDataLength);
                    byte[] completeManifest = partialManifest.AddChunk(rawData);

                    if (completeManifest != null)
                    {
                        this.LoadManifest(completeManifest);
                    }

                    return true;
                }
            }

            return false;
        } 

        public string FormatEvent(EventRecord eventRecord, out string eventType, out string eventText, int formatVersion=0)
        {
            eventType = null;
            eventText = null;
            string s = null;

            // Check if this is a special event from EventSource which contains manifest
            if (ProcessEventSourceEvent(eventRecord))
            {
                // Stop further processing for this event, we don't want it to be displayed as well so return null
                return null;
            }

            if (ManifestCache.IsStringEvent(eventRecord))
            {
                s = EventFormatter.FormatStringEvent(formatVersion, eventRecord, out eventType, out eventText);
            }

            if (s == null)
            {
                ProviderDefinition providerDef;
                if (this.providers.TryGetValue(eventRecord.EventHeader.ProviderId, out providerDef))
                {
                    s = providerDef.FormatEvent(eventRecord, out eventType, out eventText, formatVersion);
                }
            }

            return s;
        }

        public static bool IsStringEvent(EventRecord eventRecord)
        {
            return ((eventRecord.EventHeader.Flags & ManifestCache.EventHeaderFlagStringOnly) == ManifestCache.EventHeaderFlagStringOnly);
        }

        public EventDefinition GetEventDefinition(EventRecord eventRecord)
        {
            // Check if this is a special event from EventSource which contains manifest
            if (ProcessEventSourceEvent(eventRecord))
            {
                // Stop further processing for this event
                return null;
            }

            if (IsStringEvent(eventRecord))
            {
                //
                // An EventDefinition object is not created for string events
                //
                return null;
            }

            //
            // Find a matching provider
            //
            ProviderDefinition providerDef;
            if (false == this.providers.TryGetValue(eventRecord.EventHeader.ProviderId, out providerDef))
            {
                //
                // Unable to find a matching provider
                //
                return null;
            }

            //
            // Use the provider to find the event definition
            //
            return providerDef.GetEventDefinition(eventRecord);
        }
    }
}