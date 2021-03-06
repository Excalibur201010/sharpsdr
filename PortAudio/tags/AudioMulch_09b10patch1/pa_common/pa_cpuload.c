/*
 * $Id: pa_cpuload.c 298 2002-07-02 01:24:09Z rossbencina $
 * Portable Audio I/O Library CPU Load measurement functions
 * Portable CPU load measurement facility.
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 2002 Ross Bencina
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
 */

#include "pa_cpuload.h"

#include <assert.h>

#include "pa_util.h"   /* for PaUtil_MicrosecondTime() */


void PaUtil_InitializeCpuLoadMeasurer( PaUtilCpuLoadMeasurer* measurer, double sampleRate )
{
    assert( sampleRate > 0 );

    measurer->samplingPeriod = 1. / sampleRate;
}


void PaUtil_BeginCpuLoadMeasurement( PaUtilCpuLoadMeasurer* measurer )
{
    measurer->measurementStartTime = PaUtil_GetTime();
}


void PaUtil_EndCpuLoadMeasurement( PaUtilCpuLoadMeasurer* measurer, unsigned long framesProcessed )
{
    double measurementEndTime, secondsFor100Percent, measuredLoad;

    if( framesProcessed > 0 ){
        measurementEndTime = PaUtil_GetTime();

        assert( framesProcessed > 0 );
        secondsFor100Percent = framesProcessed * measurer->samplingPeriod;

        measuredLoad = (measurementEndTime - measurer->measurementStartTime) / secondsFor100Percent;

        /* Low pass filter the calculated CPU load to reduce jitter using a simple IIR low pass filter. */
#define LOWPASS_COEFFICIENT_0   (0.9)
#define LOWPASS_COEFFICIENT_1   (0.99999 - LOWPASS_COEFFICIENT_0)

        measurer->averageLoad = (LOWPASS_COEFFICIENT_0 * measurer->averageLoad) +
                               (LOWPASS_COEFFICIENT_1 * measuredLoad);
    }
}


double PaUtil_GetCpuLoad( PaUtilCpuLoadMeasurer* measurer )
{
    return measurer->averageLoad;
}
