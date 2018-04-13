# Naming

The Naming subsystem securely resolves service names to location in the service fabric Stores and retrieves properties (metadata) for a service. It is implemented as a partitioned, reliable, persistent Service Fabric service for storage of name, service, and property data.
Clients can communicate with any node in the fabric to resolve service name
