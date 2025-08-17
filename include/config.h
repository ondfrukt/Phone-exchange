// Här inkluderas gemensamma hårdvaru-konstatner som ska användas lite överallt.
// config.h inkuderas sedan i varje fil som behöver det.
// Här läggs inte stora bibliotek in


namespace cfg {

  constexpr uint8_t ACTIVE_LINES = 4; // Antal aktiva linjer

  namespace ESP_LED_PINS {
    constexpr int StatusLED_PIN = 21;
    constexpr int WiFiLED_PIN = 24;
    constexpr int MQTTLED_PIN= 25;
  }

  namespace i2c {
    constexpr int SDA_PIN= 9;
    constexpr int SCL_PIN= 10;
  }

  namespace ad9833 {
    constexpr int CS1_PIN= 5;
    constexpr int CS2_PIN = 4;
    constexpr int CS3_PIN = 2;
    constexpr int SCLK_PIN = 12;
    constexpr int MOSI_PIN = 1;
  }

  namespace mcp {
    constexpr int MCP_INT_PIN = 18;
    constexpr int MCP_SLIC_INT_1_PIN = 11;
    constexpr int MCP_SLIC_INT_2_PIN = 14;

    constexpr int TEST_BUTTON= 9; // GPB1

    constexpr uint8_t MCP_ADDRESS = 0x20;
    constexpr uint8_t MCP_MT8816_ADDRESS = 0x21;
    constexpr uint8_t MCP_SLIC1_ADDRESS = 0x22;
    constexpr uint8_t MCP_SLIC2_ADDRESS = 0x23;
  }

  namespace externalGPIO {
    constexpr int EX1 = 4; // GPA4
    constexpr int EX2 = 5; // GPA5
    constexpr int EX3 = 6; // GPA6
    constexpr int EX4 = 7; // GPA7
    constexpr int EX5 = 15;// GPB7
  } 

  namespace statusLED {
    constexpr int LED_L0 = 7; // GPA7
    constexpr int LED_L1 = 6; // GPA6
    constexpr int LED_L2 = 5; // GPA5
    constexpr int LED_L3 = 4; // GPA4
    constexpr int LED_L4 = 3; // GPA3
    constexpr int LED_L5 = 2; // GPA2
    constexpr int LED_L6 = 1; // GPA1
    constexpr int LED_L7 = 0; // GPA0
  }

  namespace MT8870 {
    constexpr int Q1 = 8; // GPB0
    constexpr int Q2 = 7; // GPA7
    constexpr int Q3 = 6; // GPA6
    constexpr int Q4 = 5; // GPA5

    constexpr int STD = 11; // GPB3
    constexpr int PWDN = 10; // GPB2
  }

  namespace mt8816 {
    constexpr uint8_t RESET     = 0; // GPA0
    constexpr uint8_t DATA      = 1; // GPA1
    constexpr uint8_t STROBE    = 2; // GPA2
    constexpr uint8_t CS        = 3; // GPA3

    constexpr uint8_t ax_pins[4] = {8, 9, 10, 11}; // GPB0-GPB3
    constexpr uint8_t ay_pins[3] = {12, 13, 14}; // GPB4-GPB6
  }

}
