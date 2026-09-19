#pragma once

#include <array>  
#include <string_view>
#include <charconv>
#include <string>
#include <format>

constexpr size_t conn_buf_size{128};
constexpr size_t match_name_buf_size{32};
//constexpr int server_udp_port{13};
//constexpr int server_tcp_port{14};

using MatchNameBuf  = std::array<char, match_name_buf_size>;
using ConnectionBuf = std::array<char, conn_buf_size>;

enum class Action { Connect, Join, Create, Leave, ListSessions, MatchId, MatchEvent, Deliminator, EndDeliminator, Success, Error, Count }; // Count = number of values ()  
 
// constexpr array of (enum, string) pairs  
constexpr std::array<std::pair<Action, char>, static_cast<size_t>(Action::Count)> client_server_contract {{
    {Action::Connect,       'C'},
    {Action::Create,        'K'}, 
    {Action::Join,          'J'},  
    {Action::Leave,         'L'},
    {Action::ListSessions,  'S'},  
    {Action::MatchId,       'M'},  
    {Action::MatchEvent,    'E'},
    {Action::Deliminator,   '|'},
    {Action::EndDeliminator,'$'},
    {Action::Success,       'Z'},
    {Action::Error,         '#'},
}};  
 
// constexpr function to convert enum to string (C++14+)  
constexpr char contract(Action c) {  
    for (const auto& pair : client_server_contract) {  
        if (pair.first == c) {  
            return pair.second;  
        }  
    }  
    return '#'; // Handle invalid values  
}

class ParseException : public std::runtime_error
{
public:
    explicit ParseException(const std::string& message)
        : std::runtime_error(message)
    {}
};

inline int ParseIntegerKnownRange(
    const ConnectionBuf& buf,
    size_t start,
    size_t range_size,
    std::size_t len
)
{
    if (start + range_size > len)
        throw ParseException(std::format(
            "ParseIntegerKnownRange: range [{}, {}) exceeds buffer length {}",
            start, start + range_size, len
        ));

    int id = 0;
    auto result = std::from_chars(buf.data() + start, buf.data() + start + range_size, id);

    if (result.ec != std::errc())
        throw ParseException(std::format(
            "ParseIntegerKnownRange: failed to parse integer at [{}, {}): {}",
            start, start + range_size,
            result.ec == std::errc::invalid_argument ? "not a number" : "value out of int range"
        ));
    
    if(result.ptr != buf.data() + start + range_size)
        throw ParseException(std::format(
            "ParseIntegerKnownRange: trailing garbage after integer, parsed {} of {} chars",
            result.ptr - (buf.data() + start), range_size
        ));

    return id;
}

inline int ParseIntegerTillDeliminator(
    const ConnectionBuf& buf,
    size_t& start_next,
    std::size_t len,
    char deliminator
)
{
    size_t digits_end{start_next};
    bool deliminator_found{false};
    while (digits_end < len)
    {
        if (buf[digits_end] == deliminator)
        {
            deliminator_found = true;
            if(start_next == digits_end)
                throw ParseException("ParseIntegerTillDeliminator: empty field");
            break;
        }
        ++digits_end;
    }
    
    if (!deliminator_found)
        throw ParseException("ParseIntegerTillDeliminator: deliminator not found");

    int id = 0;
    auto result = std::from_chars(buf.data() + start_next, buf.data() + digits_end, id);

    if (result.ec != std::errc() || result.ptr != buf.data() + digits_end)
        throw ParseException(std::format(
            "ParseIntegerTillDeliminator: failed to parse integer at [{}, {})",
            start_next, digits_end
        ));

    start_next = digits_end + 1;
    return id;
}

