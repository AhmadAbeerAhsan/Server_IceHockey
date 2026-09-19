#pragma once

#include "GameSession.hpp"
#include <format>
#include "TcpConnection.hpp"

class TcpServer
{
private:
    std::shared_ptr<std::unordered_map<int, GameSession>> m_game_sessions;
    int m_possible_new_session_id{0};
public:
    TcpServer(
        boost::asio::io_context& io,
        std::shared_ptr<std::unordered_map<int, GameSession>>& game_sessions,
        int port
    ):
        m_io{io},
        m_acceptor{io, tcp::endpoint(tcp::v4(), port)}
    {
        m_game_sessions = game_sessions;
        StartAccept();
    }

    ~TcpServer()
    {
    }
private:
    void StartAccept()
    {
        std::shared_ptr<TcpConnection> newConPointer = TcpConnection::Create(m_io, m_game_sessions, m_possible_new_session_id);
        m_possible_new_session_id++;
        m_acceptor.async_accept(
            newConPointer->GetSocket(),
            std::bind(
                &TcpServer::HandleAccept,
                this,
                newConPointer,
                boost::asio::placeholders::error
            )
        );

    }

    void HandleAccept(std::shared_ptr<TcpConnection> newConPointer, const boost::system::error_code& error)
    {
        if (!error)
        {
            newConPointer->Start();
        }
        StartAccept();
    }

    boost::asio::io_context& m_io;
    tcp::acceptor m_acceptor;
};