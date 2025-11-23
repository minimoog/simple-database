# Hub Wallet Messaging Library - C++
This is the Hub wallet messaging library. A C++ library used for internal & external communication with the Hub hardware cryptocurrency wallet.

## Installation
The project currently cannot be built or installed as a stand-alone project. It is intended for use within other projects.

There are no dependecies for this library.


## Usage

*All Request messages now have thier expected responses listed in a comment at the top of the header file, in addition to a description of thier usage.*

### Headers

All messages contain a header. This includes some meta data about the message, and a hash of the message data.
Let's take a look at the struct:
```c++
    struct MessageHeader
    {
        uint32_t            synchWord;          // Should always equal 0x555555d5

        //Source & destination
        SourceDestination   destination;        // SourceDestinations reflect defined entities in the firmware such as the Btc application, or Key manager
        SourceDestination   source;             // These are irrelevant for external messages (external to the firmware)

        //How to deserialize and use the message
        MessageType         typeOfMessage;      // The class of the message
        walletapp::CoinType typeOfCoin;         // The currency of the action (including none) (testnets are considred as seperate currencies)

        //Hashing 
        uint32_t            counter;            // A counter to be incremented for each message (to produce unique hashes)
        TimeTag             dateTime;           // Date that the message was created (to produce unique hashes)
        uint8_t             hashValue[32];      //A unique identifying hash (sha256)

        //Chunking properties
        uint32_t            partsNumber;        // Number of total parts for chunked messages
        uint32_t            currentPartIndex;   // Index of the current part, from zero

        uint32_t            currentPartLengthBytes; //The size of the message body (excluding the header)
    };
```
Most of these properties are set automatically when the message is serialized. Chunking properties are not.

### Types of messages
Message types are split into 4 different categories:

* Misc - KeepAlive & NotificationMessage

* Requests - Messages that are sent **to** the device

* Responses - Messages that are sent **from** the device, in response to a request

#### Special cases - Misc:
*Notification Message* - This message will be sent from applications running in the device, notifying whoever is connected about state changes in the wallet.

*Ack Message* - This reponse message will be sent from the communication layer of the device whenever a message is received, regardless of whether it's content was parsed correctly

### Sending a message
Sending a message requires first calling **SerializeMessage**, to populate the *Buffer* of the message. The *Buffer* is the binary data that should be broadcast.

Serializing the message also populates the message header, see above.

### Receiving a message
Binary data recieved from the device can be turned back into one of the message classes. Take an *empty* instance of a message and put the binary data into the message's buffer.

Once you have a message with a filled buffer it needs to be *deserialized* by calling **DeserializeMessage**. This takes the data from the *Buffer* and populates the class members of the class.

If you don't know what the type of the message is, first deserialize it as a **BaseMessage**. This will allow you to read its header, and check the *typeOfMessage* property, indicating what class it should be deserialized as.

### Outgoing messages
All messages that leave the device subclass the **ResponseMessage**. This means that they include some data about the incoming message that triggered them, in the form of a **PartialMessageHeader**.

Every message sent to the wallet should trigger exactly 2 messages, an **AckMessage** indicating that it reached the device, and a **ResponseMessage** indicating that the message was interrpreted and an action was taken.

If an error occured, or the **ResponseMessage** returned by the device does not require additional data (in the case of an **WalletActionMessage** for example), then an **ErrorResponseMessage** will be sent. This contains an error code indicating if there is an error, and what the source is. For example, 0 = None, 1 = General, 2 = User Cancellation etc.

### Multi-part messages (Chunking)
Chunked messages are identified by the *PartsNumber* property of the header being greater than 1, with the *CurrentPartIndex* indicating which part.

Only certain messages may be *chunked*, and should be handled on a case by case basis. However, we can say in general when one peice of a message is received the next peice must be requested. For outgoing messages, an **AckMessage** should be sent to the wallet, with the received peice used to construct the **PartialMessageHeader**. For ingoing messages, you will know when to send the next piece because the device will send you an **ErrorResponseMessage**