inline int ParseMatchNameTillDeliminator(
    const ConnectionBuf& buf,
    size_t& start_next,
    std::size_t len,
    MatchNameBuf& match_name_buf,
    char deliminator
)
{
    std::size_t field_end{start_next};
    bool deliminator_found{false};
    while (field_end < len)
    {
        if (buf[field_end] == deliminator)
        {
            deliminator_found = true;
            if(start_next == field_end)
                throw ParseException("ParseMatchNameTillDeliminator: empty field");
            break;
        }
        ++field_end;
    }

    if (!deliminator_found)
        throw ParseException("ParseMatchNameTillDeliminator: deliminator not found");

    std::size_t field_len{field_end - start_next};

    // -1 to leave room for the null terminator below
    if (field_len >= match_name_buf_size)
        throw ParseException(std::format(
            "ParseMatchNameTillDeliminator: name length {} exceeds buffer size {}",
            field_len, match_name_buf_size
        ));

    std::memcpy(match_name_buf.data(), buf.data() + start_next, field_len);
    match_name_buf[field_len] = '\0';

    start_next = field_end + 1;

    return static_cast<int>(field_len);
}

inline int ParseConnectionMessageTillDeliminator(
    const ConnectionBuf& buf,
    size_t& start_next,
    std::size_t len,
    ConnectionBuf& dst_connectionBuf_buf,
    char deliminator
)
{
    std::size_t field_end{start_next};
    bool deliminator_found{false};
    while (field_end < len)
    {
        if (buf[field_end] == deliminator)
        {
            deliminator_found = true;
            if(start_next == field_end)
                throw ParseException("ParseConnectionMessageTillDeliminator: empty field");
            break;
        }
        ++field_end;
    }

    if (!deliminator_found)
        throw ParseException("ParseConnectionMessageTillDeliminator: deliminator not found");

    std::size_t field_len{field_end - start_next};

    // -1 to leave room for the null terminator below
    if (field_len >= conn_buf_size)
        throw ParseException(std::format(
            "ParseConnectionMessageTillDeliminator: field length {} exceeds buffer size {}",
            field_len, conn_buf_size
        ));

    std::memcpy(dst_connectionBuf_buf.data(), buf.data() + start_next, field_len);
    dst_connectionBuf_buf[field_len] = '\0';

    start_next = field_end + 1;

    return static_cast<int>(field_len);
}

class GameSessionData
{
public:
    enum ConnectionStatus : int
    {
        Disconnected = 0,
        Connected = 1
    };

    size_t m_match_name_len;
    MatchNameBuf m_match_name_buf;
    int match_id{0};
    int red_connection_status{ConnectionStatus::Disconnected};
    int green_connection_status{ConnectionStatus::Disconnected};
    int red_score{0};
    int green_score{0};

    GameSessionData(const int new_id, const std::array<char, match_name_buf_size>& match_name_buf, size_t match_name_len) :
        match_id(new_id), m_match_name_buf(match_name_buf), m_match_name_len(match_name_len)
    {
    }
    GameSessionData(const ConnectionBuf &buf, size_t buf_len)
    {
        DecodeBuffer(buf, buf_len);
    }
    GameSessionData(){}

    int PlayerCount(){
        int count{0};
        if(red_connection_status == ConnectionStatus::Connected)
            count++;
        if(green_connection_status == ConnectionStatus::Connected)
            count++;
        return count;
    }
    const char* MatchName(){ return m_match_name_buf.data(); }
    int MatchId(){ return match_id; }
    int RedScore(){ return red_score; }
    int GreenScore(){ return green_score; }
    void AddRedScore(){ red_score++; }
    void AddGreenScore(){ green_score++; }
    void Reset() { red_score = 0; green_score = 0; }

    std::string EncodeBuffer() const
    {
        return std::format(
            "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
            contract(Action::Success),      contract(Action::Deliminator),
            contract(Action::ListSessions), contract(Action::Deliminator),
            match_id,                       contract(Action::Deliminator),
            m_match_name_buf.data(),        contract(Action::Deliminator),
            red_connection_status,          contract(Action::Deliminator),
            green_connection_status,        contract(Action::Deliminator),
            red_score,                      contract(Action::Deliminator),
            green_score,                    contract(Action::EndDeliminator)
        );
    }
    void DecodeBuffer(const ConnectionBuf &buf, size_t buf_len)
    {
        size_t next_start{4};
        match_id = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_match_name_len = ParseMatchNameTillDeliminator(buf, next_start, buf_len, m_match_name_buf, contract(Action::Deliminator));
        red_connection_status = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        green_connection_status = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        red_score = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        green_score = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::EndDeliminator));
    }
};

