#include "UdpServer.hpp"

UdpServer::UdpServer(
        boost::asio::io_context& io,
        std::shared_ptr<std::unordered_map<int, GameSession>>& game_sessions,
        int port
):
    m_socket(io, udp::endpoint(udp::v4(), port)),
    m_loop_inspect_timer(io, boost::asio::chrono::seconds(inspect_loop_time_s)),
    m_loop_forward_timer(io, boost::asio::chrono::milliseconds(forward_loop_time_ms))
{
    m_game_sessions = game_sessions;
    StartRecieve();
    m_loop_inspect_timer.async_wait(std::bind(&UdpServer::InspectSessionCallback, this));
    m_loop_forward_timer.async_wait(std::bind(&UdpServer::ForwardLoopCallback, this));
}

void UdpServer::InspectSessionCallback()
{
    //std::cout << "UdpServer::LoopAllGameSessions Begin\n";
    
    for (auto it = m_game_sessions->begin(); it != m_game_sessions->end();)
    {
        auto& [session_id, session] = *it;

        if (session.ShouldDestroy())
        {
            it = m_game_sessions->erase(it);
        }
        else
        {
            ++it;
        }
    }
    
    m_loop_inspect_timer.expires_at(m_loop_inspect_timer.expiry() + boost::asio::chrono::seconds(inspect_loop_time_s));

    m_loop_inspect_timer.async_wait(std::bind(&UdpServer::InspectSessionCallback, this));
    //std::cout << "UdpServer::LoopAllGameSessions End\n";
}

void UdpServer::ForwardLoopCallback()
{
    for (auto& [session_id, session] : *m_game_sessions)
    {
        session.ForwardLastMessages(m_socket);
    }
    m_loop_forward_timer.expires_at(m_loop_forward_timer.expiry() + boost::asio::chrono::milliseconds(forward_loop_time_ms));
    m_loop_forward_timer.async_wait(std::bind(&UdpServer::ForwardLoopCallback, this));
}

void UdpServer::StartRecieve()
{
    std::shared_ptr<udp::endpoint> remote_endpoint_ptr{ std::make_shared<udp::endpoint>() };
    m_socket.async_receive_from(
        boost::asio::buffer(recv_buf),
        *remote_endpoint_ptr,
        std::bind(
            &UdpServer::HandleRecieve, this, remote_endpoint_ptr, boost::asio::placeholders::bytes_transferred
        )
    );
}

void UdpServer::HandleRecieve(std::shared_ptr<udp::endpoint> remote_endpoint_ptr, std::size_t len)
{
    std::memcpy(work_buf.data(), recv_buf.data(), len);
    work_len = static_cast<int>(len);
    work_index = 0;
    try
    {
        InterpretMessage(remote_endpoint_ptr);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    StartRecieve();
}

void UdpServer::InterpretMessage(std::shared_ptr<udp::endpoint> remote_endpoint_ptr)
{
    //std::cout << "InterpretMessage: " << work_buf.data() << '\n';
    if (work_len < 2)
    {
        return;
    }
    
    if (recv_buf[0] == contract(Action::MatchEvent) && recv_buf[1] == contract(Action::Deliminator))
    {
        size_t next_start{2};
        GameEventData ge{work_buf, work_len};
        auto it = m_game_sessions->find(ge.m_match_id);
        if (it != m_game_sessions->end())
        {
            it->second.ProcessEvent(ge, m_socket);
            return;
        }
        ErrorData e{"Error: Match Id Not Found"};
        std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>(
            e.EncodeBuffer()
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

    if (work_buf[0] == contract(Action::Join) && work_buf[1] == contract(Action::Deliminator))
    {
        size_t next_start{2};
        int player_type = ParseIntegerTillDeliminator(work_buf, next_start, work_len, contract(Action::Deliminator));
        int session_id = ParseIntegerTillDeliminator(work_buf, next_start, work_len, contract(Action::EndDeliminator));
        auto it = m_game_sessions->find(session_id);
        if (it == m_game_sessions->end())
        {
            ErrorData e{"Error: Match Id Not Found"};
            std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>(
                e.EncodeBuffer()
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
            it->second.AddParticipant(remote_endpoint_ptr, static_cast<GameEventData::ObjectType>(player_type))
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

    if (work_buf[0] == contract(Action::Leave) && work_buf[1] == contract(Action::Deliminator))
    {
        size_t next_start{2};
        int player_type = ParseIntegerTillDeliminator(work_buf, next_start, work_len, contract(Action::Deliminator));
        int session_id = ParseIntegerTillDeliminator(work_buf, next_start, work_len, contract(Action::EndDeliminator));
        auto it = m_game_sessions->find(session_id);
        if (it == m_game_sessions->end())
        {
            ErrorData e{"Error: Match Id Not Found"};
            std::shared_ptr<std::string> message_ptr { std::make_shared<std::string>(
                e.EncodeBuffer()
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
        it->second.RemoveParticipant(static_cast<GameEventData::ObjectType>(player_type));
        
        return;
    }
}
