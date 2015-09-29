// foursine.cpp STK tutorial program

#include "SineWave.h"
#include "FileWvOut.h"
#include <cstdlib>

using namespace stk;

int main(int argc, char *argv[])
{
  if ( argc < 5 || argc > 6 ) {
    std::cout<<"\n\n\n****Doppler shift example*****\n\n";
    std::cout<<"Will produce output assuming source is travelling towards\n";
    std::cout<<"and then past receiver in straight line\n\n";
    std::cout<<"Will start simulation at d0=-D and end at d1=+D\n";
    std::cout<<"Simulation time = 2D/(relative velocity)\n";
    std::cout<<"input will be looped for duration of simulation\n\n";
    std::cout<< "usage: doppler [spd src (m/s)] [spd recv (m/s)] [dist (m)] [input file (wav)]\n";
    std::cout<< "NOTE1: for speeds, + is towards, - is away from\n";
    std::cout<< "NOTE2: if source and receiver are diverging, fix simulation duration to 5s\n";
    return 0;
  }
  else {
    std::cout<<"INPUT OK...\n parsed values:";
    double v_src = atof(argv[1]);
    double v_rcv = atof(argv[2]);
    double dist = atof(argv[3]);
    std::cout<<"v_src=" << v_src << " v_rcv=" << v_rcv << " dist=" << dist << " if=" << argv[4] << "\n\n";
    
    //do some basic input checking:
    if ((v_src + v_rcv) <= 0) {
      
    }
    std::cout<<"";
    return 0;
  }
  // Set the global sample rate before creating class instances.
  Stk::setSampleRate( 44100.0 );

  int i;
  FileWvOut output;
  SineWave inputs[4];

  // Set the sine wave frequencies.
  for ( i=0; i<4; i++ )
    inputs[i].setFrequency( 220.0 * (i+1) );

  // Define and open a 16-bit, four-channel AIFF formatted output file
  try {
	  output.openFile( "doppler_out.wav", 4, FileWrite::FILE_WAV, Stk::STK_SINT16 );
  }
  catch (StkError &) {
    exit( 1 );
  }

  // Write two seconds of four sines to the output file
  StkFrames frames( 88200, 4 );
  for ( i=0; i<4; i++ )
    inputs[i].tick( frames, i );

  output.tick( frames );

  // Now write the first sine to all four channels for two seconds
  for ( i=0; i<88200; i++ ) {
    output.tick( inputs[0].tick() );
  }

  return 0;
}
