#include "chatRoom.hpp"

/*
when client joins, it should join a room.
hence a session(socket, room) will be created and this client will have a session
now, whenever it wants to start its delivery it call call start() function, where it will listen for incoming messages and push to the message Queue of the room
when client wants to send message it can call session's deliver() message
session will call deliver() to deliver the message to the room
room will call write() function to write any message to the client's queue
It will trigger the write() for each participant except the sender itself
*/

void Room::join(ParticipantPointer participant){ // Adds a new client (Participant) to the room's set
    this->participants.insert(participant);
}

void Room::leave(ParticipantPointer participant){ // Removes a client from the room when they disconnect
    this->participants.erase(participant);
}
// broadcast logic
void Room::deliver(ParticipantPointer participant, Message &message){
    messageQueue.push_back(message); // puts the message into a queue
    while (!messageQueue.empty()) {
        Message msg = messageQueue.front();
        messageQueue.pop_front(); 
        // sends it to all other client(not sender)
        for (ParticipantPointer _participant : participants) {
            if (participant != _participant) { // except sender, calls write() on each of them to deliver
                _participant->write(msg);
            }
        }
    }
}

void Session::async_read() {
    auto self(shared_from_this());
    // waits until a full message (ending with "\n") is received
    boost::asio::async_read_until(clientSocket, buffer, "\n",
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::string data(boost::asio::buffers_begin(buffer.data()), 
                                 boost::asio::buffers_begin(buffer.data()) + bytes_transferred);
                buffer.consume(bytes_transferred);
                std::cout << "Received: " << data << std::endl;
                
                Message message(data); // creates a Message object
                deliver(message); // calls deliver()-> which sends it to the room and eventually to others
                async_read(); // then keeps listening again via recursive async_read() call.
            } else {
                room.leave(shared_from_this());
                if (ec == boost::asio::error::eof) {
                    std::cout << "Connection closed by peer" << std::endl;
                } else {
                    std::cout << "Read error: " << ec.message() << std::endl;
                }
            }
        }
    );
}

// It is low level send function. Asynchronously write the message data to the socket. On completion prints status.
void Session::async_write(std::string messageBody, size_t messageLength){
    auto write_handler = [&](boost::system::error_code ec, std::size_t bytes_transferred){
        if(!ec){
            std::cout<<"Data is written to the socket: "<<std::endl;
        }else{
            std::cerr << "Write error: " << ec.message() << std::endl;
        }
    };
    boost::asio::async_write(clientSocket, boost::asio::buffer(messageBody, messageLength), write_handler);
}
// called when a new client connects
void Session::start(){
    room.join(shared_from_this()); // It joins the room
    async_read(); // starts loop and keeps listening.
}

Session::Session(tcp::socket s, Room& r): clientSocket(std::move(s)), room(r){};

// Used when this client is receiving a message from others.
void Session::write(Message &message){
    messageQueue.push_back(message); // message is queued.
    while(messageQueue.size() != 0){
        // for each message->header is decoded->message body is extracted->message is written to the client socket using async_write(...)
        Message message = messageQueue.front();
        messageQueue.pop_front();
        bool header_decode = message.decodeHeader(); //1
        if(header_decode){
            std::string body = message.getBody(); // 2
            async_write(body, message.getBodyLength()); // 3
        }else{
            std::cout<<"Message length exceeds the maximum length"<<std::endl;
        }
    }
}
// It is called when this client sends a message
// Passes the message to the room's deliver function so the room can broadcast it.
void Session::deliver(Message& incomingMessage){
    room.deliver(shared_from_this(), incomingMessage);
}
//-----------
using boost::asio::ip::address_v4;
// recursive function that keeps accepting new clients.
void accept_connection(boost::asio::io_context &io, char *port,tcp::acceptor &acceptor, Room &room, const tcp::endpoint& endpoint) {
    tcp::socket socket(io);
    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
        if(!ec) {
            std::shared_ptr<Session> session = std::make_shared<Session>(std::move(socket), room); // Creates a new Session object
            session->start(); // starts it
        }
        accept_connection(io, port,acceptor, room, endpoint); // recursively calls itself again to wait for next client
    });
}

// sets up the server
int main(int argc, char *argv[]) {
    try {
        if(argc < 2) { // parses the port number
            std::cerr << "Usage: server <port> [<port> ...]\n";
            return 1;
        }
        Room room;
        boost::asio::io_context io_context; // Initializes Boost ASIO's I/O context
        tcp::endpoint endpoint(tcp::v4(), atoi(argv[1]));
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), std::atoi(argv[1]))); // sets up socket acceptor
        accept_connection(io_context, argv[1], acceptor, room, endpoint); // starts accepting clientss
        io_context.run(); // runs the event loop
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}

/*
Chat flow summary:-
Client connects → Session created → Session joins Room
Client sends message:
  async_read → create Message → deliver() to Room
Room:
  deliver() → calls write() on all other Sessions
Each Session:
  write() → async_write() to their client socket
*/