/*
 * Copyright (C) Three Laws of Mobility Inc.
 */

#ifndef _RESPONSECODE_H
#define _RESPONSECODE_H

class ResponseCode {
public:
    // 100 series - Requestion action was initiated; expect another reply
    // before proceeding with a new command.
    static const int ActionInitiated          = 100;

    // 200 series - Requested action has been successfully completed
    static const int CommandOkay              = 200;

    // 400 series - The command was accepted but the requested action
    // did not take place.
    static const int OperationFailed          = 400;

    // 500 series - The command was not accepted and the requested
    // action did not take place.
    static const int CommandSyntaxError       = 500;

    // 600 series - Unsolicited broadcasts
    static const int UnsolicitedInformational = 600;
};
#endif
