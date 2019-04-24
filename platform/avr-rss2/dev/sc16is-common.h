#define GP0 1
#define GP1 2
#define GP2 4
#define GP3 8
#define GP4 16
#define GP5 32
#define GP6 64
#define GP7 128

/* Pin assignments for GPRS RS/KTH board */

#define G_RESET   GP0
#define G_GPIO1   GP1
#define G_PWR     GP2
#define G_U_5V_CTRL GP3
#define G_SET     GP4
#define G_LED_RED GP5
#define G_LED_YELLOW GP6
#define G_GPIO7   GP7

void set_bit(uint8_t *s, uint8_t bit);
void clr_bit(uint8_t *s, uint8_t bit);
void toggle_bit(uint8_t *s, uint8_t bit);
void set_board_5v(uint8_t on);

