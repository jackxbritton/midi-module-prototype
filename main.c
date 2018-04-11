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
#include <assert.h>

// 8 MHz / 44.1 ksps.
// See /usr/local/stmdev/include/init486.h for examples.
#define FS_44K1 1814

typedef struct {
    float *buffer;
    int blocksize;
} NoteContext;

typedef struct {
    int half_steps, // From C.
        octave;     // From A1 = 55Hz.
} Note;

int note_context_init(NoteContext *nc, int blocksize) {

    int i, j;

    nc->buffer = malloc(12*blocksize*sizeof(float));
    if (nc->buffer == NULL) return 0;

    for (i = 0; i < 12; i++) {
        for (j = 0; j < blocksize; j++) {

            // Let A be 55 Hz (A1).
            // A is index 9.
            // https://pages.mtu.edu/~suits/NoteFreqCalcs.html
            // sin(w*t) = sin(2*pi*f*t) = sin(2*pi*f*n*ts) = sin(2*pi*f*n/fs)

            const float a = powf(2.0f, 1.0f/12);

            nc->buffer[i*blocksize + j] = sinf(2.0f*M_PI * j/44.1e3f * 1660.0f*powf(a, i - 9));

            nc->buffer[i*blocksize + j] += 1.0f; // TODO What's the desirable voltage range here?

        }
    }

    nc->blocksize = blocksize;

    return 1;
}

void note_context_destroy(NoteContext *nc) {
    free(nc->buffer);
}

void note_context_sample_note(const NoteContext *nc, const Note *note, float *out) {

    assert(note->half_steps >= 0 && note->half_steps < 12);

    const float sampling_factor = powf(2.0f, note->octave);
    int i;

    for (i = 0; i < nc->blocksize; i++) {
        out[i] = nc->buffer[note->half_steps*nc->blocksize + ((int) (i*sampling_factor) % nc->blocksize)];
    }

}

void note_context_sample_notes(const NoteContext *nc, const Note *notes, int notes_len, float *out) {

    //const float sampling_factor = powf(2.0f, note->octave);

    float sampling_factor;
    int i, j;

    // Zero the buffer.
    for (i = 0; i < nc->blocksize; i++) out[i] = 0.0f;

    // For each note..
    for (i = 0; i < notes_len; i++) {

        assert(notes[i].half_steps >= 0 && notes[i].half_steps < 12);

        sampling_factor = powf(2.0f, notes[i].octave);

        // Generate the sample..
        for (j = 0; j < nc->blocksize; j++) {
            out[i] += nc->buffer[notes[i].half_steps*nc->blocksize + ((int) (i*sampling_factor) % nc->blocksize)];
        }

    }

    // Normalize the magnitude.
    for (i = 0; i < nc->blocksize; i++) out[i] /= notes_len;

}

extern FlagStatus KeyPressed;   // Use to detect button presses

int main(void) {

    char lcd_str[8];

    int blocksize;
    float *input,
          *output1,
          *output2;

    int state = 0;
    int i;
    int block_count = 0;

    NoteContext nc;

    //const int scale_diatonic[7] = {
    //    0, 2, 4, 5, 7, 9, 11
    //};

    const Note notes[32] = {
        { 0,-5 },
        {10, 1 },
        { 0,-5 },
        {10, 1 },
        { 4, 0 },
        { 4, 0 },
        {10,-3 },
        { 2,-2 },

        { 0,-5 },
        { 2, 1 },
        { 4, 0 },
        { 2, 5 },
        { 2,-2 },
        { 0,-5 },
        { 2, 1 },
        { 4, 0 },

        { 3,-5 },
        { 7, 0 },
        { 5, 5 },
        { 5,-2 },
        { 5, 1 },
        { 3,-5 },
        { 5, 1 },
        { 7, 0 },

        {11,-6 },
        { 1, 1 },
        {11,-6 },
        { 1, 1 },
        { 3, 8 },
        { 3, 8 },
        { 1, 5 },
        { 1,-2 }
    };

    const Note chords[8][3] = {
        { { 0,-5 }, { 3,-5 }, { 7,-5 } },
        { { 0,-5 }, { 3,-5 }, { 7,-5 } },
        { { 2, 1 }, { 6, 1 }, { 9, 1 } },
        { { 2, 1 }, { 6, 1 }, { 9, 1 } },
        { { 4, 0 }, { 7, 0 }, {11, 0 } },
        { { 2, 5 }, { 6, 5 }, { 9, 5 } },
        { { 2,-2 }, { 6,-2 }, { 9,-2 } }
    };

    initialize_ece486(FS_44K1, MONO_IN, STEREO_OUT, MSI_INTERNAL_RC);

    blocksize = getblocksize();

    input   = malloc(blocksize * sizeof(float));
    output1 = malloc(blocksize * sizeof(float));
    output2 = malloc(blocksize * sizeof(float));

    if (input   == NULL ||
        output1 == NULL ||
        output2 == NULL) {
        flagerror(MEMORY_ALLOCATION_ERROR);
        while(1);
    }

    note_context_init(&nc, blocksize);

    while(1) {

        getblock(input);	

        DIGITAL_IO_SET(); // Use a scope on PD0 to measure execution time.

        for (i = 0; i < blocksize; i++) {

            //output1[i] = nc.buffer[chords[state][0]*blocksize + i]
            //           + nc.buffer[chords[state][1]*blocksize + i]
            //           + nc.buffer[chords[state][2]*blocksize + i];
            //output1[i] /= 3;

            note_context_sample_note(&nc, &notes[state], output1);
            //note_context_sample_notes(&nc, chords[state], 3, output1);

        }

        DIGITAL_IO_RESET();

        block_count++;

        state = ((int) (blocksize*block_count / 44.1e3f * 32.0f)) % 32;

        sprintf(lcd_str, "%2d", state);
        BSP_LCD_GLASS_DisplayString( (uint8_t *)lcd_str);

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
            //button_count++;
            //sprintf(lcd_str, "BTN %2d", button_count);
            //BSP_LCD_GLASS_DisplayString( (uint8_t *)lcd_str);
            //BSP_LED_Toggle(LED5);
        }
    }
}
