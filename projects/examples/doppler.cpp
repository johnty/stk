// doppler.cpp MUMT618 Assignemt 3
// 30 Sep 2015
// Johnty Wang (johnty.wang@mail.mcgill.ca)

#include "FileWvIn.h"
#include "FileLoop.h"
#include "FileWvOut.h"
#include "DelayL.h"
#include <cstdlib>

double SAMPLE_RATE = 44100.0;
unsigned int DELAY_SIZE = 1024;
double SPEED_OF_SOUND = 343.0;

using namespace stk;

int main(int argc, char *argv[])
{
  double velSource;
  double velReciver;
  double dist;
  double velRelative;
  double totalTime;
  unsigned long numSamples;
  
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
    velSource = atof(argv[1]);
    velReciver = atof(argv[2]);
    dist = atof(argv[3]);
    std::cout<<"velSource=" << velSource << " velReciver=" << velReciver << " dist=" << dist << " if=" << argv[4] << "\n\n";
    
    velRelative = velSource+velReciver;
    totalTime = dist/velRelative;
    //do some basic input checking:
    if ((velSource + velReciver) <= 0) {
      std::cout<<"clamping time to 5 seconds";
      totalTime = 5.0;
    }
    numSamples = totalTime * SAMPLE_RATE;
    std::cout<<"relative speed = " << velRelative<<"; simulation time = " << totalTime<< "s; numSamples = "<< numSamples<<"\n";
  }
  // Set the global sample rate before creating class instances.
  Stk::setSampleRate( SAMPLE_RATE );

  int i;
  FileWvOut outputFile;
  FileLoop inputLoop;

  // Define and open a 16-bit, single-channel WAV
  try {
	  outputFile.openFile( "doppler_out.wav", 1, FileWrite::FILE_WAV, Stk::STK_SINT16 );
    inputLoop.openFile(argv[4]);
    std::cout<<"opened input file samples = " << inputLoop.getSize() <<"\n";
  }
  catch (StkError &) {
    exit( 1 );
  }
  
  //the doppler effect's frequency shift is implemented using a delay line that is written incrementally
  // and read using a variable delay.
  // Since the read pointer might catch up/wrap around the write pointer,
  // We need to do some averaging between the current read position and half-delayline-length position
  // to do so we can tap the delay at 1/2 the length
  // and keep track of a "mixing" variable that goes towards zero as the read ptr approaches the write ptr
  DelayL delay(1.0, DELAY_SIZE);
  

  //initialize: since read pointer can increment faster than write pointer, we should
  // fill up the delay line with initial values for continuity's sake...
  
  // since our input is a loop, it doesn't matter too much where the "starting" point
  // in the audio sample is...
  
  for (int i=0; i<DELAY_SIZE; i++) {
    delay.tick(inputLoop.tick());
  }
  
  stk::StkFrames frames( numSamples, 1);
  //go through the buffer sample by sample
  double read_ptr = 0;
  unsigned int write_ptr = 0;
  //numSamples = 100;
  double g=0;
  int numWrap = 0;
  for (int i=0; i<numSamples; i++) {
    //0.) write current value into delay line
    
    //get current relative speed, conver into delay amount and update delay line
    //note: we have a small truncation error for the mixing value here
    if (i < numSamples/2)
      g = - velRelative/SPEED_OF_SOUND; // "growth parameter"
    else
      g = + velRelative/SPEED_OF_SOUND;
    
    read_ptr+=g;

    //wraparound checks:
    if (read_ptr < 0)
      read_ptr+=DELAY_SIZE;
    if (read_ptr>DELAY_SIZE-1)
      read_ptr-=DELAY_SIZE;
    
    //write pointer simply increments...
    if (write_ptr>DELAY_SIZE)
      write_ptr-=DELAY_SIZE;

    write_ptr++;
    
    delay.setDelay(1+read_ptr);
    double mix = ( (int)(read_ptr-write_ptr) % (DELAY_SIZE) ) / (double) DELAY_SIZE;
    mix = 1;
    std::cout << g << std::endl;
    float outsample = mix*delay.tick(inputLoop.tick());
    //outsample += (1-mix)*delay.tapOut(DELAY_SIZE/2);
    frames[i] = outsample;
  }
  std::cout<<"numWrap = " << numWrap<<"\n";

  //dump all frames into outputFile file. (could have incrementally done it directly above...)
  outputFile.tick( frames );



  return 0;
}
