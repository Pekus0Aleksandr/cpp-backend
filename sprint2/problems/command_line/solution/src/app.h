#pragma once
#include "sdk.h"
#include <boost/json.hpp>
#include <boost/asio/strand.hpp>
#include <string>
#include <random>
#include "model.h"
#include "token.h"
#include "ticker.h"

namespace app {

    namespace js = boost::json;
    namespace net = boost::asio;
    namespace sys = boost::system;

    enum class JoinError {
        None,
        BadJson,
        MapNotFound,
        InvalidName
    };

    enum class error_code {
        None,
        InvalidToken,
        UnknownToken,
        InvalidArgument
    };

    class ModelToJson {
    public:
        explicit ModelToJson(model::Game& game) : game_{ game } {}

        std::string GetMaps() const;
        std::string GetMap(std::string_view nameMap) const;
    private:
        model::Game& game_;
        static js::array GetRoads(const model::Map::Roads& roads);
        static js::array GetBuildings(const model::Map::Buildings& buildings);
        static js::array GetOffice(const model::Map::Offices& offices);
    };

    std::string JsonMessage(std::string_view code, std::string_view message);
    std::string ToHex(uint64_t n);

    class Player {
    public:
        using Id = util::Tagged<uint64_t, Player>;
        Player(model::GameSession* session, model::Dog* dog) : session_(session), dog_(dog), id_({idn++}) {}

        const Id& GetId() const {
            return id_;
        }
        const model::Map::Id& MapId() const {
            return session_->MapId();
        }
        std::string_view GetName() const {
            return dog_->GetName();
        }
        const model::GameSession* GetSession() {
            return session_;
        }
        void Move(std::string_view move_cmd);

    private:
        static std::atomic<uint64_t> idn;
        Id id_;
        model::GameSession* session_;
        model::Dog* dog_;
    };

    class PlayerTokens {
    public:
        Player* FindPlayer(Token token) const;
        Token AddPlayer(Player* player);

    private:
        std::unordered_map<Token, Player*, util::TaggedHasher<Token>> token_to_player;
        std::random_device random_device_;
        std::mt19937_64 generator1_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
        std::mt19937_64 generator2_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
        std::string GetToken() {
            std::string r1 = ToHex(generator1_());
            std::string r2 = ToHex(generator2_());
            return r1 + r2;
        }
    };

    class Players {
    public:
        Player* Add(model::Dog *dog, model::GameSession *session);
        Player* FindPlayer(Player::Id player_id, model::Map::Id map_id) noexcept;
        const Player::Id* FindPlayerId(std::string_view player_name) const noexcept;
        Player* FindPlayer(Player::Id player_id) const noexcept;
    
    private:
        using PlayerIdHasher = util::TaggedHasher<Player::Id>;
        using PlayerIdToIndex = std::unordered_map<Player::Id, size_t, PlayerIdHasher>;
        std::deque<std::unique_ptr<Player>> players_;
        PlayerIdToIndex player_id_to_index_;
    };

    class App {
    public:
        explicit App(model::Game& game) : game_{ game } {}

        std::pair<std::string, bool> GetMapBodyJson(std::string_view requestTarget) const;
        std::pair<std::string, JoinError> ResponseJoin(std::string_view jsonBody);
        std::pair<std::string, error_code> ActionMove(const Token& token, std::string_view jsonBody);
        std::pair<std::string, error_code> Tick(std::string_view jsonBody);
        std::pair<std::string, error_code> GetPlayers(const Token& token) const;
        std::pair<std::string, error_code> GetState(const Token& token) const;
        std::pair<std::string, error_code> CheckToken(const Token& token) const;
    
    private:
        model::Game& game_;
        Players players_;
        PlayerTokens player_tokens_;
        Player* GetPlayer(const Token& token) const;
        Player* GetPlayer(std::string_view nick, std::string_view mapId);
    };

} //namespace app

