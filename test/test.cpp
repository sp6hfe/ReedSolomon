#include <array>
#include <optional>
#include <stdint.h>
#include <stdio.h>

#include "../ReedSolomon.hpp"

// RS(15,9)
static constexpr auto codewordSize{15U};
static constexpr auto userDataSize{9U};
static constexpr auto fecSize{codewordSize - userDataSize};
static constexpr auto allowedErrorneousSymbols{fecSize / 2U};
static constexpr auto symbolSize{4U}; // log2(codewordSize + 1) -> 4

void printUint16(uint16_t data)
{
  printf("%0X", data);
}

void printMessage(const reedsolomon::ReedSolomon<symbolSize, allowedErrorneousSymbols>::Message &message)
{
  for (auto element : message)
  {
    printUint16(element);
    printf(" ");
  }
}

void printCodeword(const reedsolomon::ReedSolomon<symbolSize, allowedErrorneousSymbols>::Codeword &codeword)
{
  for (auto element : codeword)
  {
    printUint16(element);
    printf(" ");
  }
}

bool validateMessages(
    const reedsolomon::ReedSolomon<symbolSize, allowedErrorneousSymbols>::Message &message1,
    const reedsolomon::ReedSolomon<symbolSize, allowedErrorneousSymbols>::Message &message2)
{
  static constexpr bool NO_ERROR{false};
  static constexpr bool AN_ERROR{true};

  auto index{0};
  for (auto element : message1)
  {
    if (element != message2.at(index++))
    {
      return AN_ERROR;
    }
  }

  return NO_ERROR;
}

int main()
{
  // Test input
  const reedsolomon::ReedSolomon<symbolSize, allowedErrorneousSymbols>::Message message = {6, 15, 8, 9, 8, 3, 0, 0, 5};
  const reedsolomon::ReedSolomon<symbolSize, allowedErrorneousSymbols>::Codeword expectedCodeword = {6, 15, 8, 9, 8, 3, 0, 0, 5, 0, 12, 11, 2, 0, 9};

  printf("\nTest for recovery from transmission channel errors using Reed-Solomon forward error correcion RS(%d,%d)\n", codewordSize, userDataSize);

  /* 1. Intstantiate RS(15,9) engine */
  reedsolomon::ReedSolomon<symbolSize, allowedErrorneousSymbols> rs{};

  // Validate calculated parameters
  {
    const auto codewordSizRead{rs.getCodewordSize()};
    if (codewordSize != codewordSizRead)
    {
      printf("Error: codeword size expected: %d but read %d", codewordSize, codewordSizRead);
      return -1;
    }

    const auto userDataSizeRead{rs.getMessageSize()};
    if (userDataSize != userDataSizeRead)
    {
      printf("Error: user data size expected: %d but read %d", userDataSize, userDataSizeRead);
      return -1;
    }

    const auto fecSizeRead{rs.getFecSize()};
    if (fecSize != fecSizeRead)
    {
      printf("Error: FEC size expected: %d but got %d", fecSize, fecSizeRead);
      return -1;
    }

    const auto symbolSizeRead{rs.getSymbolSize()};
    if (symbolSize != symbolSizeRead)
    {
      printf("Error: symbol bit size expected: %d but got %d", symbolSize, symbolSizeRead);
      return -1;
    }
  }

  printf("\nMessage to send:    ");
  printMessage(message);

  /* 2. Encode message into codeword */
  const auto codeword{rs.generateCodeword(message)};

  printf("\nGenerated codeword: ");
  printCodeword(codeword);

  // Validate codeword
  {
    auto index{0U};
    for (auto element : codeword)
    {
      if (element != expectedCodeword.at(index++))
      {
        printf("\nError: Generated codeword do not match expected one:");
        printf("\n                  ");
        printCodeword(expectedCodeword);
        return -1;
      }
    }
  }

  /* 3. Recover message from not modified codeword */
  {
    const auto messageRecovered{rs.recoverMessage(codeword)};
    if (not messageRecovered.has_value())
    {
      printf("\nError: It was not possible to recover the message out of not modified codeword");
      return -1;
    }

    printf("\nRecovered message:  ");
    printMessage(messageRecovered.value());

    // Validate recovered message
    const auto validationError{validateMessages(messageRecovered.value(), message)};
    if (validationError)
    {
      printf("\nError: Recovered message do not match sent one");
      return -1;
    }
  }

  auto errorneousCodeword{codeword};

  /* 4. Make 1 error in the received codeword (say in the message area) and try to recover original message */
  {
    errorneousCodeword.at(2U) = 0x0;

    printf("\n\nSimulating transmission channel issue producing 1 error in the message area");
    printf("\nFaulty codeword:    ");
    printCodeword(errorneousCodeword);

    const auto messageRecovered{rs.recoverMessage(errorneousCodeword)};
    if (not messageRecovered.has_value())
    {
      printf("\nError: It was not possible to recover the message out of received codeword");
      return -1;
    }

    printf("\nRecovered message:  ");
    printMessage(messageRecovered.value());

    // Validate recovered message
    const auto validationError{validateMessages(messageRecovered.value(), message)};
    if (validationError)
    {
      printf("\nError: Recovered message do not match sent one");
      return -1;
    }
  }

  /* 5. Make 2nd error in the received codeword (say also in the message area) and try to recover original message */
  {
    errorneousCodeword.at(3U) = 0x0;

    printf("\n\nSimulating transmission channel issue producing 2 errors in the message area");
    printf("\nFaulty codeword:    ");
    printCodeword(errorneousCodeword);

    const auto messageRecovered{rs.recoverMessage(errorneousCodeword)};
    if (not messageRecovered.has_value())
    {
      printf("\nError: It was not possible to recover the message out of received codeword");
      return -1;
    }

    printf("\nRecovered message:  ");
    printMessage(messageRecovered.value());

    // Validate recovered message
    const auto validationError{validateMessages(messageRecovered.value(), message)};
    if (validationError)
    {
      printf("\nError: Recovered message do not match sent one");
      return -1;
    }
  }

  /* 6. Make 3rd error in the received codeword (say in the FEC area) and try to recover original message */
  {
    errorneousCodeword.at(11U) = 0x0;

    printf("\n\nSimulating transmission channel issue producing 3 errors in the codeword");
    printf("\nFaulty codeword:    ");
    printCodeword(errorneousCodeword);

    const auto messageRecovered{rs.recoverMessage(errorneousCodeword)};
    if (not messageRecovered.has_value())
    {
      printf("\nError: It was not possible to recover the message out of received codeword");
      return -1;
    }

    printf("\nRecovered message:  ");
    printMessage(messageRecovered.value());

    // Validate recovered message
    const auto validationError{validateMessages(messageRecovered.value(), message)};
    if (validationError)
    {
      printf("\nError: Recovered message do not match sent one");
      return -1;
    }
  }

  /* 7. Make 4th error in the received codeword which is too much for RS(15,9) */
  {
    errorneousCodeword.at(0U) = 0x0;

    printf("\n\nSimulating transmission channel issue producing 4 errors in the codeword");
    printf("\nFaulty codeword:    ");
    printCodeword(errorneousCodeword);

    const auto messageRecovered{rs.recoverMessage(errorneousCodeword)};
    if (messageRecovered.has_value())
    {
      printf("\nError: It should't be possible to recover the message out of received codeword due to excessive amount of errors");
      return -1;
    }

    printf("\nMessage recovering failed as expected");
  }

  printf("\n\nPASSED\n");
  return 0;
}