#ifndef BANK_H
#define BANK_H

#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bank {
inline std::string funds_error(int availableMoney, int requestedMoney);
struct transfer_error : std::runtime_error {
    explicit transfer_error(const std::string &message);
};
struct not_enough_funds_error : transfer_error {
    not_enough_funds_error(int availableMoney, int requestedMoney);
};
struct empty_balance : transfer_error {
    empty_balance();
};
struct self_transaction : transfer_error {
    self_transaction();
};
class user;
struct transaction {
    // NOLINTNEXTLINE (misc-non-private-member-variables-in-classes)
    const int balance_delta_xts;
    // NOLINTNEXTLINE (misc-non-private-member-variables-in-classes)
    const std::string comment;
    // NOLINTNEXTLINE (misc-non-private-member-variables-in-classes)
    const user *const counterparty;
    transaction(const user *name, int balance, std::string message) noexcept;
};
class user_transactions_iterator {
private:
    const user *client;
    int countTransactions = 0;

public:
    user_transactions_iterator(int userTransactions, const user *newClient);
    transaction wait_next_transaction() noexcept;
};

class user {
private:
    const std::string nickname;
    int balance;
    std::vector<transaction> transactions;
    mutable std::mutex mutex;
    mutable std::condition_variable conditionVariable;

public:
    friend user_transactions_iterator;
    explicit user(std::string name);
    [[nodiscard]] const std::string &name() const noexcept;

    [[nodiscard]] int balance_xts() const;

    template <typename T>
    user_transactions_iterator snapshot_transactions(const T &f) const {
        std::unique_lock locked(mutex);
        f(transactions, balance);
        return {static_cast<int>(transactions.size()), this};
    }

    void transfer(user &counterparty,
                  int amount_xts,
                  const std::string &comment);

    [[nodiscard]] user_transactions_iterator monitor() const noexcept;
};

class ledger {
private:
    std::unordered_map<std::string, std::unique_ptr<user>> clients;
    mutable std::mutex mutex;

public:
    user &get_or_create_user(const std::string &name);
};
}  // namespace bank
#endif