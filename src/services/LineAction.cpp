#pragma once
#include "LineAction.h"


LineAction::LineAction(LineManager& lineManager, Settings& settings)
: lineManager_(lineManager), settings_(settings) {
  // ev. startlogik
};

void LineAction::update() {

  if(lineManager_.lineChangeFlag != 0){

  uint8_t changes = lineManager_.lineChangeFlag & settings_.activeLinesMask;
  for (int idx = 0; idx < 8; ++idx)
      if (changes & (1 << idx)) {
        lineManager_.clearChangeFlag(idx); // Rensa flaggan direkt
        action(idx);
    } 
  }
}

void LineAction::action(int index) {
  using namespace model;
  LineHandler& line = lineManager_.getLine(index);
  LineStatus newStatus = line.currentLineStatus;
  LineStatus previousStatus = line.previousLineStatus;
  
  // ev. privat metod
}

