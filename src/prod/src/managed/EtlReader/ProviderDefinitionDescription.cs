// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Xml;
    using System.Xml.XPath;

    /// <summary>
    /// This class contains all the data needed to describe a provider
    /// It has no ability to parse an event by itself
    /// </summary>
    public class ProviderDefinitionDescription
    {
        private readonly Guid guid;
        private readonly string name;
        private readonly Dictionary<int, EventDefinitionDescription> events;
        private readonly ValueMaps valueMaps;

        public ProviderDefinitionDescription(IXPathNavigable inputProvider, StringTable strings)
        {
            if (strings == null)
            {
                throw new ArgumentNullException("strings");
            }

            if (inputProvider == null)
            {
                throw new ArgumentNullException("inputProvider");
            }

            XmlElement provider = inputProvider as XmlElement;
            if (provider == null)
            {
                throw new ArgumentException(StringResources.ETLReaderError_InvalidXmlElement_InputProvider);
            }

            this.guid = new Guid(provider.GetAttribute("guid"));
            bool wellKnownProviderId = WellKnownProviderList.IsWellKnownProvider(this.guid);
            bool dynamicManifest = WellKnownProviderList.IsDynamicProvider(this.guid);
            this.name = provider.GetAttribute("name");
            this.events = new Dictionary<int, EventDefinitionDescription>();

            XmlElement mapElements = provider["maps"];
            this.valueMaps = new ValueMaps(mapElements, strings);

            XmlElement eventElements = provider["events"];
            XmlElement templates = provider["templates"];
            string eventSourceName = dynamicManifest ? this.name : null;

            foreach (XmlElement evt in eventElements.GetElementsByTagName("event"))
            {
                string tid = evt.GetAttribute("template");
                XmlElement template = null;

                foreach (XmlElement t in templates.GetElementsByTagName("template"))
                {
                    if (tid == t.GetAttribute("tid"))
                    {
                        template = t;
                        break;
                    }
                }

                var eventDef = new EventDefinitionDescription(evt, template, strings, wellKnownProviderId, eventSourceName);
                
                // We are not using Version information as key here. This is intentional while we make some structural changes
                // to handle scenarios where a single file can have events with multiple versions. Till these changes can materialize,
                // we have to handle scenario where we have same events with multiple versions in the same .etl file.
                this.events.Add(eventDef.Id, eventDef);
            }
        }

        internal ProviderDefinitionDescription(Guid guid, string name, Dictionary<int, EventDefinitionDescription> events)
        {
            this.guid = guid;
            this.name = name;
            this.events = events;
        }

        public Guid Guid
        {
            get
            {
                return this.guid;
            }
        }

        public string Name
        {
            get
            {
                return this.name;
            }
        }

        public Dictionary<int, EventDefinitionDescription> Events
        {
            get { return this.events; }
        }

        public ValueMaps ValueMaps
        {
            get { return this.valueMaps; }
        }

        internal string FormatEvent(EventStack stack, EventRecord eventRecord)
        {
            string unusedEventType;
            string unusedEventText;

            return this.FormatEvent(stack, eventRecord, out unusedEventType, out unusedEventText);
        }

        internal string FormatEvent(EventStack stack, EventRecord eventRecord, out string eventType, out string eventText, int formatVersion = 0)
        {
            eventType = null;
            eventText = null;

            EventDefinitionDescription eventDef;
            if (!this.events.TryGetValue(eventRecord.EventHeader.EventDescriptor.Id, out eventDef))
            {
                return null;
            }

            return eventDef.FormatEvent(stack, eventRecord, this.valueMaps, out eventType, out eventText, formatVersion);
        }

        public EventDefinitionDescription GetEventDefinition(EventRecord eventRecord)
        {
            //
            // Using the event key, find the matching event definition
            //
            EventDefinitionDescription eventDef;
            if (!this.events.TryGetValue(eventRecord.EventHeader.EventDescriptor.Id, out eventDef))
            {
                //
                // No matching event definition found
                //
                return null;
            }

            return eventDef;
        }
    }
}