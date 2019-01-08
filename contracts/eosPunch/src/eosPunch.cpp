#include <eosPunch.hpp>

using namespace eosio;

class eosPunch : public contract
{
public:
    using contract::contract;
    const uint64_t MAX_PUNCHES = 5;
    const uint64_t PUNCH_TYPE = 3; // 1; paper, 2: scissors, 3: stone
    const double DRAW = 0.9; // 5% for banker, 5% for jackpot
    const double WIN = 2.0;
    const double LOSE = -1.0;
    // const double MIN_BET = 0.1;

    [[eosio::action]]
    void hello( name user )
    {
        print( "User ", user, "Punched something.");
    }

    [[eosio::action]]
    void punch( name user, std::vector<uint64_t>& punches, asset bet )
    {
        int i = 0;
        uint64_t playerPunch = 0;
        uint64_t bankerPunch = 0;
        bool jackpot = true;
        std::vector<double> punchResult;

        print("User ", user);
        print(" punched ");
        for (i = 0; i < punches.size(); i++)
        {
            eosio_assert( punches[i] <= PUNCH_TYPE, "Invalid punch type!" );
            print(punches[i]);
            print(", ");
        }
        print("with bet ", bet.amount / 10000);
        print(" EOS, Banker: ");

        eosio_assert( punches.size() == MAX_PUNCHES, "Numbers of punches doesn't match!" );

        for (i = 0; i < punches.size(); i++)
        {
            playerPunch = punches[i];
            bankerPunch = random();

            // paper v.s stone or stone v.s paper
            if (playerPunch != 2 && (playerPunch + bankerPunch) == 4)
            {
                if (playerPunch == 1)
                {
                    print("[", bankerPunch);
                    print(":Win(1->3)], ");
                    punchResult.push_back(bet.amount * WIN);
                }
                else
                {
                    print("[", bankerPunch);
                    print(":Lose(3->1)], ");
                    jackpot = false;
                    punchResult.push_back(bet.amount * LOSE);
                }
                continue;
            }

            if (playerPunch == bankerPunch) // draw
            {
                print("[", bankerPunch);
                print(":Draw], ");
                jackpot = false;
                punchResult.push_back(bet.amount * DRAW);
            }
            else if (playerPunch > bankerPunch) // win
            {
                print("[", bankerPunch);
                print(":Win], ");
                punchResult.push_back(bet.amount * WIN);
            }
            else //lose
            {
                print("[", bankerPunch);
                print(":Lose], ");
                jackpot = false;
                punchResult.push_back(bet.amount * LOSE);
            }
        }

        users players(_self, _code.value);
        auto iterator = players.find(user.value);
        if (iterator == players.end())
        {
            players.emplace(user, [&](auto & row)
            {
                struct round tmpRound =
                {
                    punchResult,
                    time_point_sec(now())
                };
                std::vector<struct round> punchHistory;

                row.userName = user;
                row.punchHistory = punchHistory;
                row.punchHistory.push_back(tmpRound);

            });
        }
        else
        {
            players.modify(iterator, user, [&](auto & row)
            {
                struct round tmpRound =
                {
                    punchResult,
                    time_point_sec(now())
                };

                row.userName = user;
                row.punchHistory.push_back(tmpRound);
            });
        }

    }
    // void transferAction( name    from,
    //                      name    to,
    //                      asset   quantity,
    //                      std::string  memo )
    // {
    //     print("Require receipant works!!!");

    // eosio_assert(_code == "eosio.token"_n, "I reject your non-eosio.token deposit");
    // auto data = unpack_action_data<token::transfer>();
    // if (data.from == _self || data.to != _self)
    //     return;
    // eosio_assert(data.quantity.symbol == string_to_symbol(4, "EOS"),
    //              "I think you're looking for another contract");
    // eosio_assert(data.quantity.is_valid(), "Are you trying to corrupt me?");
    // eosio_assert(data.quantity.amount > 0, "When pigs fly");

    //     print("And passed the assertion!!!!");
    // }

private:

    struct round
    {
        std::vector<double> punchResult;// positive: player win or draw; negative: player lose
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

    typedef eosio::multi_index< "users"_n, user > users;

    uint64_t random()
    {
        uint64_t seed = (now() % PUNCH_TYPE) + 1;
        return seed;
    }
};

EOSIO_DISPATCH( eosPunch, (hello)(punch))
// extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
// {
//     switch (action)
//     {
//     case N(transfer):
//         return transferAction(receiver, code);
//     }
// }
extern "C"
{
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action)
    {
        if ((code == "eosio.token"_n.value) && (action == "transfer"_n.value))
        {
            eosio::execute_action(eosio::name(receiver), eosio::name(code), &eosPunch::transfer);
        }

        if (code == receiver)
        {
            switch (action)
            {
                EOSIO_DISPATCH_HELPER(eosPunch, (hello)(punch))
            }
        }
        eosio_exit(0);
    }
}