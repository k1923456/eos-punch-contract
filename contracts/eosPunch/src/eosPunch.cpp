#include <eosPunch.hpp>

using namespace eosio;

class[[eosio::contract]] eosPunch : public eosio::contract
{

  public:
    using contract::contract;

    eosPunch(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds){}

    [[eosio::action]]
    void cleartables()
    {
        require_auth(get_self());

        userTable players(_self, _self.value); // code, scope
        stateTable state_table(_self, _self.value);
        for (auto itr = players.begin(); itr != players.end();)
        {
            itr = players.erase(itr);
        }
        for (auto itr = state_table.begin(); itr != state_table.end();)
        {
            itr = state_table.erase(itr);
        }
    }

    void transferAction(name from,
                        name to,
                        asset quantity,
                        std::string memo)
    {
        if (to == _self && memo.size() == 5)
        {
            uint64_t i = 0;
            uint64_t tmp = 0;
            std::vector<uint64_t> punches;

            for (i = 0; i < memo.size(); i++)
            {
                tmp = getInt(memo[i]);
                if (tmp >= 1 && tmp <= 3)
                {
                    punches.push_back(tmp);
                }
                else
                {
                    print("Fuck you hakers!!");
                }
            }

            eosio_assert(punches.size() == MAX_PUNCHES, "Numbers of punches don't match!");
            this->punch(from, quantity / MAX_PUNCHES, punches);
        }
        else
        {
            print("Nothing happened but transfering.");
        }
    }

  private:
    const name GAME1 = "version1"_n;
    const std::string BANKER_MSG = "banker_pay";
    const uint64_t MAX_PUNCHES = 5;
    const uint64_t PUNCH_TYPE = 3;
    const double DRAW = 0.9;
    const double WIN = 2.0;
    const double LOSE = -1.0;
    const int64_t WIN_STATE = 1;
    const int64_t LOSE_STATE = -1;
    const int64_t DRAW_STATE = 0;
    const uint64_t MAX_VALUE = 50000;
    const uint64_t MIN_VALUE = 1000;
    uint32_t timeSeed = now();

    [[eosio::action]] void
    punch(name user, asset bet, std::vector<uint64_t> punches) {

        const symbol EOSSymbol(symbol_code("EOS"), 4);
        const auto my_balance = eosio::token::get_balance("eosio.token"_n, get_self(), EOSSymbol.code());

        eosio_assert(_code == "eosio.token"_n, "Fuck you hakers 2.");
        eosio_assert(bet.symbol.code() == EOSSymbol.code(), "The token MUST BE EOS!!");
        // eosio_assert(bet.amount >= -eosio::asset::max_amount, "input bet underflow");
        // eosio_assert(bet.amount <= eosio::asset::max_amount, "input bet overflow");
        eosio_assert(bet.amount <= MAX_VALUE, "The max bet value is 5 EOS");
        eosio_assert(bet.amount >= MIN_VALUE, "The max bet value is 0.1 EOS");
        // eosio_assert(bet.amount <= my_balance.amount, "Banker has been broken");

        int64_t playerValue = 0;
        int64_t playerTotalValue = 0;
        int64_t state = DRAW_STATE;

        uint64_t i = 0;
        uint64_t bankerValue = 0;
        uint64_t jackpotValue = 0;
        uint64_t playerPunch = 0;
        uint64_t bankerPunch = 0;

        bool jackpot = true;
        std::vector<struct punchRecord> round;

        for (i = 0; i < punches.size(); i++)
        {
            playerPunch = punches[i];
            bankerPunch = random();

            if (playerPunch != 2 && (playerPunch + bankerPunch) == 4)
            {
                if (playerPunch == 1)
                {
                    playerValue = bet.amount * WIN;
                    state = WIN_STATE;
                }
                else
                {
                    playerValue = bet.amount * LOSE;
                    state = LOSE_STATE;
                    jackpot = false;
                }
            }
            else
            {
                if (playerPunch == bankerPunch)
                {
                    playerValue = bet.amount * DRAW;
                    jackpotValue = (bet.amount - playerValue) / 2;
                    updateJackpot(GAME1, DRAW_STATE, jackpotValue);
                    state = DRAW_STATE;
                    jackpot = false;
                }
                else if (playerPunch > bankerPunch)
                {
                    playerValue = bet.amount * WIN;
                    state = WIN_STATE;
                }
                else
                {
                    playerValue = bet.amount * LOSE;
                    state = LOSE_STATE;
                    jackpot = false;
                }
            }

            if (state == WIN_STATE)
            {
                struct punchRecord p =
                    {
                        i + 1,
                        playerPunch,
                        bankerPunch,
                        state,
                        playerValue - bet.amount};
                round.push_back(p);
                playerTotalValue += playerValue;
            }
            else if (state == DRAW_STATE)
            {
                struct punchRecord p =
                    {
                        i + 1,
                        playerPunch,
                        bankerPunch,
                        state,
                        playerValue - bet.amount};
                round.push_back(p);
                playerTotalValue += playerValue;
            }
            else
            {
                struct punchRecord p =
                    {
                        i + 1,
                        playerPunch,
                        bankerPunch,
                        state,
                        playerValue};
                round.push_back(p);
            }
            jackpotValue = 0;
        }

        if (jackpot)
        {
            updateUser(user, round);
            uint64_t bigValue = updateJackpot(GAME1, WIN_STATE, jackpotValue);
            payUser(user, bigValue + playerTotalValue);
        }
        else
        {
            updateUser(user, round);
            payUser(user, playerTotalValue);
        }
    }

