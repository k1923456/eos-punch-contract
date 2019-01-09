#include <eosPunch.hpp>

using namespace eosio;

class eosPunch : public contract
{

public:

    using contract::contract;
    const uint64_t MAX_PUNCHES = 5;
    const uint64_t PUNCH_TYPE = 3; // 1; paper, 2: scissors, 3: stone
    const double DRAW = 0.9;       // 5% for banker, 5% for jackpot
    const double WIN = 2.0;
    const double LOSE = -1.0;
    // const double MIN_BET = 0.1;

    [[eosio::action]]
    void punch( name user, asset bet, std::vector<uint64_t> punches )
    {
        int i = 0;
        int64_t playerWin = 0;  // Player's win value

        uint64_t bankerWin = 0;  // Banker's win value
        uint64_t jackpotWin = 0; // Jackpot's win value
        uint64_t playerPunch = 0;
        uint64_t bankerPunch = 0;

        bool jackpot = true;
        std::vector<double> punchResult;

        // Print parameters
        print("User ", user, " punched ");
        for (i = 0; i < punches.size(); i++)
        {
            eosio_assert( punches[i] <= PUNCH_TYPE, "Invalid punch type!" );
            print(punches[i]);
            print(", ");
        }
        print("with bet ", bet.amount, " ", bet.symbol, ", Banker: ");

        // Pre-check
        eosio_assert( punches.size() == MAX_PUNCHES, "Numbers of punches doesn't match!" );
        eosio_assert( bet.amount >= -eosio::asset::max_amount, "input bet underflow" );
        eosio_assert( bet.amount <= eosio::asset::max_amount,  "input bet overflow" );

        for (i = 0; i < punches.size(); i++)
        {
            playerPunch = punches[i];
            bankerPunch = random();

            // paper v.s stone or stone v.s paper
            if (playerPunch != 2 && (playerPunch + bankerPunch) == 4)
            {
                if (playerPunch == 1) // win
                {
                    print("[", bankerPunch, ":Win(1->3)], ");
                    playerWin = bet.amount * WIN;
                }
                else // lose
                {
                    print("[", bankerPunch, ":Lose(3->1)], ");
                    playerWin = bet.amount * LOSE;
                    jackpot = false;
                }
                continue;
            }

            if (playerPunch == bankerPunch) // draw
            {
                print("[", bankerPunch, ":Draw], ");
                playerWin = bet.amount * DRAW;
                jackpot = false;
            }
            else if (playerPunch > bankerPunch) // win
            {
                print("[", bankerPunch, ":Win], ");
                playerWin = bet.amount * WIN;
            }
            else //lose
            {
                print("[", bankerPunch, ":Lose], ");
                playerWin = bet.amount * LOSE;
                jackpot = false;
            }
        }

        punchResult.push_back(bet.amount * WIN);
        // updateUser();
        // payUser();

    }

    void transferAction( name    from,
                         name    to,
                         asset   quantity,
                         std::string  memo )
    {
        print("Require receipant works!!!Memo = ");
        printf("%s", memo.c_str());
        print("!!!");

        eosio_assert(_code == "eosio.token"_n, "I reject your non-eosio.token deposit");
        // auto data = unpack_action_data<token::transfer>();
        // if (data.from == _self || data.to != _self)
        //     return;
        // eosio_assert(data.quantity.symbol == string_to_symbol(4, "EOS"),
        //              "I think you're looking for another contract");
        // eosio_assert(data.quantity.is_valid(), "Are you trying to corrupt me?");
        // eosio_assert(data.quantity.amount > 0, "When pigs fly");

        std::vector<uint64_t> punches;

        int i = 0;
        uint64_t tmp = 0;
        for (i = 0; i < memo.size(); i++) // When i == memo.size(), i is null character
        {
            tmp = getInt(memo[i]);
            if ( tmp >= 1 && tmp <= 3 )
            {
                punches.push_back(tmp);
            }
            else
            {
                print("NNN");
            }
        }

        this->punch(from, quantity, punches);
    }

private:

    struct round
    {
        std::vector<double> punchResult; // positive: player win or draw; negative: player lose
        time_point_sec timestamp;
    };

    struct [[eosio::table]] user
    {
        name userName;
        std::vector<struct round> punchHistory;
        uint64_t primary_key() const
        {
            return userName.value;
        }
    };

    // typedef eosio::multi_index< "users"_n, user > users;

    // void updateUser()
    // {
    //     users players(_self, _code.value);
    //     auto iterator = players.find(user.value);
    //     if (iterator == players.end())
    //     {
    //         players.emplace(user, [&](auto & row)
    //         {
    //             struct round tmpRound =
    //             {
    //                 punchResult,
    //                 time_point_sec(now())
    //             };
    //             std::vector<struct round> punchHistory;

    //             row.userName = user;
    //             row.punchHistory = punchHistory;
    //             row.punchHistory.push_back(tmpRound);

    //         });
    //     }
    //     else
    //     {
    //         players.modify(iterator, user, [&](auto & row)
    //         {
    //             struct round tmpRound =
    //             {
    //                 punchResult,
    //                 time_point_sec(now())
    //             };

    //             row.userName = user;
    //             row.punchHistory.push_back(tmpRound);
    //         });
    //     }
    // }

    // void payUser() {}

    uint64_t random()
    {
        uint64_t seed = (now() % PUNCH_TYPE) + 1;
        return seed;
    }
    uint64_t getInt(char c)
    {
        uint64_t res = (uint64_t)(c - '0');
        print("{c=", c);
        print(",res=", res);
        print("}");

        return res;
    }
};

extern "C"
{
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action)
    {
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