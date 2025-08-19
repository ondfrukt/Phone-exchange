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

    constexpr int TEST_BUTTON= 9; // GPB1

    constexpr int MCP_MAIN_INT_PIN = 18;
    constexpr int MCP_SLIC_INT_1_PIN = 11;
    constexpr int MCP_SLIC_INT_2_PIN = 14;

    constexpr uint8_t MCP_MAIN_ADDRESS = 0x20; //BEHÖVER ÄNDRAS!
    constexpr uint8_t MCP_MT8816_ADDRESS = 0x21; //BEHÖVER ÄNDRAS!
    constexpr uint8_t MCP_SLIC1_ADDRESS = 0x22; //BEHÖVER ÄNDRAS!
    constexpr uint8_t MCP_SLIC2_ADDRESS = 0x23; //BEHÖVER ÄNDRAS!

    struct PinModeEntry {
    uint8_t mode;   // INPUT, OUTPUT, INPUT_PULLUP
    bool    initial; // gäller bara om OUTPUT
    };

    inline constexpr PinModeEntry MCP_MAIN[16] = {
      /*0*/  {OUTPUT, LOW},   // GPA0  LED_L7
      /*1*/  {OUTPUT, LOW},   // GPA1  LED_L6
      /*2*/  {OUTPUT, LOW},   // GPA2  LED_L5
      /*3*/  {OUTPUT, LOW},   // GPA3  LED_L4
      /*4*/  {OUTPUT, LOW},   // GPA4  LED_L3
      /*5*/  {OUTPUT, LOW},   // GPA5  LED_L2
      /*6*/  {OUTPUT, LOW},   // GPA6  LED_L1
      /*7*/  {OUTPUT, LOW},   // GPA7  LED_L0
      /*8*/  {OUTPUT, LOW},   // GPB0  Not in use
      /*9*/  {INPUT_PULLUP,0},// GPB1  Test_button (aktiv låg mot GND)
      /*10*/ {OUTPUT, LOW},   // GPB2  PWDN_MT8870 (justera nivå vid behov)
      /*11*/ {INPUT, 0},      // GPB3  STD (status från MT8870)
      /*12*/ {INPUT, 0},      // GPB4  Q4 (MT8870)
      /*13*/ {INPUT, 0},      // GPB5  Q3 (MT8870)
      /*14*/ {INPUT, 0},      // GPB6  Q2 (MT8870)
      /*15*/ {INPUT, 0},      // GPB7  Q1 (MT8870)
    };

    inline constexpr PinModeEntry MCP_MT8816[16] = {
      /*0*/  {OUTPUT, LOW},  // GPA0 -> RESET (MT8816)
      /*1*/  {OUTPUT, LOW},  // GPA1 -> DATA
      /*2*/  {OUTPUT, LOW},  // GPA2 -> STROBE
      /*3*/  {OUTPUT, LOW},  // GPA3 -> CS
      /*4*/  {OUTPUT, LOW},  // GPA4 -> AX0
      /*5*/  {OUTPUT, LOW},  // GPA5 -> AX1
      /*6*/  {OUTPUT, LOW},  // GPA6 -> AX2
      /*7*/  {OUTPUT, LOW},  // GPA7 -> AY0
      /*8*/  {OUTPUT, LOW},  // GPB0 -> AY1
      /*9*/  {OUTPUT, LOW},  // GPB1 -> AY2
      /*10*/ {OUTPUT, LOW},  // GPB2 -> X0
      /*11*/ {OUTPUT, LOW},  // GPB3 -> EX1
      /*12*/ {OUTPUT, LOW},  // GPB4 -> EX2
      /*13*/ {OUTPUT, LOW},  // GPB5 -> EX3
      /*14*/ {OUTPUT, LOW},  // GPB6 -> EX4
      /*15*/ {OUTPUT, LOW},  // GPB7 Not in use
    };

    inline constexpr PinModeEntry MCP_SLIC[16] = {
      /*0*/ {OUTPUT, LOW},     // GPA0 Not in use
      /*1*/ {OUTPUT, LOW},     // GPA1 Not in use
      /*2*/ {OUTPUT, LOW},     // GPA2 FR_1/5
      /*3*/ {OUTPUT, LOW},     // GPA3 RM 1/5
      /*4*/ {INPUT, LOW},      // GPA4 SHK 1/5 
      /*5*/ {INPUT, LOW},      // GPA5 SHK 0/4
      /*6*/ {OUTPUT, LOW},     // GPA6 RM 0/4
      /*7*/ {OUTPUT, LOW},     // GPA7 FR 0/4 
      /*8*/ {INPUT, LOW},      // GPB0 SHK 2/6
      /*9*/ {OUTPUT, LOW},     // GPB1 RM 2/6
      /*10*/{OUTPUT, LOW},     // GPB2 FR 2/6
      /*11*/{INPUT, LOW},      // GPB3 SHK 3/7 
      /*12*/{OUTPUT, LOW},     // GPB4 RM 3/7
      /*13*/{OUTPUT, LOW},     // GPB5 FR 3/7  
      /*14*/{OUTPUT, LOW},     // GPB6 Not in use
      /*15*/{OUTPUT, LOW},     // GPB7 Not in use
    };

    constexpr uint8_t SHK_PINS[8] = {
      4,   // Line 0 & 4: MCP_SLIC1, GPA4
      5,   // Line 1 & 5: MCP_SLIC1, GPA5
      8,   // Line 2 & 6: MCP_SLIC1, GPB0
      11   // Line 3 & 7: MCP_SLIC1, GPB3
    };

    constexpr uint8_t FR_PINS[8] = {
      2,   // Line 0 & 4: MCP_SLIC1, GPA2
      0,   // Line 1 & 5: MCP_SLIC1, GPA0
      9,   // Line 2 & 6: MCP_SLIC1, GPB1
      12,  // Line 3 & 7: MCP_SLIC1, GPB4
    };

    constexpr uint8_t RM_PINS[8] = {
      3,   // Line 0 & 4: MCP_SLIC1, GPA3
      1,   // Line 1 & 5: MCP_SLIC1, GPA1
      10,  // Line 2 & 6: MCP_SLIC1, GPB2
      13,  // Line 3 & 7: MCP_SLIC1, GPB5
    };


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
