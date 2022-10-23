# Hailer

This repo contains the source code for Hailer server application and Hailer library(hailer.so)

# About Hailer

Hailer is an inter-node message transportation framework which is developed for sending and recieving messages between devices which are connected to the same Local Area Network. 

Hailer consists of a server application which runs in all nodes that needs to use the common communication framework.The main function of the hailer server is to listen to messages from aplplications running in the same node or nodes connected to the same LAN and route those messages to correct desitnation which might be inside or outside of the node in which the server application runs.

Hailer also provides a library which contains various APIs which can be used by the applications to send messages to other applications within the same node or other nodes in the same network in which a hailer server runs.

# Main features of Hailer
- Provides a inter-process communication mechanism to send messages between applications running in the same node.
- Provides a inter-node communication mechanism to send messages between nodes in the same LAN.
- Provides a node discovery mechanism which help each node in the network to be aware of other nodes, their capabilities and other informations.
- Provides a library which helps Applications using hailer to register with hailer server, send and recieve messges.

# Message sequence diagram

```plantuml
@startuml
skinparam ParticipantPadding 20
skinparam BoxPadding 30


autonumber
box "Node 1" #LightBlue
participant Node1AppA
participant Node1AppB
participant Node1HailerServer
end box


box "Node 2" #LightGreen
participant Node2HailerServer
participant Node2AppC
participant Node2AppD
end box


box "Node 3" #LightYellow
participant Node3HailerServer
participant Node3AppE
end box

== App Initialization ==

Node1AppA -> Node1HailerServer : HAILER_MSG_APP_LAUNCH
Node1HailerServer -> Node1AppA : HAILER_MSG_APP_REG_SUCCESS

== Inter Process Messaging ==

autonumber 1
Node1AppA -> Node1HailerServer : HAILER_MSG_1(Dest = Node1AppB)
Node1HailerServer -> Node1AppB : HAILER_MSG_1(Dest = Node1AppB)

== Inter Node Messaging ==

autonumber 1
Node1AppA -> Node1HailerServer : HAILER_MSG_1(Dest = Node2AppC)
Node1HailerServer -> Node2HailerServer : HAILER_MSG_1(Dest = Node2AppC)
Node2HailerServer -> Node2AppC : HAILER_MSG_1(Dest = Node2AppC)

== Message Transport through Channels  ==

autonumber 1
Node1AppA -> Node1HailerServer : HAILER_MSG_SUBSCRIBE_CHANNEL1
Node1HailerServer -> Node1AppA : HAILER_MSG_CHANNEL_SUBSCRIPTION_SUCCESS

Node2AppD -> Node2HailerServer : HAILER_MSG_SUBSCRIBE_CHANNEL1
Node2HailerServer -> Node2AppD : HAILER_MSG_CHANNEL_SUBSCRIPTION_SUCCESS

Node3AppE -> Node3HailerServer : HAILER_MSG_SUBSCRIBE_CHANNEL1
Node3HailerServer -> Node3AppE : HAILER_MSG_CHANNEL_SUBSCRIPTION_SUCCESS


Node1AppB -> Node1HailerServer : HAILER_MSG_2(Dest = CHANNEL1)
Node1HailerServer -> Node2HailerServer : HAILER_MSG_2(Dest = CHANNEL1)
Node1HailerServer -> Node3HailerServer : HAILER_MSG_2(Dest = CHANNEL1)
Node1HailerServer -> Node1AppA : HAILER_MSG_2
Node2HailerServer -> Node2AppD : HAILER_MSG_2
Node3HailerServer -> Node3AppE : HAILER_MSG_2

@enduml

```




