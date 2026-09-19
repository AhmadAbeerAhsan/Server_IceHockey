#include <boost/asio.hpp>
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
#include <format>
#include <string>
#include <array>
#include <vector>
#include <sstream>

using boost::asio::ip::udp;

#include <chrono>
#include <format>

constexpr unsigned short kServerPort = 8080;

std::string make_daytime_string()
{
    auto now = std::chrono::system_clock::now();
    return std::format("{:%a %b %d %H:%M:%S %Y}", std::chrono::floor<std::chrono::seconds>(now));
}

class ChatMessage
{
public:
    explicit ChatMessage(std::string _participant, std::string _message) :
        m_participant(std::move(_participant)),
        m_message(std::move(_message)),
        m_time(make_daytime_string())
    {
    }

    std::string get_formatted_message() const {
        return std::format("[{}] {}: {}", m_time, m_participant, m_message);
    }
private:
    std::string m_participant;
    std::string m_message;
    std::string m_time;
};

class ChatRoom
{
public:
    explicit ChatRoom(std::string _chat_room_name):
        m_chat_room_name(_chat_room_name)
    {}
    
    void AddParticipant(std::shared_ptr<udp::endpoint> participant_endpoint_ptr)
    {
        for (const std::shared_ptr<udp::endpoint>& existing_endpoint_ptr : m_participants_endpoint_ptrs)
        {
            if (*existing_endpoint_ptr == *participant_endpoint_ptr)
            {
                std::cout << "Error: Participant already in chat" << '\n';
                return;
            }
        }
        m_participants_endpoint_ptrs.push_back(participant_endpoint_ptr);
    }

    template <typename Fun>
    void SendMessageToAll(Fun&& fun)
    {
        for (size_t i = 0; i < m_participants_endpoint_ptrs.size(); i++)
        {
            fun(m_participants_endpoint_ptrs[i]);
        }
    }

    template <typename Fun>
    void RemoveParticipant(Fun&& fun)
    {
        std::erase_if(m_participants_endpoint_ptrs, fun);
    }

    std::string AddMessage(std::string participant, std::string message)
    {
        m_chat_messages.emplace_back(std::move(participant), std::move(message));
        return m_chat_messages.back().get_formatted_message();
    }
private:
    std::string m_chat_room_name;
    std::vector<std::shared_ptr<udp::endpoint>> m_participants_endpoint_ptrs{ };
    std::vector<ChatMessage> m_chat_messages { };
};

class UdpServer
{
public:
    UdpServer(boost::asio::io_context& io):
        m_socket(io, udp::endpoint(udp::v4(), kServerPort))
    {
        StartRecieve();
    }
private:
    udp::socket m_socket;
    std::array<char,128> recv_buf;

    std::unordered_map<std::string, ChatRoom> m_chat_rooms {};

    void StartRecieve()
    {
        std::shared_ptr<udp::endpoint> remote_endpoint_ptr{ std::make_shared<udp::endpoint>() };;
        m_socket.async_receive_from(
            boost::asio::buffer(recv_buf),
            *remote_endpoint_ptr,
            std::bind(
                &UdpServer::HandleRecieve, this, remote_endpoint_ptr, boost::asio::placeholders::bytes_transferred
            )
        );
    }

    void HandleRecieve(std::shared_ptr<udp::endpoint> remote_endpoint_ptr, std::size_t len)
    {
        std::string message{recv_buf.data(), len};
        InterpretMessage(message, remote_endpoint_ptr);

        StartRecieve();
    }

    void HandleSend(std::shared_ptr<std::string> message_ptr)
    {

    }

    const std::string connect{"Connect"};
    const std::string leave{"Leave"};
    const std::string join{"Join"};
    const char deliminator{'|'};
    const std::string listChats{"ListChats"};
    const std::string chat{"Chat"};

    void InterpretMessage(std::string& message, std::shared_ptr<udp::endpoint> remote_endpoint_ptr)
    {
        std::cout << "Message: " << message << '\n';
        if (message == connect)
        {
            std::cout << "Connect called" << '\n';
            std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>("Connected To Server...")};
            m_socket.async_send_to(
                boost::asio::buffer(*message_ptr),
                *remote_endpoint_ptr,
                std::bind(
                    &UdpServer::HandleSend,
                    this,
                    message_ptr
                )
            );
            return;
        }

