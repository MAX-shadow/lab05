#include <gtest/gtest.h>

#include <stdexcept>

#include "Account.h"

// Конструктор сохраняет id и начальный баланс.
TEST(AccountTest, ConstructorAndGetBalance) {
  Account acc(1, 100);
  EXPECT_EQ(acc.id(), 1);
  EXPECT_EQ(acc.GetBalance(), 100);
}

// Изменение баланса без блокировки запрещено.
TEST(AccountTest, ChangeBalanceWithoutLockThrows) {
  Account acc(1, 100);
  EXPECT_THROW(acc.ChangeBalance(50), std::runtime_error);
  EXPECT_EQ(acc.GetBalance(), 100);
}

// После блокировки баланс меняется корректно.
TEST(AccountTest, ChangeBalanceAfterLock) {
  Account acc(1, 100);
  acc.Lock();
  acc.ChangeBalance(50);
  EXPECT_EQ(acc.GetBalance(), 150);
  acc.ChangeBalance(-70);
  EXPECT_EQ(acc.GetBalance(), 80);
}

// Повторная блокировка запрещена.
TEST(AccountTest, DoubleLockThrows) {
  Account acc(1, 100);
  acc.Lock();
  EXPECT_THROW(acc.Lock(), std::runtime_error);
}

// После разблокировки счёт можно заблокировать снова.
TEST(AccountTest, UnlockAllowsRelock) {
  Account acc(1, 100);
  acc.Lock();
  acc.Unlock();
  EXPECT_NO_THROW(acc.Lock());
}
