#include "app/App.h"
using namespace cfg;

void App::begin() {

    Wire.begin(i2c::SDA_PIN, i2c::SCL_PIN);
    mcpDriver_.begin();

}
void App::loop() {

    for ()


}