        if (message == listChats)
        {
            std::cout << "listChats called" << '\n';
            std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>(GetChatLists())};
            m_socket.async_send_to(
                boost::asio::buffer(*message_ptr),
                *remote_endpoint_ptr,
                std::bind(
                    &UdpServer::HandleSend,
                    this,
                    message_ptr
                )
            );
            return;
        }
        
        std::vector<std::string> message_parts {};
        size_t start = 0;
        while(true)
        {
            size_t pos = message.find(deliminator, start);
            message_parts.emplace_back(message, start, pos == std::string::npos ? std::string::npos : pos - start);
            if (pos == std::string::npos) break;
            start = pos + 1;
        }

        if (message_parts.size() == 3)
        {
            if (message_parts[0] == leave)
            {
                std::cout << "leave called" << '\n';
                auto it = m_chat_rooms.find(message_parts[1]);
                if (it == m_chat_rooms.end())
                {
                    std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>(
                        std::format("Server: {} chat room does not exist!", message_parts[1])
                    )};
                    m_socket.async_send_to(
                        boost::asio::buffer(*message_ptr),
                        *remote_endpoint_ptr,
                        std::bind(
                            &UdpServer::HandleSend,
                            this,
                            message_ptr
                        )
                    );
                    return;
                }
                std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>(message_parts[2] + " left Chatroom " + message_parts[1])};
                it->second.RemoveParticipant(
                    [this, message_ptr, remote_endpoint_ptr](std::shared_ptr<udp::endpoint> participant_endpoint_ptr) -> bool{
                        m_socket.async_send_to(
                            boost::asio::buffer(*message_ptr),
                            *participant_endpoint_ptr,
                            std::bind(
                                &UdpServer::HandleSend,
                                this,
                                message_ptr
                            )
                        );
                        return *participant_endpoint_ptr == *remote_endpoint_ptr;
                    }
                );
                return;
            }
            if (message_parts[0] == join)
            {
                std::cout << "join called" << '\n';
                std::string action;
                auto it = m_chat_rooms.find(message_parts[1]);
                if (it == m_chat_rooms.end())
                {
                    auto result = m_chat_rooms.try_emplace(message_parts[1], message_parts[1]);
                    it = result.first;
                    it->second.AddParticipant(remote_endpoint_ptr);
                    action = "Created";
                }
                else
                {
                    it->second.AddParticipant(remote_endpoint_ptr);
                    action = "Joined";
                }
                
                std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>(
                    std::format("{} {} Chatroom {}", message_parts[2], action, message_parts[1])
                )};
                it->second.SendMessageToAll(
                    [this, message_ptr](std::shared_ptr<udp::endpoint> participant_endpoint_ptr){
                        m_socket.async_send_to(
                            boost::asio::buffer(*message_ptr),
                            *participant_endpoint_ptr,
                            std::bind(
                                &UdpServer::HandleSend,
                                this,
                                message_ptr
                            )
                        );
                    }
                );
                return;
            }
        }
        if (message_parts.size() == 4)
        {
            if (message_parts[0] == chat)
            {
                auto it = m_chat_rooms.find(message_parts[1]);
                if (it == m_chat_rooms.end())
                {
                    std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>(
                        std::format("Server: {} chat room does not exist!", message_parts[1])
                    )};
                    m_socket.async_send_to(
                        boost::asio::buffer(*message_ptr),
                        *remote_endpoint_ptr,
                        std::bind(
                            &UdpServer::HandleSend,
                            this,
                            message_ptr
                        )
                    );
                    return;
                }
                std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>(
                    it->second.AddMessage(message_parts[2], message_parts[3])
                )};
                it->second.SendMessageToAll(
                    [this, message_ptr](std::shared_ptr<udp::endpoint> participant_endpoint_ptr){
                        m_socket.async_send_to(
                            boost::asio::buffer(*message_ptr),
                            *participant_endpoint_ptr,
                            std::bind(
                                &UdpServer::HandleSend,
                                this,
                                message_ptr
                            )
                        );
                    }
                );
                return;
            }
        }
    }
    
    std::string GetChatLists()
    {
        std::string result;
        for (const auto& [room_name, room] : m_chat_rooms)
        {
            if (!result.empty())
            {
                result += deliminator;
            }
            result += room_name;
        }
        return result;
    }
    
};

int main(int argc, char* argv[])
{
    std::cout << "Hello World" << std::endl;
    try
    {   
        boost::asio::io_context io;
    
        UdpServer udpServer(io);

        io.run();        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}