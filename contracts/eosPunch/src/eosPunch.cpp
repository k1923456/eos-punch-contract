#include <eosPunch.hpp>

using namespace eosio;

// #define DEBUG 1
#define NOBET 1

class[[eosio::contract]] eosPunch : public eosio::contract
{

  public:
    using contract::contract;

    eosPunch(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds) {}

    const std::string BANKER_MSG = "banker_pay";
    const uint64_t MAX_PUNCHES = 5;
    const uint64_t PUNCH_TYPE = 3;
    const double DRAW = 0.9;
    const double WIN = 2.0;
    const double LOSE = -1.0;
    const int64_t WIN_STATE = 1;
    const int64_t LOSE_STATE = -1;
    const int64_t DRAW_STATE = 0;
    const name GAME1 = "version1"_n;

    [[eosio::action]] void punch(name user, asset bet, std::vector<uint64_t> punches) {
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
        print("ROUND_SIZE_INIT=", round.size());

#ifndef DEBUG

        print("User ", user, " punched SIZE ", punches.size(), ", ");
        for (i = 0; i < punches.size(); i++)
        {
            print(punches[i]);
            print(", ");
        }
        print("with bet ", bet.amount, " ", bet.symbol, ", Banker: ");

        eosio_assert(bet.amount >= -eosio::asset::max_amount, "input bet underflow");
        eosio_assert(bet.amount <= eosio::asset::max_amount, "input bet overflow");
        for (i = 0; i < punches.size(); i++)
        {
            eosio_assert(punches[i] <= PUNCH_TYPE, "Invalid punch type!");
        }

        for (i = 0; i < punches.size(); i++)
        {
            playerPunch = punches[i];
            bankerPunch = random();
            print("[rsize::", round.size(), "]");

            if (playerPunch != 2 && (playerPunch + bankerPunch) == 4)
            {
                if (playerPunch == 1)
                {
                    print("[", bankerPunch, ":Win(1->3)], ");
                    playerValue = bet.amount * WIN;
                    print("playerValue=", playerValue);
                    state = WIN_STATE;
                }
                else
                {
                    print("[", bankerPunch, ":Lose(3->1)], ");
                    playerValue = bet.amount * LOSE;
                    print("playerValue=", playerValue);
                    jackpot = false;
                    state = LOSE_STATE;
                }
            }
            else
            {
                if (playerPunch == bankerPunch)
                {
                    print("[", bankerPunch, ":Draw], ");
                    playerValue = bet.amount * DRAW;
                    print("playerValue=", playerValue);
                    jackpotValue = (bet.amount - playerValue) / 2;
                    updateJackpot(GAME1, DRAW_STATE, jackpotValue);
                    jackpot = false;
                    state = DRAW_STATE;
                }
                else if (playerPunch > bankerPunch)
                {
                    print("[", bankerPunch, ":Win], ");
                    playerValue = bet.amount * WIN;
                    print("playerValue=", playerValue);
                    state = WIN_STATE;
                }
                else //lose
                {
                    print("[", bankerPunch, ":Lose], ");
                    playerValue = bet.amount * LOSE;
                    print("playerValue=", playerValue);
                    jackpot = false;
                    state = LOSE_STATE;
                }
            }
            if (state == WIN_STATE)
            {
                print("INININ_WIN_STATE");
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
                        playerValue};
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
        }
        print("{{ROUNDSIZE:", round.size(), "}}");
        updateUser(user, round);
        payUser(user, playerTotalValue);

        if (jackpot)
        {
            uint64_t bigValue = updateJackpot(GAME1, WIN_STATE, jackpotValue);
            payUser(user, bigValue);
        }
#else
        print("DEBUG");
        for (i = 0; i < 5; i++)
        {
            struct punchRecord p =
                {
                    1,
                    1,
                    0,
                    bet.amount};
            round.push_back(p);
        }
        print("round[0].playerPunch:", round[0].playerPunch, ",");
        print("round[0].bankerPunch:", round[0].bankerPunch, ",");
        print("round[0].state:", round[0].state, ",");
        print("round[0].value:", round[0].value, ",");
        updateUser(user, round);
#endif
    }

    void
    transferAction(name from,
                   name to,
                   asset quantity,
                   std::string memo)
    {
#ifndef NOBET
        print("Require receipant works!!!Memo = ");
        printf("%s", memo.c_str());
        print("!!!");
        const symbol sym(symbol_code("EOS"), 4);
        const auto my_balance = eosio::token::get_balance("eosio.token"_n, get_self(), sym.code());
        print("QUANTITY_ASSET=", quantity.amount, "|||");
        print("BANKER_BALANCE=", my_balance.amount, "||||||||||");

        eosio_assert(_code == "eosio.token"_n, "I reject your non-eosio.token deposit");
        eosio_assert(quantity.amount <= my_balance.amount, "Banker has been brokenF");

        if (to == _self && !memo.empty() && memo.compare(BANKER_MSG) != 0)
        {
            int i = 0;
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
                    print("NNN");
                }
            }

            eosio_assert(punches.size() == MAX_PUNCHES, "Numbers of punches doesn't match!");
            this->punch(from, quantity / MAX_PUNCHES, punches);
        }
        else
        {
        }
#else

#endif
    }

  private:
    struct punchRecord
    {
        uint64_t id;
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

    typedef eosio::multi_index<"users"_n, user> userTable;
    typedef eosio::multi_index<"states"_n, contractState> stateTable;

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
        uint64_t seed = (now() % PUNCH_TYPE) + 1;
        return seed;
    }
    uint64_t getInt(char c)
    {
        uint64_t res = (uint64_t)(c - '0');
        eosio_assert(res > 0, "Your character is too small, man");
        eosio_assert(res < 4, "Your character is too big, woman");
        print("{c=", c);
        print(",res=", res);
        print("}");

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
                EOSIO_DISPATCH_HELPER(eosPunch, (punch))
            }
        }
        eosio_exit(0);
    }
}
