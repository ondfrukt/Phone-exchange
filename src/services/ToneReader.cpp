#include "services/ToneReader.h"


ToneReader::ToneReader(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager)
		: mcpDriver_(mcpDriver), settings_(settings), lineManager_(lineManager) {}

void ToneReader::update() {

	// MT8816 is not present, nothing to do
	if (!mcpDriver_.haveMT8816_) return;
	// Checks if there are any active lines that are not idle
	if ((settings_.activeLinesMask & lineManager_.linesNotIdle) == 0) {
		// Power off MT8816 if it is on
		if (mcpDriver_.mt8816Powered_) {
				mcpDriver_.mt8816PowerControl(false);
			}
		return;
	}

	// Power on MT8816 if it is off
	if (!mcpDriver_.mt8816Powered_) mcpDriver_.mt8816PowerControl(true);


}
