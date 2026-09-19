#pragma once

#include <boost/asio.hpp>
#include "ClientServerContract.hpp"
#include "GameSession.hpp"
#include <memory>
#include <iostream>
#include <format>

using boost::asio::ip::tcp;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    static std::shared_ptr<TcpConnection> Create(boost::asio::io_context& io, std::shared_ptr<std::unordered_map<int, GameSession>>& game_sessions, int possible_new_session_id)
    {
        return std::shared_ptr<TcpConnection>(new TcpConnection(io, game_sessions, possible_new_session_id));
    }

    tcp::socket& GetSocket()
    {
        return m_socket;
    }

    void Start()
    {
        boost::system::error_code ignored_error;
        boost::asio::async_read_until(
            m_socket,
            m_recv_streambuf,
            contract(Action::EndDeliminator),
            std::bind(
                &TcpConnection::HandleRecieve,
                shared_from_this(),
                boost::asio::placeholders::bytes_transferred
            )
        );
        //std::cout << "Message Read From Client" << std::endl;
    }
private:
    TcpConnection(boost::asio::io_context& io, std::shared_ptr<std::unordered_map<int, GameSession>>& game_sessions, int possible_new_session_id):
        m_socket{io}, m_possible_new_session_id(possible_new_session_id)
    {
        m_game_sessions = game_sessions;
    }

    std::shared_ptr<std::unordered_map<int, GameSession>> m_game_sessions;
    tcp::socket m_socket;
    boost::asio::streambuf m_recv_streambuf;
    std::array<char,conn_buf_size> work_buf;
    size_t work_len;
    int m_possible_new_session_id;

    void HandleRecieve(std::size_t len)
    {
        if (len >= conn_buf_size)
        {
            //return error
            return;
        }
        
        std::istream stream(&m_recv_streambuf);
        stream.read(work_buf.data(), static_cast<std::streamsize>(len));
        work_len = static_cast<int>(len);

        try
        {
            InterpretMessage();
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    void InterpretMessage()
    {
        //std::cout << "Message: " << work_buf.data() << '\n';
        if (work_len < 2)
        {
            return;
        }
        
        if (work_buf[0] == contract(Action::Connect) && work_buf[1] == contract(Action::Deliminator))
        {
            //std::cout << "Connect called" << '\n';
            std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>("Connected To Server...")};
            m_socket.async_send(
                boost::asio::buffer(*message_ptr),
                std::bind(
                    &TcpConnection::HandleSend,
                    shared_from_this(),
                    message_ptr
                )
            );
            return;
        }

        if (work_buf[0] == contract(Action::ListSessions) && work_buf[1] == contract(Action::EndDeliminator))
        {
            //std::cout << "ListSessions called" << '\n';
            for (const auto& [session_id, session] : *m_game_sessions)
            {
                std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>(session.m_data.EncodeBuffer())};
                m_socket.async_send(
                    boost::asio::buffer(*message_ptr),
                    std::bind(
                        &TcpConnection::HandleSend,
                        shared_from_this(),
                        message_ptr
                    )
                );
            }
            return;
        }

        if (work_buf[0] == contract(Action::Create) && work_buf[1] == contract(Action::Deliminator))
        {
            //std::cout << "Create called" << '\n';
            std::array<char, match_name_buf_size> match_name_buf;
            std::shared_ptr<std::string> response_message_ptr;
            size_t next_start{2};
            int match_len = ParseMatchNameTillDeliminator(work_buf, next_start, work_len, match_name_buf, contract(Action::EndDeliminator));
            if(match_len > 0)
            {
                auto result = m_game_sessions->try_emplace(m_possible_new_session_id, m_possible_new_session_id, match_name_buf, (size_t)match_len);
                auto it = result.first;
                response_message_ptr = std::make_shared<std::string>(
                    it->second.m_data.EncodeBuffer()
                );
            }
            else
            {
                response_message_ptr = std::make_shared<std::string>(
                    std::format("{}{}",contract(Action::Error),contract(Action::EndDeliminator))
                );
            }
            // std::cout << std::format(
            //     "{} {}\n", "Out Message:", *response_message_ptr
            // ); 
            m_socket.async_send(
                boost::asio::buffer(*response_message_ptr),
                std::bind(
                    &TcpConnection::HandleSend,
                    shared_from_this(),
                    response_message_ptr
                )
            );
 
            return;
        }
    }

    void HandleSend(std::shared_ptr<std::string> message_ptr)
    {

    }
};