#include "app/App.h"

void App::begin() {

    // Initialize MCP driver
    // This will set up the MCP23X17 devices with their respective I2C addresses
    mcpDriver_.begin();



    

}
void App::loop() {
    // Main application loop
    // Handle events, update states, etc.
}