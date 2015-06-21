/*
 * $Id: patest_pink.c 354 2002-10-04 07:38:59Z  $
 patest_pink.c
 Generate Pink Noise using Gardner method.
 Optimization suggested by James McCartney uses a tree
 to select which random value to replace.
 x x x x x x x x x x x x x x x x 
  x   x   x   x   x   x   x   x   
    x       x       x       x       
     x               x               
       x                               
 Tree is generated by counting trailing zeros in an increasing index.
 When the index is zero, no random number is selected.
 *
 * Author: Phil Burk  http://www.softsynth.com
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <stdio.h>
#include <math.h>
#include "portaudio.h"
#define PINK_MAX_RANDOM_ROWS   (30)
#define PINK_RANDOM_BITS       (24)
#define PINK_RANDOM_SHIFT      ((sizeof(long)*8)-PINK_RANDOM_BITS)
typedef struct
{
    long      pink_Rows[PINK_MAX_RANDOM_ROWS];
    long      pink_RunningSum;   /* Used to optimize summing of generators. */
    int       pink_Index;        /* Incremented each sample. */
    int       pink_IndexMask;    /* Index wrapped by ANDing with this mask. */
    float     pink_Scalar;       /* Used to scale within range of -1.0 to +1.0 */
}
PinkNoise;
/* Prototypes */
static unsigned long GenerateRandomNumber( void );
void InitializePinkNoise( PinkNoise *pink, int numRows );
float GeneratePinkNoise( PinkNoise *pink );
/************************************************************/
/* Calculate pseudo-random 32 bit number based on linear congruential method. */
static unsigned long GenerateRandomNumber( void )
{
    /* Change this seed for different random sequences. */
    static unsigned long randSeed = 22222;
    randSeed = (randSeed * 196314165) + 907633515;
    return randSeed;
}
/************************************************************/
/* Setup PinkNoise structure for N rows of generators. */
void InitializePinkNoise( PinkNoise *pink, int numRows )
{
    int i;
    long pmax;
    pink->pink_Index = 0;
    pink->pink_IndexMask = (1<<numRows) - 1;
    /* Calculate maximum possible signed random value. Extra 1 for white noise always added. */
    pmax = (numRows + 1) * (1<<(PINK_RANDOM_BITS-1));
    pink->pink_Scalar = 1.0f / pmax;
    /* Initialize rows. */
    for( i=0; i<numRows; i++ ) pink->pink_Rows[i] = 0;
    pink->pink_RunningSum = 0;
}
#define PINK_MEASURE
#ifdef PINK_MEASURE
float pinkMax = -999.0;
float pinkMin =  999.0;
#endif
/* Generate Pink noise values between -1.0 and +1.0 */
float GeneratePinkNoise( PinkNoise *pink )
{
    long newRandom;
    long sum;
    float output;
    /* Increment and mask index. */
    pink->pink_Index = (pink->pink_Index + 1) & pink->pink_IndexMask;
    /* If index is zero, don't update any random values. */
    if( pink->pink_Index != 0 )
    {
        /* Determine how many trailing zeros in PinkIndex. */
        /* This algorithm will hang if n==0 so test first. */
        int numZeros = 0;
        int n = pink->pink_Index;
        while( (n & 1) == 0 )
        {
            n = n >> 1;
            numZeros++;
        }
        /* Replace the indexed ROWS random value.
         * Subtract and add back to RunningSum instead of adding all the random
         * values together. Only one changes each time.
         */
        pink->pink_RunningSum -= pink->pink_Rows[numZeros];
        newRandom = ((long)GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
        pink->pink_RunningSum += newRandom;
        pink->pink_Rows[numZeros] = newRandom;
    }

    /* Add extra white noise value. */
    newRandom = ((long)GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
    sum = pink->pink_RunningSum + newRandom;
    /* Scale to range of -1.0 to 0.9999. */
    output = pink->pink_Scalar * sum;
#ifdef PINK_MEASURE
    /* Check Min/Max */
    if( output > pinkMax ) pinkMax = output;
    else if( output < pinkMin ) pinkMin = output;
#endif
    return output;
}
/*******************************************************************/
#define PINK_TEST
#ifdef PINK_TEST
/* Context for callback routine. */
typedef struct
{
    PinkNoise   leftPink;
    PinkNoise   rightPink;
    unsigned int sampsToGo;
}
paTestData;
/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback( void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           PaTimestamp outTime, void *userData )
{
    int finished;
    int i;
    int numFrames;
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    (void) inputBuffer; /* Prevent "unused variable" warnings. */
    (void) outTime;

    /* Are we almost at end. */
    if( data->sampsToGo < framesPerBuffer )
    {
        numFrames = data->sampsToGo;
        finished = 1;
    }
    else
    {
        numFrames = framesPerBuffer;
        finished = 0;
    }
    for( i=0; i<numFrames; i++ )
    {
        *out++ = GeneratePinkNoise( &data->leftPink );
        *out++ = GeneratePinkNoise( &data->rightPink );
    }
    data->sampsToGo -= numFrames;
    return finished;
}
/*******************************************************************/
int main(void);
int main(void)
{
    PortAudioStream *stream;
    PaError err;
    paTestData data;
    int totalSamps;
    /* Initialize two pink noise signals with different numbers of rows. */
    InitializePinkNoise( &data.leftPink, 12 );
    InitializePinkNoise( &data.rightPink, 16 );
    /* Look at a few values. */
    {
        int i;
        float pink;
        for( i=0; i<20; i++ )
        {
            pink = GeneratePinkNoise( &data.leftPink );
            printf("Pink = %f\n", pink );
        }
    }
    data.sampsToGo = totalSamps = 8*44100; /* Play for a few seconds. */
    err = Pa_Initialize();
    if( err != paNoError ) goto error;
    /* Open a stereo PortAudio stream so we can hear the result. */
    err = Pa_OpenStream(
              &stream,
              paNoDevice,
              0,              /* no input */
              paFloat32,  /* 32 bit floating point input */
              NULL,
              Pa_GetDefaultOutputDeviceID(), /* default output device */
              2,          /* stereo output */
              paFloat32,      /* 32 bit floating point output */
              NULL,
              44100.,
              2048,           /* 46 msec buffers */
              0,              /* number of buffers, if zero then use default minimum */
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    printf("Waiting for sound to finish.\n");
    while( Pa_StreamActive( stream ) )
    {
        Pa_Sleep(100); /* SPIN! */
    }
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
#ifdef PINK_MEASURE
    printf("Pink min = %f, max = %f\n", pinkMin, pinkMax );
#endif
    Pa_Terminate();
    return 0;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return 0;
}
#endif /* PINK_TEST */
