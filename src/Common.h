#pragma once

#ifndef _STD
#define _STD ::std::
#endif

#define MSG_KIND_START (short)(0xDEAD - 0xBEEF)
#define MSG_KIND_PACKET (short)(MSG_KIND_START + 1)
#define MSG_KIND_TIMER (short)(MSG_KIND_START + 2)
#define MSG_KIND_SCHEDULED (short)(MSG_KIND_START + 3)

#define PARAM_WINDOW_SIZE "WS"
#define PARAM_TIMEOUT "TO"
#define PARAM_PROCESSING_TIME "PT"
#define PARAM_TRANSMISSION_DELAY "TD"
#define PARAM_ERROR_DELAY "ED"
#define PARAM_DUPLICATION_DELAY "DD"
#define PARAM_LOSS_RATE "LP"
#define PARAM_BASE_PREDICTOR "BasePred"

#define FRAME_TYPE_NACK 0
#define FRAME_TYPE_ACK 1
#define FRAME_TYPE_DATA 2
