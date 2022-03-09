#pragma once

#define log(fmt, ...) printf("[%s:%d] " fmt, __FILE__, __LINE__, ## __VA_ARGS__)

#define EXCHANGE_DEBUG
#ifdef EXCHANGE_DEBUG
    #define ex_debug log
#else
    #define ex_debug(fmt, ...) do {} while (0)
#endif