#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

typedef struct {
    uint8_t type;  // 0=placement, 1=shot, 2=incoming shot, 3=game over
    uint8_t x;
    uint8_t y;
    uint8_t hit;  // Used for hit/miss and ship orientation
} Message;

#endif