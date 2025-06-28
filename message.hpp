#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <iostream>

/*
Overall working of this code:-
The Message class represents one message in your chatroom, and it's built to:

1. Wrap the message data into a format that can be safely sent over a network.
2. Include a 4-byte header to store the size of the message body (like a label saying "how long this message is").
3. It allows easy:-
    -->Encoding before sending (client side),
    -->Decoding after receiving (server side).
*/

/* 
header is of 4 bytes and maxBytes can be stored as 512 bytes
header stores the body length that is the current body length
data stores the header+bodyLength with maximum size of header+maxBytes

client attempts to send message:- It will encode header and put message into the data and send data.
Server gets the message, decodes the header, get the bodylength from the header and hence complete body.
Then server sends the message to all the clients connected to that room.
*/



// A helper class that makes sure each message has a proper size tag (header) and body, so that messages can be cleanly sent and parsed between client and server.
class Message {
    public: 
        Message() : bodyLength_(0) {} // default constructor, starts with empty message
        
        enum {maxBytes = 512};
        enum {header = 4};

        Message(std::string message){ // constructor to prepare message to send
            // 1. it sets body length.
            bodyLength_ = getNewBodyLength(message.size());
            encodeHeader(); // 2. encodes header
            std::memcpy(data + header, message.c_str(), bodyLength_); // 3. and copies message to data buffer.
        };

        size_t getNewBodyLength(size_t newLength){ // helper method which ensures body isn't longer than 512 bytes
            if(newLength > maxBytes){
                return maxBytes;
            }
            return newLength;
        }

        void printMessage(){ // printing received message for debugging purpose
            std::string message = getData();
            std::cout<<"Message recieved: "<<message<<std::endl;
        }

        std::string getData(){ // returns the full message (header+body) as a single string.
            int length = header + bodyLength_;
            std::string result(data, length);
            return result;
        }

        std::string getBody(){ // extracts only the message content excluding header.
            std::string dataStr = getData();
            std::string result = dataStr.substr(header, bodyLength_);
            return result;
        }

        void encodeHeader(){ // puts the body length into the first 4 bytes of data.
            char new_header[header+1] = "";
            sprintf(new_header, "%4d", static_cast<int>(bodyLength_));
            memcpy(data, new_header, header);
        }
        
       bool decodeHeader(){ // Reads first 4 bytes of incoming data and sets bodyLength_.
        // Returns false if it is invalid
            char new_header[header+1] = "";
            strncpy(new_header, data, header);
            new_header[header] = '\0';
            int headerValue = atoi(new_header);
            if(headerValue > maxBytes){
                bodyLength_ = 0;
                return false;
            }
            bodyLength_ = headerValue;
            return true;
        }

        size_t getBodyLength(){ // returns how long the message body is.
            return bodyLength_;
        }

    private: 
        char data[header+maxBytes];
        size_t bodyLength_;
};

#endif

/*
1. Client side:- creates Message("hello") -> internally sets header to "0005"-->packs "0005hello" into data[] -> sends data[] to server
2. Server side:- receives data into a Message object-> Calls decodeHeader() to extract length (5)->Calls getBody() to get "hello"->Broadcasts that message to all connected clients
*/
