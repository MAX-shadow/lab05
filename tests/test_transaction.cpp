#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

#include "Account.h"
#include "Transaction.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

// Mock-объект счёта: подменяем виртуальные методы, чтобы управлять
// поведением Account из тестов Transaction.
class MockAccount : public Account {
 public:
  MockAccount(int id, int balance) : Account(id, balance) {}

  MOCK_METHOD(int, GetBalance, (), (const, override));
  MOCK_METHOD(void, ChangeBalance, (int diff), (override));
  MOCK_METHOD(void, Lock, (), (override));
  MOCK_METHOD(void, Unlock, (), (override));
};

// Mock-объект транзакции: подменяем только SaveToDataBase, остальная
// логика Make/Credit/Debit остаётся настоящей.
class MockTransaction : public Transaction {
 public:
  MOCK_METHOD(void, SaveToDataBase, (Account & from, Account & to, int sum),
              (override));
};

// Перевод на тот же счёт недопустим.
TEST(TransactionTest, SameAccountThrows) {
  NiceMock<MockAccount> from(1, 1000);
  NiceMock<MockAccount> to(1, 1000);
  Transaction tx;
  EXPECT_THROW(tx.Make(from, to, 100), std::logic_error);
}

// Отрицательная сумма перевода недопустима.
TEST(TransactionTest, NegativeSumThrows) {
  NiceMock<MockAccount> from(1, 1000);
  NiceMock<MockAccount> to(2, 1000);
  Transaction tx;
  EXPECT_THROW(tx.Make(from, to, -100), std::invalid_argument);
}

// Сумма меньше минимально допустимой (100) отклоняется.
TEST(TransactionTest, TooSmallSumThrows) {
  NiceMock<MockAccount> from(1, 1000);
  NiceMock<MockAccount> to(2, 1000);
  Transaction tx;
  EXPECT_THROW(tx.Make(from, to, 50), std::logic_error);
}

// Если комиссия слишком велика относительно суммы — перевод не выполняется.
TEST(TransactionTest, FeeTooBigReturnsFalse) {
  NiceMock<MockAccount> from(1, 1000);
  NiceMock<MockAccount> to(2, 1000);
  Transaction tx;
  tx.set_fee(60);  // fee * 2 = 120 > 100
  EXPECT_FALSE(tx.Make(from, to, 100));
}

// Успешный перевод: на счёте получателя достаточно средств для списания.
TEST(TransactionTest, SuccessfulTransfer) {
  NiceMock<MockAccount> from(1, 1000);
  NiceMock<MockAccount> to(2, 1000);
  MockTransaction tx;  // SaveToDataBase замокан

  // Guard блокирует оба счёта и снимает блокировку в деструкторе.
  EXPECT_CALL(from, Lock());
  EXPECT_CALL(from, Unlock());
  EXPECT_CALL(to, Lock());
  EXPECT_CALL(to, Unlock());

  // Credit зачисляет сумму, затем Debit списывает сумму + комиссию.
  EXPECT_CALL(to, ChangeBalance(200));   // Credit
  EXPECT_CALL(to, GetBalance()).WillOnce(Return(1000));
  EXPECT_CALL(to, ChangeBalance(-201));  // Debit (200 + fee=1)

  EXPECT_CALL(tx, SaveToDataBase(_, _, 200));

  EXPECT_TRUE(tx.Make(from, to, 200));
}

// Недостаточно средств для списания: перевод откатывается и возвращает false.
TEST(TransactionTest, InsufficientFundsRollback) {
  NiceMock<MockAccount> from(1, 1000);
  NiceMock<MockAccount> to(2, 1000);
  MockTransaction tx;

  EXPECT_CALL(from, Lock());
  EXPECT_CALL(from, Unlock());
  EXPECT_CALL(to, Lock());
  EXPECT_CALL(to, Unlock());

  EXPECT_CALL(to, ChangeBalance(200));   // Credit
  EXPECT_CALL(to, GetBalance()).WillOnce(Return(100));  // < 201 -> Debit неудача
  EXPECT_CALL(to, ChangeBalance(-200));  // откат зачисления

  EXPECT_CALL(tx, SaveToDataBase(_, _, 200));

  EXPECT_FALSE(tx.Make(from, to, 200));
}

// Интеграционный тест с настоящими объектами: проверяет реальную реализацию
// SaveToDataBase и сквозной сценарий перевода.
TEST(TransactionTest, RealObjectsSuccessfulTransfer) {
  Account from(1, 1000);
  Account to(2, 1000);
  Transaction tx;
  EXPECT_TRUE(tx.Make(from, to, 200));
}

// Геттер и сеттер комиссии.
TEST(TransactionTest, FeeAccessors) {
  Transaction tx;
  EXPECT_EQ(tx.fee(), 1);
  tx.set_fee(5);
  EXPECT_EQ(tx.fee(), 5);
}
