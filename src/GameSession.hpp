#pragma once
#include <boost/asio.hpp>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include "ClientServerContract.hpp"
#include <chrono>

//#include "GameMessage.hpp"

using boost::asio::ip::udp;

class GameSession
{
public:
    explicit GameSession(const int new_id, MatchNameBuf& match_name_buf, size_t match_name_len):
        m_data(new_id, match_name_buf, match_name_len)
    {
        m_last_red_event.m_player_type = GameEventData::ObjectType::Red;
        m_last_green_event.m_player_type = GameEventData::ObjectType::Green;

        m_last_red_event.m_match_id = new_id;
        m_last_green_event.m_match_id = new_id;
        m_last_ball_event.m_match_id = new_id;

        m_last_red_event.m_join_level = m_global_join_level;
        m_last_green_event.m_join_level = m_global_join_level;
        m_last_ball_event.m_join_level = m_global_join_level;

        last_red_time_point = std::chrono::steady_clock::now();
        last_green_time_point = std::chrono::steady_clock::now();
    }

    std::string AddParticipant(std::shared_ptr<udp::endpoint> participant_endpoint_ptr, GameEventData::ObjectType type);
    void RemoveParticipant(GameEventData::ObjectType type);
    void ProcessEvent(GameEventData& e, boost::asio::ip::udp::socket& socket);
    void ForwardLastMessages(boost::asio::ip::udp::socket& socket);
    bool ShouldDestroy();

    GameSessionData m_data;
private:
    GameEventData m_last_red_event{};
    GameEventData m_last_green_event{};
    GameEventData m_last_ball_event{};
    std::shared_ptr<udp::endpoint> red_endpoint_ptr = nullptr;
    std::shared_ptr<udp::endpoint> green_endpoint_ptr = nullptr;

    bool first_added;
    std::chrono::steady_clock::time_point last_red_time_point;
    std::chrono::steady_clock::time_point last_green_time_point;
    int m_global_join_level{0};

    void HandleSend(std::shared_ptr<std::string> message_ptr);
    int m_time_padding_red_ms{0};
    int m_time_padding_green_ms{0};
};
