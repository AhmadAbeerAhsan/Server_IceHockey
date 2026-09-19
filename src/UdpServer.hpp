#pragma once

#include "GameSession.hpp"
#include "ClientServerContract.hpp"
#include <format>

constexpr int inspect_loop_time_s{5};
constexpr int forward_loop_time_ms{300};

class UdpServer
{
public:
    UdpServer(
        boost::asio::io_context& io,
        std::shared_ptr<std::unordered_map<int, GameSession>>& game_sessions,
        int port
    );
private:
    udp::socket m_socket;
    boost::asio::steady_timer m_loop_inspect_timer;
    boost::asio::steady_timer m_loop_forward_timer;
    std::array<char,conn_buf_size> recv_buf;
    std::array<char,conn_buf_size> work_buf;
    size_t work_len;
    size_t work_index;

    std::shared_ptr<std::unordered_map<int, GameSession>> m_game_sessions;
    void StartRecieve();

    void HandleRecieve(std::shared_ptr<udp::endpoint> remote_endpoint_ptr, std::size_t len);

    void HandleSend(std::shared_ptr<std::string> message_ptr) {}

    void InterpretMessage(std::shared_ptr<udp::endpoint> remote_endpoint_ptr);

    void InspectSessionCallback();
    void ForwardLoopCallback();
};
