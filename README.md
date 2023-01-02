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

$\color{red}{NOTE:\ Message\ channels\ are\ not\ implemented\ yet.}$

# Hailer peer discovery

In addition to the messaging, Hailer also contains a peer discovery mechanism, by which every device in a network with Hailer installed will be notified about other peers with Hailer installed.

- Peer discovery uses Broadcast messages at regaular intervals to inform other devices in the network about device capabilities and other info.
- Broadcast messages contains JSON formatted information like device uptime, MAC address etc. New info can be easily added based on requirement.
- The Peer details including the IP address will be stored in a shared memory and can be accessed by any applications. This info can be used by              applications to get required peer details. For example an application can use the IP address of the peer to send a msg to it.
- Client apps can intialse and access the Hailer shared memory using the below API's in libhailer

```
API to access shared meory at client side

    hailerShmlist_t *hailer_client_shmlist_init(void)
    
API's to lock and unlock the semaphore for synchronising the shared memory access

    void hailer_shmList_lock(int sem_lock_id)
    void hailer_shmList_unlock(int sem_lock_id)
    
```

# Hailer CLI

Hailer also provides a CLI tool which can be used to set/get various data from the Hailer server. Currently supported commands are:

```
Usage: hailer_cli -s or --show                -  Dump all available info about the peers in the network
       hailer_cli -l or --loglevel [loglevel] -  Get/Set hailer loglevel
       hailer_cli -h or --help                -  Display hailer CLI help text
```

For example hailer_cli -s command can be used to disaply the info of all peers in the network as shown below:

```
$ sudo ../bin/bin/hailer_cli -s

    HAILER - Inter/intra node communication  Library


IP             : 192.168.1.191
Last seen time : Fri 2022-12-30 13:04:58 GMT
Peer Uptime    : 6 mins, 20 secs
MAC            : dc:a6:xx:xx:xx:xx


IP             : 192.168.1.192
Last seen time : Fri 2022-12-30 13:05:00 GMT
Peer Uptime    : 6 mins, 5 secs
MAC            : dc:a6:xx:xx:xx:xx
```
# How to install?

### Prerequisites

1) libjson-c-dev

  Install command:
  ```
  sudo apt install libjson-c-dev
  ```
### Compile and install

```
    cd Hailer
    ./configure --with-tests prefix=/home/.../Hailer/bin/
    make
    make install
```
Output files will be available in the Hailer/bin/ directory as shown below:

```
pi@raspberrypi:~/Hailer/bin $ tree
.
├── bin
│   ├── hailer_cli
│   ├── hailer_server
│   ├── hailer_test_a
│   └── hailer_test_b
├── include
│   ├── config.h
│   ├── hailer_app_id.h
│   ├── hailer.h
│   ├── hailer_msgtype.h
│   ├── hailer_peer_discovery.h
│   └── hailer_server.h
└── lib
    ├── libhailer-0.1.so.0 -> libhailer-0.1.so.0.0.0
    ├── libhailer-0.1.so.0.0.0
    ├── libhailer.a
    ├── libhailer.la
    ├── libhailer.so -> libhailer-0.1.so.0.0.0
    └── pkgconfig
        └── hailer.pc
```

# How to test?

#### Start Hailer server

```
hailer_server <LAN interface> &
```
**Note:** Hailer Server will be using the MAC address of the LAN interfcae passed to it in the peer discovery packets

To test Hailer messaging capabilities, test apps are provided in the [Hailer/test-app](https://github.com/anandsuresh9988/Hailer/tree/main/test-app) folder. [test_hailer.sh](https://github.com/anandsuresh9988/Hailer/blob/develop/test-app/test_hailer.sh) script is provided in the same folder, which will kill running test apps and restart hailer server.

```
cd Hailer/test-app
sudo ./test_hailer.sh  - This will kill runnting test apps and restart hailer server.
```

#### Testing inter-app messaging within same node

After running hailer server, run the test apps as shown:

```
sudo ./hailer_test_a "127.0.0.1" "127.0.0.1" &
sudo ./hailer_test_b "127.0.0.1" "127.0.0.1" &

Here 127.0.0.1(localhost) indicates that both the sender and recieving apps are within the same node
```
We should be able to see output like shown below:

```
App B Recieved msg_type=6 from Appid=2 SndrIp=127.0.0.1
App A Recieved msg_type=7 from Appid=3 SndrIp=127.0.0.1
App B Recieved msg_type=6 from Appid=2 SndrIp=127.0.0.1
App A Recieved msg_type=7 from Appid=3 SndrIp=127.0.0.1
```

#### Testing inter-node messaging

     1) Connect both the nodes to the same network.
     2) After running hailer server, run the test apps. This has to be done in both nodes.

```
sudo ./hailer_test_<a/b> <IP address of the node in which test app is running> <IP address of the other node> &

Eg: node1
sudo ./hailer_test_a "192.168.1.10" "192.168.1.11" &
sudo ./hailer_test_b "192.168.1.10" "192.168.1.11" &

Eg: node2
sudo ./hailer_test_a "192.168.1.11" "192.168.1.10" &
sudo ./hailer_test_b "192.168.1.11" "192.168.1.10" &
```

In the node with IP address "192.168.1.10", we should be able to see below messages.
```
App A Recieved msg_type=7 from Appid=3 SndrIp=192.168.1.11
App B Recieved msg_type=6 from Appid=2 SndrIp=192.168.1.11
App A Recieved msg_type=7 from Appid=3 SndrIp=192.168.1.11
App B Recieved msg_type=6 from Appid=2 SndrIp=192.168.1.11
```

In the node with IP address "192.168.1.11", we should be able to see below messages.
```
App A Recieved msg_type=7 from Appid=3 SndrIp=192.168.1.10
App B Recieved msg_type=6 from Appid=2 SndrIp=192.168.1.10
App A Recieved msg_type=7 from Appid=3 SndrIp=192.168.1.10
App B Recieved msg_type=6 from Appid=2 SndrIp=192.168.1.10
```
