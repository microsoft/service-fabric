// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryConfigParser
{
    using System;
    using System.Collections.Generic;
    using Tools.EtlReader;

    // this class implements an utility for the semantics analyzer to validate configuration files based on the etw events manifest files
    public class ManifestEventFieldsLookup
    {
        private const string LogSourceId = "ManifestEventFieldsLookup";

        // the format of this dictionary is <<TaskName, EventName>, Dictionary<fieldName, fieldDefinition>>
        // this allows for querying etw events (taskname, eventname) fast as well as querying their respective fields fast
        private readonly Dictionary<Tuple<string, string>, Dictionary<string, FieldDefinition>> events;

        public ManifestEventFieldsLookup(IEnumerable<string> manifestFilePaths, LogErrorDelegate logErrorDelegate, bool verbose)
        {
            int taskEventNamesCollisionCount = 0;

            // constructing the lookup dictionary
            this.events = new Dictionary<Tuple<string, string>, Dictionary<string, FieldDefinition>>();

            // iterating over all manifest files
            foreach (var manifestFile in manifestFilePaths)
            {
                ManifestLoader manifestLoader = new ManifestLoader();
                ManifestDefinitionDescription manifestDefinitionDescription = manifestLoader.LoadManifest(manifestFile);

                // iterating over all providers in the manifest file
                foreach (var provider in manifestDefinitionDescription.Providers)
                {
                    // including all events from the provider to the dictionary
                    foreach (var ev in provider.Events.Values)
                    {
                        try
                        {
                            Dictionary<string, FieldDefinition> fieldDictionary = new Dictionary<string, FieldDefinition>();
                            this.events.Add(Tuple.Create(ev.TaskName, ev.EventName), fieldDictionary);

                            // including all fields from the event
                            foreach (var field in ev.Fields)
                            {
                                fieldDictionary.Add(field.Name, field);
                            }
                        }
                        catch (Exception)
                        {
                            if (verbose)
                            {
                                logErrorDelegate(LogSourceId, "(TaskName: {0}, EventName: {1}) collision for UnparsedEventName: {2}", ev.TaskName, ev.EventName, ev.UnparsedEventName);
                            }

                            taskEventNamesCollisionCount++;
                        }
                    }
                }

                if (taskEventNamesCollisionCount > 0)
                {
                    logErrorDelegate(LogSourceId, "Warning - Found {0} collisions on pairs (TaskName, EventName). Parser is using first occurence. For more details use verbose mode.", taskEventNamesCollisionCount);
                }
            }
        }

        public bool TryGetEvent(string taskName, string eventName, out Dictionary<string, FieldDefinition> eventFields)
        {
            return this.events.TryGetValue(Tuple.Create(taskName, eventName), out eventFields);
        }

        public bool TryGetField(string taskName, string eventName, string fieldName, out FieldDefinition fieldDef)
        {
            fieldDef = null;
            Dictionary<string, FieldDefinition> eventFields;

            if (this.TryGetEvent(taskName, eventName, out eventFields))
            {
                return eventFields.TryGetValue(fieldName, out fieldDef);
            }

            return false;
        }
    }
}