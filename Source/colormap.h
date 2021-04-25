/*
  ==============================================================================

    colormap.h
    Created: 23 Mar 2021 10:41:08pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once

namespace colormap{

typedef struct {
    double r,g,b;
} COLOUR;

COLOUR GetColour(double v,double vmin,double vmax);
double interpolate( double val, double y0, double x0, double y1, double x1 );
double base( double val );
double red( double gray );
double green( double gray );
double blue( double gray );

}
