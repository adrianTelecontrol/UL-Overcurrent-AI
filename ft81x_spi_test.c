/*
 * ft81x_spi_test.c
 * 
 * Test functions to verify SPI read/write operations with FT81x display
 * These tests verify DMA-based SPI communication is working correctly
 */

#include <stdint.h>
#include <stdbool.h>

#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>

#include <utils/uartstdio.h>

#include "FT8xx.h"
#include "helpers.h"
#include "gpu_ft81x.h"
#include "hal_tft_spi.h"
#include "tiva_log.h"

#include "ft81x_spi_test.h"


// Test counter
static uint32_t g_testsPassed = 0;
static uint32_t g_testsFailed = 0;

static const char TASK_NAME[] = "FT81x_SPI_TEST";

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Print test result
 */
void PrintTestResult(TestResult result) {
    if (result.passed) {
        TIVA_LOGI(TASK_NAME, "[PASS] %s", result.test_name);
        g_testsPassed++;
    } else {
        TIVA_LOGE(TASK_NAME, "[FAIL] %s - Expected: 0x%08X, Got: 0x%08X", 
                   result.test_name, result.expected, result.actual);
        g_testsFailed++;
    }
}

// ============================================================================
// BASIC READ TESTS
// ============================================================================

/**
 * @brief Test reading the FT81x chip ID register
 * This register always returns 0x7C on FT81x chips
 */
TestResult Test_ReadChipID(void) {
    TestResult result;
    result.test_name = "Read Chip ID (REG_ID)";
    result.expected = 0x7C;
    
    result.actual = EVE_MemRead8(REG_ID);
    result.passed = (result.actual == result.expected);
    
    return result;
}

/**
 * @brief Test reading a 16-bit register
 */
TestResult Test_Read16BitRegister(void) {
    TestResult result;
    result.test_name = "Read 16-bit Frequency Register";
    
    // Read the frequency register (should be 60MHz = 60000000 Hz for most FT81x)
    uint32_t freq = EVE_MemRead32(REG_FREQUENCY);
    
    // We just check if we got a reasonable value (between 40MHz and 80MHz)
    result.expected = 60000000;  // Typical value
    result.actual = freq;
    result.passed = (freq >= 40000000 && freq <= 80000000);
    
    return result;
}

/**
 * @brief Test reading a 32-bit register
 */
TestResult Test_Read32BitRegister(void) {
    TestResult result;
    result.test_name = "Read 32-bit CMD_READ Register";
    
    // Read command buffer read pointer (should be 0 after reset)
    result.actual = EVE_MemRead32(REG_CMD_READ);
    result.expected = 0x00000000;
    
    // Allow for some variance if display is initialized
    result.passed = (result.actual < 4096);  // Should be within command buffer
    
    return result;
}

// ============================================================================
// BASIC WRITE/READ-BACK TESTS
// ============================================================================

/**
 * @brief Test 8-bit write and read-back to RAM_G
 */
TestResult Test_Write8ReadBack(void) {
    TestResult result;
    result.test_name = "8-bit Write/Read-back to RAM_G";
    
    uint32_t testAddr = RAM_G + 0x1000;  // Use offset in graphics RAM
    uint8_t testValue = 0xA5;            // Test pattern
    
    // Write test value
    EVE_MemWrite8(testAddr, testValue);
    SysCtlDelay(MS_2_CLK(100));  // Small delay
    
    // Read back
    result.actual = EVE_MemRead8(testAddr);
    result.expected = testValue;
    result.passed = (result.actual == result.expected);
    
    return result;
}

/**
 * @brief Test 16-bit write and read-back to RAM_G
 */
TestResult Test_Write16ReadBack(void) {
    TestResult result;
    result.test_name = "16-bit Write/Read-back to RAM_G";
    
    uint32_t testAddr = RAM_G + 0x2000;
    uint16_t testValue = 0xABCD;
    
    // Write test value
    EVE_MemWrite16(testAddr, testValue);
    SysCtlDelay(MS_2_CLK(100));
    
    // Read back
    result.actual = EVE_MemRead16(testAddr);
    result.expected = testValue;
    result.passed = (result.actual == result.expected);
    
    return result;
}

/**
 * @brief Test 32-bit write and read-back to RAM_G
 */
TestResult Test_Write32ReadBack(void) {
    TestResult result;
    result.test_name = "32-bit Write/Read-back to RAM_G";
    
    uint32_t testAddr = RAM_G + 0x3000;
    uint32_t testValue = 0x12345678;
    
    // Write test value
    EVE_MemWrite32(testAddr, testValue);
    SysCtlDelay(MS_2_CLK(100));
    
    // Read back
    result.actual = EVE_MemRead32(testAddr);
    result.expected = testValue;
    result.passed = (result.actual == result.expected);
    
    return result;
}

