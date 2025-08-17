#pragma once
#include "LineHandler.h"

class LineAction {
public:
    static void SetLineStatus(LineHandler& line);
    static void SetNewPhoneNumber(LineHandler& line, const String& newNumber);

    
    static void processDigit(LineHandler& line, char digit);
    static void onIncomingCall(LineHandler& line, int fromLine);
    static void onOutgoingCall(LineHandler& line, int toLine);
};