    struct punchRecord
    {
        uint64_t sort;
        uint64_t playerPunch;
        uint64_t bankerPunch;
        int64_t state;
        int64_t value;
    };

    struct [[eosio::table]] user
    {
        name userName;
        std::vector<struct punchRecord> round;
        time_point_sec timestamp;
        uint64_t primary_key() const
        {
            return userName.value;
        }
    };

    struct [[eosio::table]] contractState
    {
        name game;
        uint64_t jackpot;
        uint64_t primary_key() const
        {
            return game.value;
        }
    };

    typedef eosio::multi_index<"users1"_n, user> userTable;
    typedef eosio::multi_index<"states1"_n, contractState> stateTable;

    void updateUser(name user, std::vector<struct punchRecord> round)
    {
        userTable players(_self, _self.value);
        auto iterator = players.find(user.value);
        time_point_sec timestamp = (time_point_sec)now();

        if (iterator == players.end())
        {
            players.emplace(_self, [&](auto &row) {
                row.userName = user;
                row.round = round;
                row.timestamp = timestamp;
            });
        }
        else
        {
            players.modify(iterator, _self, [&](auto &row) {
                row.round = round;
                row.timestamp = timestamp;
            });
        }
    }

    void payUser(name to, int64_t amount)
    {
        transaction txn{};
        std::string memo = BANKER_MSG;
        if (amount < 0)
        {
            return;
        }
        else
        {
            txn.actions.emplace_back(
                           eosio::permission_level(_self, "active"_n),
                           "eosio.token"_n,
                           "transfer"_n,
                           std::make_tuple(_self, to, asset(amount, symbol("EOS", 4)), memo))
                .send();
        }
    }

    uint64_t updateJackpot(name game, int64_t jackpotState, uint64_t jackpotValue)
    {
        stateTable state_table(_self, _self.value);
        auto iterator = state_table.find(game.value);

        if (iterator == state_table.end())
        {
            state_table.emplace(_self, [&](auto &row) {
                row.game = GAME1;
                row.jackpot = jackpotValue;
            });
            return 0;
        }
        else
        {
            if (jackpotState == WIN_STATE)
            {
                uint64_t res = iterator->jackpot;
                state_table.modify(iterator, _self, [&](auto &row) {
                    row.jackpot = jackpotValue;
                });
                return res;
            }
            else
            {
                state_table.modify(iterator, _self, [&](auto &row) {
                    row.jackpot += jackpotValue;
                });
                return 0;
            }
        }
    }

    uint64_t random()
    {
        timeSeed += now();
        uint64_t seed = (timeSeed % PUNCH_TYPE) + 1;
        return seed;
    }
    uint64_t getInt(char c)
    {
        uint64_t res = (uint64_t)(c - '0');

        eosio_assert(res > 0, "Your integer is too small or , man");
        eosio_assert(res < 4, "Your integer is too big, woman");

        return res;
    }
};

extern "C"
{
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        if ((code == "eosio.token"_n.value) && (action == "transfer"_n.value))
        {
            eosio::execute_action(eosio::name(receiver), eosio::name(code), &eosPunch::transferAction);
        }

        if (code == receiver)
        {
            switch (action)
            {
                EOSIO_DISPATCH_HELPER(eosPunch, (cleartables))
            }
        }
        eosio_exit(0);
    }
}
