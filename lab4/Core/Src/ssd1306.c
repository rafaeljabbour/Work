/**
 * original author:  Tilen Majerle<tilen@majerle.eu>
 * modification for STM32f10x: Alexander Lutsai<s.lyra@ya.ru>

   ----------------------------------------------------------------------
    Copyright (C) Alexander Lutsai, 2016
    Copyright (C) Tilen Majerle, 2015

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
 */
#include "ssd1306.h"

extern I2C_HandleTypeDef hi2c2;

uint16_t SSD1306_CurrentX = 0;
uint16_t SSD1306_CurrentY = 0;

/* SSD1306 data buffer. This is the buffer you must write to for setting pixel values. */
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

/* The set of registers to write to when initializing the SSD1306 screen. */
static uint8_t SSD1306_Init_Config[] = {0xAE, 0x20, 0x10, 0xB0, 0xC8, 0x00, 0x10, 0x40, 0x81, 0xFF,
                                        0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3, 0x00, 0xD5, 0xF0, 0xD9,
                                        0x22, 0xDA, 0x12, 0xDB, 0x20, 0x8D, 0x14, 0xAF, 0x2E};

/* An array you can use for the commands to be sent for scrolling. */
static uint8_t SSD1306_Scroll_Commands[8] = {0};

/* The following functions are provided for you to use, without requiring any modifications */

void SSD1306_UpdateScreen(void) {
    uint8_t m;
    uint8_t packet[3] = {0x00, 0x00, 0x10};

    for (m = 0; m < 8; m++) {
        packet[0] = (0xB0 + m);
        ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, packet, 3);
        ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x40, &SSD1306_Buffer[SSD1306_WIDTH * m], SSD1306_WIDTH);
    }
}

void SSD1306_Fill(SSD1306_COLOR_t color) {
    /* Use memset to efficiently set the entire SSD1306_Buffer to a single value. */
    memset(SSD1306_Buffer, (color == SSD1306_COLOR_BLACK) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

void SSD1306_Clear(void) {
    SSD1306_Fill(0);
    SSD1306_UpdateScreen();
}

/* Start of the functions you must complete for this lab. */
void ssd1306_I2C_Write(uint8_t address, uint8_t reg, uint8_t* data, uint16_t count) {
    /* TODO */
    // we have 1 byte for the reg, and then count bytes for the data
    uint8_t buffer[count + 1];
    // we set the first byte to reg, which is the control (0x00 for commands, 0x40 for data)
    buffer[0] = reg;
    // we copy the data into the buffer starting at index 1
    memcpy(&buffer[1], data, count);

    // we transmit the buffer over I2C to the given address
    if (HAL_I2C_Master_Transmit(&hi2c2, address, buffer, count + 1, 10) != HAL_OK) {
        return HAL_ERROR;
    }
    // HAL_I2C_Master_Transmit(/* TODO */, /* TODO */, /* TODO */, /* TODO */, 10);
}

HAL_StatusTypeDef SSD1306_Init(void) {
    /* Check if the OLED is connected to I2C */
    if (HAL_I2C_IsDeviceReady(&hi2c2, SSD1306_I2C_ADDR, 1, 20000) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Keep this delay to prevent overflowing the I2C controller */
    HAL_Delay(10);

    /* Init LCD */
    // ssd1306_I2C_Write(/* TODO */);
    ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, SSD1306_Init_Config, sizeof(SSD1306_Init_Config));

    /* we could also do it with a loop same thing
       for (int i = 0; i < sizeof(SSD1306_Init_Config); i++) {
           ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, &SSD1306_Init_Config[i], 1);
       }
    */

    /* Clear screen */
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    /* Update screen */
    SSD1306_UpdateScreen();

    /* Return OK */
    return HAL_OK;
}

HAL_StatusTypeDef SSD1306_DrawPixel(uint16_t x, uint16_t y, SSD1306_COLOR_t color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return HAL_ERROR;
    }

    /* Set the pixel at position (x,y) to 'color'. */
    if (color == SSD1306_COLOR_WHITE) {
        /* TODO */
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }

    return HAL_OK;
}

void SSD1306_Scroll(SSD1306_SCROLL_DIR_t direction, uint8_t start_row, uint8_t end_row) {
    /* TO DO */
    // we fill the command values using the table in the document
    SSD1306_Scroll_Commands[0] =
        (direction == SSD1306_SCROLL_RIGHT) ? SSD1306_RIGHT_HORIZONTAL_SCROLL : SSD1306_LEFT_HORIZONTAL_SCROLL;
    SSD1306_Scroll_Commands[1] = 0x00;
    SSD1306_Scroll_Commands[2] = start_row;
    SSD1306_Scroll_Commands[3] = 0x00;
    SSD1306_Scroll_Commands[4] = end_row;
    SSD1306_Scroll_Commands[5] = 0x00;
    SSD1306_Scroll_Commands[6] = 0xFF;
    SSD1306_Scroll_Commands[7] = SSD1306_ACTIVATE_SCROLL;

    // we write the commands to the screen to start the scrolling
    ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, SSD1306_Scroll_Commands, 8);
}

void SSD1306_Stopscroll(void) {
    /* TO DO */
    uint8_t cmd = SSD1306_DEACTIVATE_SCROLL;
    ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, &cmd, 1);
}

void SSD1306_Putc(uint16_t x, uint16_t y, char ch, FontDef_t* Font) {
    /* TODO */
    // we find the character we want to display in the font array
    uint32_t index = (ch - 32) * Font->FontHeight;

    for (uint16_t row = 0; row < Font->FontHeight; row++) {
        // we extract one row of the character array
        uint16_t bitmapRow = Font->data[index + row];

        for (uint16_t col = 0; col < Font->FontWidth; col++) {
            // we check if this pixel is set
            if (bitmapRow & (1 << (15 - col))) {
                SSD1306_DrawPixel(x + col, y + row, SSD1306_COLOR_WHITE);
            }
        }
    }
}

HAL_StatusTypeDef SSD1306_Puts(char* str, FontDef_t* Font) {
    /* Loop over every character until we see \0. */
    while (*str != '\0') {
        /* TODO */
        if (*str == '\n') {
            // we reset to the left edge
            SSD1306_CurrentX = 0;
            // we move down one line
            SSD1306_CurrentY += Font->FontHeight;
        } else {
            // we write the char
            SSD1306_Putc(SSD1306_CurrentX, SSD1306_CurrentY, *str, Font);
            // we move the cursor to the right of the char
            SSD1306_CurrentX += Font->FontWidth;
        }
        /* Increase string pointer */
        str++;
    }
    return HAL_OK;
}
