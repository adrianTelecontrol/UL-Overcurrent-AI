#ifndef _FT81x_SPI_TEST_
#define _FT81x_SPI_TEST_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool passed;
    uint32_t expected;
    uint32_t actual;
    const char* test_name;
} TestResult;


void PrintTestResult(TestResult result);

bool RunAllSPITests(void);

void DiagnosticRawRead(uint32_t address);

TestResult Test_RapidWrites(void);

bool QuickSanityCheck(void);

#endif // _FT81x_SPI_TEST_




