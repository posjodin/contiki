#define MBUS_FRAME_TYPE_ANY     0
#define MBUS_FRAME_TYPE_ACK     1
#define MBUS_FRAME_TYPE_SHORT   2
#define MBUS_FRAME_TYPE_CONTROL 3
#define MBUS_FRAME_TYPE_LONG    4

#define MBUS_FRAME_BASE_SIZE_ACK       1
#define MBUS_FRAME_BASE_SIZE_SHORT     5
#define MBUS_FRAME_BASE_SIZE_CONTROL   9
#define MBUS_FRAME_BASE_SIZE_LONG      9

#define MBUS_FRAME_FIXED_SIZE_ACK      1
#define MBUS_FRAME_FIXED_SIZE_SHORT    5
#define MBUS_FRAME_FIXED_SIZE_CONTROL  6
#define MBUS_FRAME_FIXED_SIZE_LONG     6

#define MBUS_FRAME_ACK_START     0xE5
#define MBUS_FRAME_SHORT_START   0x10
#define MBUS_FRAME_CONTROL_START 0x68
#define MBUS_FRAME_LONG_START    0x68
#define MBUS_FRAME_STOP          0x16

#define MBUS_FRAME_DATA_LENGTH 252

typedef struct _mbus_frame {

    uint8_t start1;
    uint8_t length1;
    uint8_t length2;
    uint8_t start2;
    uint8_t control;
    uint8_t address;
    uint8_t control_information;
    uint8_t checksum;
    uint8_t stop;

    uint8_t   data[MBUS_FRAME_DATA_LENGTH];
    size_t data_size;

    int type;

    void *next; // pointer to next mbus_frame for multi-telegram replies

} mbus_frame;
