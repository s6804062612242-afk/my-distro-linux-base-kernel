#include "intf/print.h"

void kernal_main(void) {
    print_clear();
    print_set_color(PRINT_COLOR_LIGHT_GREEN, PRINT_COLOR_BLACK);
    print_str("welcome to best os!!");
}