// ============================================================================
// PATTERN TESTS
// ============================================================================

/**
 * @brief Test multiple sequential writes and reads
 */
TestResult Test_SequentialWriteRead(void) {
    TestResult result;
    result.test_name = "Sequential Write/Read (8 bytes)";
    result.passed = true;
    
    uint32_t baseAddr = RAM_G + 0x4000;
    uint8_t testData[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    
    // Write sequential bytes
    int i = 0;
    for(i = 0; i < 8; i++) {
        EVE_MemWrite8(baseAddr + i, testData[i]);
    }
    
    SysCtlDelay(MS_2_CLK(50));
    
    // Read back and verify
    for(i = 0; i < 8; i++) {
        uint8_t readValue = EVE_MemRead8(baseAddr + i);
        TIVA_LOGI(TASK_NAME, "\tValue Expected: 0x%2x,\tValue Read: 0x%2x", testData[i], readValue);
        if(readValue != testData[i]) {
            result.passed = false;
            result.expected = testData[i];
            result.actual = readValue;
            // break;
        }
    }
    
    return result;
}

/**
 * @brief Test alternating 0xFF and 0x00 pattern
 */
TestResult Test_AlternatingPattern(void) {
    TestResult result;
    result.test_name = "Alternating 0xFF/0x00 Pattern";
    result.passed = true;
    
    uint32_t baseAddr = RAM_G + 0x5000;
    
    int i = 0;
    // Write alternating pattern
    for(i = 0; i < 16; i++) {
        uint8_t value = (i % 2) ? 0xFF : 0x00;
        EVE_MemWrite8(baseAddr + i, value);
    }
    
    SysCtlDelay(MS_2_CLK(50));
    
    // Read back and verify
    for(i = 0; i < 16; i++) {
        uint8_t expected = (i % 2) ? 0xFF : 0x00;
        uint8_t actual = EVE_MemRead8(baseAddr + i);
        TIVA_LOGI(TASK_NAME, "\tValue Expected: 0x%2x,\tValue Read: 0x%2x", expected, actual);
        if(actual != expected) {
            result.passed = false;
            result.expected = expected;
            result.actual = actual;
            //break;
        }
    }
    
    return result;
}

/**
 * @brief Test byte order (endianness) for 32-bit operations
 */
TestResult Test_EndiannessCheck(void) {
    TestResult result;
    result.test_name = "32-bit Endianness Check";
    
    uint32_t testAddr = RAM_G + 0x6000;
    uint32_t testValue = 0x12345678;
    
    // Write as 32-bit
    EVE_MemWrite32(testAddr, testValue);
    SysCtlDelay(MS_2_CLK(100));
    
    // Read back as individual bytes (should be little-endian)
    uint8_t byte0 = EVE_MemRead8(testAddr + 0);  // Should be 0x78
    uint8_t byte1 = EVE_MemRead8(testAddr + 1);  // Should be 0x56
    uint8_t byte2 = EVE_MemRead8(testAddr + 2);  // Should be 0x34
    uint8_t byte3 = EVE_MemRead8(testAddr + 3);  // Should be 0x12
    
    bool byte0_ok = (byte0 == 0x78);
    bool byte1_ok = (byte1 == 0x56);
    bool byte2_ok = (byte2 == 0x34);
    bool byte3_ok = (byte3 == 0x12);
    
    result.passed = byte0_ok && byte1_ok && byte2_ok && byte3_ok;
    result.expected = 0x12345678;
    result.actual = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
    
    return result;
}

// ============================================================================
// STRESS TESTS
// ============================================================================

/**
 * @brief Test rapid successive reads
 */
TestResult Test_RapidReads(void) {
    TestResult result;
    result.test_name = "Rapid Successive Reads (100x)";
    result.passed = true;
    
    uint32_t testAddr = RAM_G + 0x7000;
    uint8_t testValue = 0xAA;
    
    // Write initial value
    EVE_MemWrite8(testAddr, testValue);
    SysCtlDelay(MS_2_CLK(500));
    
    // Perform 100 rapid reads
    int i = 0;
    for(i = 0; i < 100; i++) {
        uint8_t readValue = EVE_MemRead8(testAddr);
        if(readValue != testValue) {
            result.passed = false;
            result.expected = testValue;
            result.actual = readValue;
            break;
        }
    }
    
    return result;
}

/**
 * @brief Test rapid successive writes
 */
TestResult Test_RapidWrites(void) {
    TestResult result;
    result.test_name = "Rapid Successive Writes (100x)";
    result.passed = true;
    
    uint32_t baseAddr = RAM_G + 0x8000;
    
    // Write 100 sequential values
    int i = 0;
    for(i = 0; i < 100; i++) {
        EVE_MemWrite8(baseAddr + i, (uint8_t)i);
    }
    
    SysCtlDelay(MS_2_CLK(100));
    
    // Read back and verify
    for(i = 0; i < 100; i++) {
        uint8_t readValue = EVE_MemRead8(baseAddr + i);
        if(readValue != (uint8_t)i) {
            result.passed = false;
            result.expected = i;
            result.actual = readValue;
            break;
        }
    }
    
    return result;
}

// ============================================================================
// MAIN TEST SUITE
// ============================================================================

/**
 * @brief Run all SPI read/write tests
 * @return true if all tests passed, false otherwise
 */
bool RunAllSPITests(void) {
    g_testsPassed = 0;
    g_testsFailed = 0;
    
    TIVA_LOGI(TASK_NAME, "\n");
    TIVA_LOGI(TASK_NAME, "========================================");
    TIVA_LOGI(TASK_NAME, "  FT81x SPI Read/Write Test Suite");
    TIVA_LOGI(TASK_NAME, "========================================\n");
    
    // Basic Read Tests
    TIVA_LOGI(TASK_NAME, "--- Basic Read Tests ---");
    PrintTestResult(Test_ReadChipID());
    PrintTestResult(Test_Read16BitRegister());
    PrintTestResult(Test_Read32BitRegister());
    
    // Write/Read-back Tests
    TIVA_LOGI(TASK_NAME, "--- Write/Read-back Tests ---");
    PrintTestResult(Test_Write8ReadBack());
    PrintTestResult(Test_Write16ReadBack());
    PrintTestResult(Test_Write32ReadBack());
    
    // Pattern Tests
    TIVA_LOGI(TASK_NAME, "--- Pattern Tests ---");
    PrintTestResult(Test_SequentialWriteRead());
    PrintTestResult(Test_AlternatingPattern());
    PrintTestResult(Test_EndiannessCheck());
    
    // Stress Tests
    TIVA_LOGI(TASK_NAME, "--- Stress Tests ---");
    PrintTestResult(Test_RapidReads());
    PrintTestResult(Test_RapidWrites());
    
    // Summary
    //uint32_t totalTests = g_testsFailed + g_testsPassed;
    TIVA_LOGI(TASK_NAME, "========================================");
    TIVA_LOGI(TASK_NAME, "  Test Summary");
    TIVA_LOGI(TASK_NAME, "========================================");
    TIVA_LOGI(TASK_NAME, "Tests Passed: %u", g_testsPassed);
    TIVA_LOGI(TASK_NAME, "Tests Failed: %u", g_testsFailed);
    //TIVA_LOGI(TASK_NAME, "Total Tests: %u\n", totalTests);
    
    if(g_testsFailed == 0) {
        TIVA_LOGI(TASK_NAME, "*** ALL TESTS PASSED ***\n");
        return true;
    } else {
        TIVA_LOGE(TASK_NAME, "*** SOME TESTS FAILED ***\n");
        return false;
    }
}

/**
 * @brief Quick sanity check - just verify chip ID
 * Use this as a first test after boot
 */
bool QuickSanityCheck(void) {
    TIVA_LOGI(TASK_NAME, "Quick Sanity Check: Reading FT81x Chip ID...");
    
    uint8_t chipID = EVE_MemRead8(REG_ID);
    
    if(chipID == 0x7C) {
        TIVA_LOGI(TASK_NAME, "[PASS] Chip ID = 0x%02X (Expected 0x7C)", chipID);
        TIVA_LOGI(TASK_NAME, "SPI communication is working!");
        return true;
    } else {
        TIVA_LOGE(TASK_NAME, "[FAIL] Chip ID = 0x%02X (Expected 0x7C)", chipID);
        return false;
    }
}

/**
 * @brief Diagnostic function to show raw SPI transfer
 * Useful for debugging with logic analyzer
 */
void DiagnosticRawRead(uint32_t address) {
    TIVA_LOGI(TASK_NAME, "\nDiagnostic: Reading from address 0x%08X", address);
    
    // Turn on LED to mark timing on logic analyzer
    
    uint8_t value = EVE_MemRead8(address);
    
    
    TIVA_LOGI(TASK_NAME, "Read value: 0x%02X", value);
}


