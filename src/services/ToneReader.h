#pragma once
#include <Arduino.h>
#include "config.h"
#include "drivers/MCPDriver.h"
#include "services/LineManager.h"
#include "settings.h"

class ToneReader {
	public:
		ToneReader(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager);
		void update();

	private:
		MCPDriver& mcpDriver_;
		Settings& settings_;
		LineManager& lineManager_;

		const char dtmf_map[16] = {
		' ', 
		'1', '2', '3',
		'4', '5', '6', 
		'7', '8', '9',
		'0', '*', '#',
		};



};