class GameEventData
{
public:
    static constexpr float factor{1000.f};
    enum ObjectType : int
    {
        Red = 0,
        Green = 1,
        Ball = 2
    };
    int m_match_id{0};
    int m_player_type{0};
    int m_red_score{0};
    int m_green_score{0};
    int m_player_pos_x{0};
    int m_player_pos_z{0};
    int m_player_vel_x{0};
    int m_player_vel_z{0};
    int m_join_level{0};    // number of join reqs
    int m_time_stamp_now_ms{0};
    int m_lag_ms{0};

    GameEventData() {}

    GameEventData(const ConnectionBuf& buf, size_t buf_len)
    {
        DecodeBuffer(buf, buf_len);
    }

    GameEventData(
        const int& match_id,
        const int& player_type,
        const int& red_score,
        const int& green_score,
        int player_pos_x,
        int player_pos_z,
        int player_vel_x,
        int player_vel_z,
        int time_stamp_now_ms,
        int lag_ms
    ):
        m_match_id{match_id},
        m_player_type{player_type},
        m_red_score{red_score},
        m_green_score{green_score},
        m_player_pos_x{player_pos_x},
        m_player_pos_z{player_pos_z},
        m_player_vel_x{player_vel_x},
        m_player_vel_z{player_vel_z},
        m_time_stamp_now_ms{time_stamp_now_ms},
        m_lag_ms{lag_ms}
    {
    }

    std::string EncodeBuffer() const
    {
        return std::format(
            "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
            contract(Action::MatchEvent),contract(Action::Deliminator),
            m_match_id,                  contract(Action::Deliminator),
            m_player_type,               contract(Action::Deliminator),
            m_red_score,                 contract(Action::Deliminator),
            m_green_score,               contract(Action::Deliminator),
            m_player_pos_x,              contract(Action::Deliminator),
            m_player_pos_z,              contract(Action::Deliminator),
            m_player_vel_x,              contract(Action::Deliminator),
            m_player_vel_z,              contract(Action::Deliminator),
            m_join_level,                contract(Action::Deliminator),
            m_time_stamp_now_ms,         contract(Action::Deliminator),
            m_lag_ms,                    contract(Action::EndDeliminator)
        );
    }

    void DecodeBuffer(const ConnectionBuf& buf, size_t buf_len)
    {
        size_t next_start{2};
        m_match_id       = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_player_type    = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_red_score      = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_green_score    = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_player_pos_x   = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_player_pos_z   = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_player_vel_x   = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_player_vel_z   = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_join_level     = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_time_stamp_now_ms = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::Deliminator));
        m_lag_ms         = ParseIntegerTillDeliminator(buf, next_start, buf_len, contract(Action::EndDeliminator));
    }
};

inline bool AreTwins(const GameEventData& a, const GameEventData& b)
{
    return a.m_time_stamp_now_ms == b.m_time_stamp_now_ms;
}

class ErrorData
{
private:
    size_t m_error_message_len{0};
    ConnectionBuf m_error_message_buf;

public:
    ErrorData() {}

    ErrorData(const std::string& error_message)
    {
        m_error_message_len = std::min(error_message.size(), m_error_message_buf.size());
        std::copy_n(error_message.data(), m_error_message_len, m_error_message_buf.data());
    }

    ErrorData(const ConnectionBuf& buf, size_t buf_len)
    {
        DecodeBuffer(buf, buf_len);
    }

    std::string EncodeBuffer() const
    {
        return std::format(
            "{}{}{}{}{}",
            contract(Action::Error), contract(Action::Deliminator),
            std::string_view(m_error_message_buf.data(), m_error_message_len),
            contract(Action::EndDeliminator), ""
        );
    }

    void DecodeBuffer(const ConnectionBuf& buf, size_t buf_len)
    {
        size_t next_start{2};
        m_error_message_len = ParseConnectionMessageTillDeliminator(
            buf, next_start, buf_len, m_error_message_buf, contract(Action::EndDeliminator)
        );
    }
};