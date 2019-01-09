/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/symbol.hpp>
#include <eosiolib/multi_index.hpp>

#include <string>
#include <vector>
#include <eosio.token.hpp>

using eosio::action;
using eosio::asset;
using eosio::name;
using eosio::permission_level;
using eosio::print;
using eosio::time_point_sec;
using eosio::transaction;
using eosio::unpack_action_data;
using std::string;