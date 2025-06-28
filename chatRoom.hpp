#include <iostream>

#ifndef CHATROOM_HPP
#define CHATROOM_HPP

#include "message.hpp"
#include <deque>
#include <set>
#include <memory>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

// This is an abstract base class for anything that can send and receive messages.
// It is just like a client that can talk in the chatroom.
// Any class that joins the room must implement these.
class Participant {
    public: 
        virtual void deliver(Message& message) = 0; // how the chatroom sends a message to this participant
        virtual void write(Message &message) = 0; // how this participant sends a message out
        virtual ~Participant() = default;
};

typedef std::shared_ptr<Participant> ParticipantPointer;

// It is core manager of all connected users (particiants)
/*
    1. It stores all connected users.
    2. It stores recent messages to broadcast.
*/ 
class Room{
    public:
        void join(ParticipantPointer participant); // adds a new client(Participant) to the room
        void leave(ParticipantPointer participant); // remove a client when they disconnect.
        void deliver(ParticipantPointer participantPointer, Message &message); // sends a message to all connected clients(broadcast) except sender.
    private:
        std::deque<Message> messageQueue; // queue of recent messages
        enum {maxParticipants = 100};
        std::set<ParticipantPointer> participants; // set of Participant objects
};

// Session <-> A Connected Client
/*
    1. It implements Participant interface.
    2. Handles:-
        -> Reading messages from the client.
        -> Writing messages to the client.
        -> Joining/leaving the room.
*/
class Session: public Participant, public std::enable_shared_from_this<Session>{
    public:
        Session(tcp::socket s, Room &room); // sets up the session with a socket and reference to the shared chatroom
        void start(); // Joins the room and starts reading from client socket.
        void deliver(Message& message); // Called when server wants to send a message to this client.
        void write(Message &message); // Sends a message to the client socket.
        void async_read(); // Asynchronously waits to read a message from client
        void async_write(std::string messageBody, size_t messageLength); // Asynchronously sends message to client socket.
    private:
        tcp::socket clientSocket;
        boost::asio::streambuf buffer;
        Room& room;
        std::deque<Message> messageQueue; 
};

#endif CHATROOM_HPP

/*
Session = handles 1 client
Room = manages all clients
Message = packages the chat text
Participant = abstract interface to keep it flexible
*/