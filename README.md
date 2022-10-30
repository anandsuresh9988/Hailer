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

![PlantUML model](http://www.plantuml.com/plantuml/png/hLDDRvmm4BtxLupIItke5Coj4bM0HGiIeMZ97df2Jh30gh6HCMtKNrydgkD8Nh6osiiUtkpxOERvRjL4wMtcQFt1n48KcQ4r27jY2n6w9SF0n0ZuzjqyayyMZsyHGcJJKcpp8rNeKJx3JIC72j4DVAZPEEcCRnGjJX9Unb6wBEb5DFFHaZ1ELKVLJ-D0nG36nTWwBsozZefIuGdWzmB-r9Zc5z73HRFOcdsddCjV7ZFUTOkDRk5qamGC09LWcX7EVXFDf80aGUXjI_3vgxy6-09bMjN5gR_lZdtQjdsTdfkyHFu_3TrfURHJSUoxSvuNNVUQfXBQS5ebd-Ywmhtv8QCvz6iaTsU8Wl957mzqrP2u8t_Q3vfTvxkmB-3_JsgRiPCKX-jWJyVEh_FD5__zJ0eFYeXraKg3dfJSnWdoYGX1-Gh_je3kd7NvgSgAFY_JfYdgAAHIbcHbseVgM-FwzVQmt0R3Wkmiy10KnBv4gn071udz85Op80XoZRz-6snBOOE4TpDK9WwH2MeA3Zed3jevHdTK3DlC_m00)


Notes:

Prerequisites

libjson-c-dev
