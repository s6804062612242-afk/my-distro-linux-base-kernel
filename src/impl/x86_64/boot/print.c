#include "intf/print.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xb8000;

static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint8_t terminal_color = 0;

static uint8_t vga_entry_color(uint8_t foreground, uint8_t background) {
    return foreground | (background << 4);
}

static uint16_t vga_entry(unsigned char character, uint8_t color) {
    return (uint16_t) character | ((uint16_t) color << 8);
}

void print_set_color(uint8_t foreground, uint8_t background) {
    terminal_color = vga_entry_color(foreground, background);
}

void print_clear(void) {
    size_t y = 0;
    while (y < VGA_HEIGHT) {
        size_t x = 0;
        while (x < VGA_WIDTH) {
            const size_t index = y * VGA_WIDTH + x;
            VGA_MEMORY[index] = vga_entry(' ', terminal_color);
            x++;
        }
        y++;
    }

    terminal_row = 0;
    terminal_column = 0;
}

void print_char(char character) {
    if (character == '\n') {
        terminal_column = 0;
        terminal_row++;
        if (terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
        return;
    }

    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    VGA_MEMORY[index] = vga_entry((unsigned char) character, terminal_color);

    terminal_column++;
    if (terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        terminal_row++;
        if (terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
    }
}

void print_str(const char* string) {
    size_t i = 0;
    while (string[i] != '\0') {
        print_char(string[i]);
        i++;
    }
}
