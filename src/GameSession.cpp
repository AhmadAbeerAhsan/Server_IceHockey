#include "GameSession.hpp"

std::string GameSession::AddParticipant(std::shared_ptr<udp::endpoint> participant_endpoint_ptr, GameEventData::ObjectType type)
{
    ErrorData e{"Error: Could not Join Game"};
    std::string response{e.EncodeBuffer()};
    if (m_data.PlayerCount() >= 2)
    {
        return response;
    }
    
    if (red_endpoint_ptr == nullptr && type == GameEventData::ObjectType::Red)
    {
        red_endpoint_ptr = participant_endpoint_ptr;
        last_red_time_point = std::chrono::steady_clock::now();
        m_data.red_connection_status = GameSessionData::ConnectionStatus::Connected;
        m_last_red_event.m_time_stamp_now_ms = 0;
        response = m_last_red_event.EncodeBuffer();
    }
    else if (green_endpoint_ptr == nullptr && type == GameEventData::ObjectType::Green)
    {
        last_green_time_point = std::chrono::steady_clock::now();
        green_endpoint_ptr = participant_endpoint_ptr;
        m_data.green_connection_status = GameSessionData::ConnectionStatus::Connected;
        m_last_green_event.m_time_stamp_now_ms = 0;
        response = m_last_green_event.EncodeBuffer();
    }
    m_global_join_level++;

    return response;
}

void GameSession::RemoveParticipant(GameEventData::ObjectType type)
{
    if (type == GameEventData::ObjectType::Red)
    {
        red_endpoint_ptr.reset();
        red_endpoint_ptr = nullptr;
        m_data.red_connection_status = GameSessionData::ConnectionStatus::Disconnected;
    }
    else if (type == GameEventData::ObjectType::Green)
    {
        green_endpoint_ptr.reset();
        green_endpoint_ptr = nullptr;
        m_data.green_connection_status = GameSessionData::ConnectionStatus::Disconnected;
    }
}

void GameSession::ProcessEvent(GameEventData& e, boost::asio::ip::udp::socket& socket)
{
    e.m_join_level = m_global_join_level;
    
    if (e.m_player_type == GameEventData::ObjectType::Red && e.m_time_stamp_now_ms > m_last_red_event.m_time_stamp_now_ms)
    {
        m_last_red_event = e;
        last_red_time_point = std::chrono::steady_clock::now();
    }
    else if (e.m_player_type == GameEventData::ObjectType::Green && e.m_time_stamp_now_ms > m_last_green_event.m_time_stamp_now_ms)
    {
        m_last_green_event = e;
        last_green_time_point = std::chrono::steady_clock::now();
    }
    else if (e.m_player_type == GameEventData::ObjectType::Ball && e.m_time_stamp_now_ms > m_last_ball_event.m_time_stamp_now_ms)
    {
        m_last_ball_event = e;
    }
    
    std::shared_ptr<std::string> res{std::make_shared<std::string>(e.EncodeBuffer())};

    if (green_endpoint_ptr != nullptr)
        socket.async_send_to(
            boost::asio::buffer(*res),
            *green_endpoint_ptr,
            std::bind(&GameSession::HandleSend, this, res)
        );

    if (red_endpoint_ptr != nullptr)
        socket.async_send_to(
            boost::asio::buffer(*res),
            *red_endpoint_ptr,
            std::bind(&GameSession::HandleSend, this, res)
        );
}

void GameSession::ForwardLastMessages(boost::asio::ip::udp::socket &socket)
{
    std::shared_ptr<std::string> red_e{std::make_shared<std::string>(m_last_red_event.EncodeBuffer())};
    std::shared_ptr<std::string> green_e{std::make_shared<std::string>(m_last_green_event.EncodeBuffer())};
    std::shared_ptr<std::string> ball_e{std::make_shared<std::string>(m_last_ball_event.EncodeBuffer())};
    if (green_endpoint_ptr != nullptr)
    {
        socket.async_send_to(
            boost::asio::buffer(*red_e),
            *green_endpoint_ptr,
            std::bind(&GameSession::HandleSend, this, red_e)
        );
        socket.async_send_to(
            boost::asio::buffer(*ball_e),
            *green_endpoint_ptr,
            std::bind(&GameSession::HandleSend, this, ball_e)
        );
    }

    if (red_endpoint_ptr != nullptr)
    {
        socket.async_send_to(
            boost::asio::buffer(*green_e),
            *red_endpoint_ptr,
            std::bind(&GameSession::HandleSend, this, green_e)
        );
        socket.async_send_to(
            boost::asio::buffer(*ball_e),
            *red_endpoint_ptr,
            std::bind(&GameSession::HandleSend, this, ball_e)
        );
    }
}

bool GameSession::ShouldDestroy()
{
     int dirty{0};
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    
    int time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_red_time_point).count();
    if (time_elapsed > 30000)
    {
        red_endpoint_ptr.reset();
        red_endpoint_ptr = nullptr;
        m_data.red_connection_status = GameSessionData::ConnectionStatus::Disconnected;
        dirty++;
    }
    
    time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_green_time_point).count();
    if (time_elapsed > 30000)
    {
        green_endpoint_ptr.reset();
        green_endpoint_ptr = nullptr;
        m_data.green_connection_status = GameSessionData::ConnectionStatus::Disconnected;
        dirty++;
    }
    if (dirty == 2)
    {
        return true;
    }
    return false;
}

void GameSession::HandleSend(std::shared_ptr<std::string> message_ptr)
{
}
