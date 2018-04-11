/*
 * Example program to illustrate the use of the ECE 486 interface.
 * 
 * An input waveform is copied to the output DAC.  The waveform is also
 * squared and streamed to the second DAC output.  Each
 * USER button press inverts the signal on the original DAC.
 * 
 * The use of printf(), the pushbutton, and the LCD is also illustrated.
 */

#include "stm32l4xx_hal.h"
#include "stm32l476g_discovery.h"

#include "ece486.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// 8 MHz / 44.1 ksps.
// See /usr/local/stmdev/include/init486.h for examples.
#define FS_44K1 1814

extern FlagStatus KeyPressed;   // Use to detect button presses

float *notes;

int main(void) {

    char lcd_str[8];

    int blocksize;
    float *input,
          *output1,
          *output2;
    int button_count = 0;
    int i, j;
    int state = 0;
    int first,
        third,
        fifth;

    initialize_ece486(FS_44K1, MONO_IN, STEREO_OUT, MSI_INTERNAL_RC);

    blocksize = getblocksize();

    input   = malloc(blocksize * sizeof(float));
    output1 = malloc(blocksize * sizeof(float));
    output2 = malloc(blocksize * sizeof(float));
    notes   = malloc(12*blocksize*sizeof(float));

    if (input   == NULL ||
        output1 == NULL ||
        output2 == NULL ||
        notes   == NULL) {
        flagerror(MEMORY_ALLOCATION_ERROR);
        while(1);
    }

    // Initialize the notes array.

    for (i = 0; i < 12; i++) {
        for (j = 0; j < blocksize; j++) {

            // Let A4 be 440 Hz.
            // A4 is index 9.
            // https://pages.mtu.edu/~suits/NoteFreqCalcs.html
            // sin(w*t) = sin(2*pi*f*t) = sin(2*pi*f*n*ts) = sin(2*pi*f*n/fs)

            static const float a = powf(2.0f, 1.0f/12);

            notes[i*blocksize + j] = sinf(2.0f*M_PI * j/44.1e3f * 440.0f*powf(a, i - 9));

            // TODO What's the desirable voltage range here?
            //      Transforming to 0-1.
            notes[i*blocksize + j] = (notes[i*blocksize + j] + 1.0f) / 2.0f;

        }
    }

    while(1) {

        getblock(input);	

        DIGITAL_IO_SET(); // Use a scope on PD0 to measure execution time.

        switch (state) {
        
        case 0:
            first = 0;
            third = 2;
            fifth = 5;
            break;

        case 1:
            first = 2;
            third = 5;
            fifth = 9;
            break;

        case 2:
            first = 4;
            third = 7:
            fifth = 11;
            break;

        case 3:
            first = 2;
            third = 5;
            fifth = 9;
            break;

        }

        for (i = 0; i < blocksize; i++) {

            output1[i] = notes[first*blocksize + i]
                       + notes[third*blocksize + i]
                       + notes[fifth*blocksize + i];
            output1[i] /= 3.0f;

        }

        state = (state + 1) % 4;

        DIGITAL_IO_RESET();

        putblockstereo(output1, output2);

        if (KeyPressed) {
            KeyPressed = RESET;

            /*
             * On each press, modify the LCD display, and toggle an LED
             * (LED4=red, LED5=green) (Red is used to show error conditions)
             * 
             * Don't be surprised when these cause a Sample Overrun error, 
             * depending on your sample rate.
             */
            button_count++;
            sprintf(lcd_str, "BTN %2d", button_count);
            BSP_LCD_GLASS_DisplayString( (uint8_t *)lcd_str);
            BSP_LED_Toggle(LED5);
        }
    }
}
