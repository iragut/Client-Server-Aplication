# Client-Server application
### Description :
We need to implement a server what has a role of a broker for the **UDP and TCP clients**,  
**TCP** are a simple client who connect and subscribe to  a specific topic and **UDP** are the clients 
who public the topics.  
### Server description:
Accept only one command and this is **exit** who close all connection and the
server. In rest the server wait for the event to happen then check if is a new
connection with a **TCP client** and accept it or close the connection if 
the already client is connected to the server, then wait for the commnad 
from client to know if they are subscribed to some topic, or if is **UDP client**
and check if the connected clients are subscribed to that topic and send the 
message to client.
### Server implentation:

   I created two initial sockets for **UDP** and **TCP** when i added to the poll with the **stdin** file descriptor. 
   We create **two map** for store the sockets from the client and they subscribed
   topics, the key are the id client.  When we recive a event from a the UDP socket ,we expect a packet like this:
   ```C++
    typedef struct udp_header {
	    char topic[MAX_TOPIC_LEN]; 
	    uint8_t data_type;
	    char content[MAX_CONTENT_LEN];
    } udp_header;
  ```
  Then we iterate through **map** ancd check if someone has the subscribed to specif topic, 
if we find one we send to it using thes struct:(first we send the header after the content)
```C++
// Header for tcp protocol
  typedef struct header{
      int port;
      int size;
      uint32_t ip;
      uint8_t data_type;
      char topic[MAX_TOPIC_LEN];
  } header;

  // Packet for tcp protocol
  typedef struct packet {
      header head_packet;
      char *content;
  } packet;
```
  to open the server use this commnad: 
  ``` 
  ./server <PORT>
  ```
  ### Client / Subscriber description:
  We connect to the server, then we can **subscribe, unsubscribe** from a topic
  and **disconnect** from the server  ,then we recive a packet we print the information from the packet.
  ### Implementation:
  We create the socket to connect to server and deactivate the **(Nagle algorithm)** . 
  Then we wait the for the events ,when the event are from **stdin** we check if is a valid commnad and 
  send to server, otherthise we notify the user that the command is invalid. 
  If the event is from the socket when we recived a packet so we need to print it ,
  we check what **data_type** are and substract from the content using this formulas:
  

 **- if data is int we know what content is like this :**
			    Sign byte* followed by a **uint32_t** formatted in the network byte order
**- if data is short:**
	**uint16_t** representing the modulus of the number multiplied by 100
**- if data is float then:**
	A sign byte, followed by a **uint32_t** (in network
byte order) representing the modulus of the number obtained from
pasting the whole part to the decimal part of the number,
followed by a **uint8_t** which represents the modulus of the negative power of
10 with which it must be multiplied, it the modulus for
to obtain, holds the original number (in the module)
**- if data is string we simple print it**

to open a client use this command:

```
./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>
```
