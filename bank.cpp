#include "bank.h"

namespace bank {

const int startupCapital = 100;

const std::string initialDepositFor = "Initial deposit for ";

const std::string emptyBalance = "You don't have money";

const std::string selfTransfer = "You cannot transfer money to yourself";

inline std::string funds_error(int availableMoney, int requestedMoney) {
    return "Not enough funds: " + std::to_string(availableMoney) +
           " XTS available, " + std::to_string(requestedMoney) +
           " XTS requested";
}

transaction::transaction(const user *name,
                         int balance,
                         // cppcheck-suppress passedByValue
                         std::string message) noexcept
    : balance_delta_xts(balance),
      comment(std::move(message)),
      counterparty(name) {
}

user::user(std::string name)
    : nickname(std::move(name)),
      balance(startupCapital),
      transactions{
          transaction(nullptr, startupCapital, initialDepositFor + nickname)} {
    conditionVariable.notify_all();
}

[[nodiscard]] const std::string &user::name() const noexcept {
    return nickname;
}

[[nodiscard]] int user::balance_xts() const {
    std::unique_lock locked(mutex);
    return balance;
}

void user::transfer(user &counterparty,
                    int amount_xts,
                    const std::string &comment) {
    std::scoped_lock locked(mutex, counterparty.mutex);
    if (balance < amount_xts) {
        throw not_enough_funds_error(balance, amount_xts);
    }
    if (balance <= 0) {
        throw empty_balance();
    }
    if (this == &counterparty) {
        throw self_transaction();
    }
    balance -= amount_xts;
    counterparty.balance += amount_xts;
    counterparty.transactions.emplace_back(
        transaction{this, amount_xts, comment});
    transactions.emplace_back(transaction{&counterparty, -amount_xts, comment});
    conditionVariable.notify_all();
}

user &ledger::get_or_create_user(const std::string &name) {
    std::unique_lock locked(mutex);
    clients.try_emplace(name, std::make_unique<user>(name));
    return *clients[name];
}

user_transactions_iterator::user_transactions_iterator(int userTransactions,
                                                       const user *newClient)
    : client(newClient), countTransactions(userTransactions) {
}

transaction user_transactions_iterator::wait_next_transaction() noexcept {
    std::unique_lock l(client->mutex);
    client->conditionVariable.wait(l, [&]() {
        return countTransactions <
               static_cast<int>(client->transactions.size());
    });
    return client->transactions[countTransactions++];
}

[[nodiscard]] user_transactions_iterator user::monitor() const noexcept {
    std::unique_lock l(mutex);
    return {static_cast<int>(transactions.size()), this};
}

transfer_error::transfer_error(const std::string &message)
    : std::runtime_error(message) {
}

not_enough_funds_error::not_enough_funds_error(int availableMoney,
                                               int requestedMoney)
    : transfer_error(bank::funds_error(availableMoney, requestedMoney)) {
}

empty_balance::empty_balance() : transfer_error(bank::emptyBalance) {
}

self_transaction::self_transaction() : transfer_error(bank::selfTransfer) {
}
}  // namespace bank