#include <functional>
#include <thread>
#include <chrono>
#include <format>
#include <array>
#include <sstream>

#include <chrono>

#include "UdpServer.hpp"
#include "TcpServer.hpp"

bool ParsePort(const char* arg, unsigned short& outPort)
{
    int value{};
    const char* end = arg + std::strlen(arg);
    auto result = std::from_chars(arg, end, value);

    if (result.ec != std::errc{} || result.ptr != end)
        return false;

    if (value < 1 || value > 65535)
        return false;

    outPort = static_cast<unsigned short>(value);
    return true;
}


int main(int argc, char* argv[])
{
    std::cout << std::unitbuf;
    std::cout << "Server Started\n";  
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <tcp_port> <udp_port>" << std::endl;
        return 1;
    }

    unsigned short tcpPort{};
    unsigned short udpPort{};

    if (!ParsePort(argv[1], tcpPort))
    {
        std::cerr << "Invalid TCP port: " << argv[1] << std::endl;
        return 1;
    }
    if (!ParsePort(argv[2], udpPort))
    {
        std::cerr << "Invalid UDP port: " << argv[2] << std::endl;
        return 1;
    }

    try
    {   
        std::cout << "Ports Read\n";  
        boost::asio::io_context io;
        std::shared_ptr<std::unordered_map<int, GameSession>> game_session{std::make_shared<std::unordered_map<int, GameSession>>()};

        UdpServer udpServer(io, game_session, udpPort);
        TcpServer tcpServer(io, game_session, tcpPort);

        io.run();        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}