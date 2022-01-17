#ifndef QUEUE_MESSAGE_H_
#define QUEUE_MESSAGE_H_

#define KEY_ROLL_OVER 6

namespace fex
{
    enum class MessageType 
    {
        PRESS,
        RELEASE,
        DELAY,
        LAYER_SWITCH,
        MOUSE_MOVE_LEFT_RIGHT,
        MOUSE_MOVE_UP_DOWN,
        MOUSE_SCROLL_LEFT_RIGHT,
        MOUSE_SCROLL_UP_DOWN,
        MOUSE_CLICK,
        MOUSE_RELEASE,
        REBOOT,
        REBOOT_BOOTLOADER,
    };

    // Queue for key presses:
    //  - keyboard task polls for presses and queues them
    //  - usb task takes queue and enacts it
    typedef struct QueueMessage
    {
        MessageType type;
        unsigned char codes[KEY_ROLL_OVER];
        unsigned char length;
        unsigned long delay;
        int layer;
        int8_t mouse_delta;
        uint8_t mouse_click;
    } QueueMessage;

    typedef struct KeyMessage
    {
        uint8_t keys[10];
        uint32_t time;
    } KeyMessage;
}

#endif