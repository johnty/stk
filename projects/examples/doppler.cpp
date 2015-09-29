// doppler.cpp
// MUMT618 Assignemt 3
// 30 Sep 2015
// Johnty Wang (johnty.wang@mail.mcgill.ca)

// Implements a simple 1D doppler effect for a given input .wav file as well as a few input parameters
// that are described if the application is executed without any arguments.

//***** To compile: add following to stk/projects/examples/MakeFile.in
//      and then run root ./configure followed by "make doppler" in examples folder.
//
//doppler: doppler.cpp Stk.o DelayL.o FileRead.o FileWvIn.o FileWvOut.o FileWrite.o FileLoop.o
//  $(CC) $(LDFLAGS) $(CFLAGS) $(DEFS) -o doppler doppler.cpp $(OBJECT_PATH)/Stk.o $(OBJECT_PATH)/DelayL.o $(OBJECT_PATH)/FileRead.o $(OBJECT_PATH)/FileWvIn.o $(OBJECT_PATH)/FileWrite.o $(OBJECT_PATH)/FileWvOut.o $(OBJECT_PATH)/FileLoop.o $(OBJECT_PATH)/RtAudio.o $(LIBRARY)
//
//*****

//
// Usage: ./doppler [spd src (m/s)] [spd recv (m/s)] [distX (m)] [distY (m)] [input file (wav)] [output file (wav)] [mix]
//
//    - spd src/recv: speed of source and receivers, in m/s. + if towards the other.
//
//    - dist: initial distance between the two. distY makes the relative speeds change as they objects get close to each other
//             (otherwise we simply get a quick jump between two constant frequencies...)
//
//    - input file: input file to be processed. will be looped continuously throughout duration of simulation
//
//    - output file: output file to be written
//
//    - mix: whether to do "half delay length" mixing or not, for smoothing discontinuity when read pointer
//      overtakes the write pointer.
//
//
// Sample input:
// ./doppler 10 -15 100 10 rawwaves/siren.wav siren_doppler.wav 0
// will create a source and receiver travelling at 10m/s and -15m/s respectively
// from a distance of distX=120m and distY of 10m. The simulation will end when they are dixtX=120m apart after crossing each other.
// in our internal model, we set the starting location of the source as 0, and receiver in the positive direction
//
//
//    SRC -> (10m/s)
//    |                                              |
//    |                                              |
//    |                                              |
//    |                                              | (distY)
//    |                                              |
//    |                                              |
//     ----------------- 120m -----------------------
//                       (distX)           (5m/s) <- RECEIVER
//               fig 1.
//
//
//                                                                 SRC -> (10m/s)
//                  |                                              |
//                  |                                              |
//                  |                                              |
//                  |                                              | (distY)
//                  |                                              |
//                  |                                              |
//                  ----------------- 120m -----------------------
//   (5m/s) <- RECEIVER                    (distX)
//
//               fig 2.
//
//
//   based on the above configuration, the simulation will take 2*distX/(speed of source + speed of receiver)
//   if the distance between the two are diverging (or constant) as an initial condition, we clamp simulation to 5 secs
//   since the crossover point will never be reached, and we should get no frequency shift.
//
//   Possible improvements:
//      - extend kinematic model to arbiturary directions
//

#include "FileWvIn.h"
#include "FileLoop.h"
#include "FileWvOut.h"
#include "DelayL.h"
#include <cstdlib>

double SAMPLE_RATE = 44100.0;
unsigned int DELAY_SIZE = 1024; //for setting the delay line length, used for tapping into the delay line for (optional) mixing
double SPEED_OF_SOUND = 343.0;

using namespace stk;

int main(int argc, char *argv[])
{
  double velSource;
  double velReciver;
  double distX;
  double distY;
  double velRelativeX;
  double totalTime;
  unsigned long numSamples;
  
  if ( argc < 7 || argc > 8 ) {
    std::cout<<"\n\n\n****Doppler shift example*****\n\n";
    std::cout<<"Will produce output assuming source is travelling towards\n";
    std::cout<<"and then past receiver in straight line\n\n";
    std::cout<<"Will start simulation at d0=-D and end at d1=+D\n";
    std::cout<<"Simulation time = 2D/(relative velocity)\n";
    std::cout<<"input will be looped for duration of simulation\n\n";
    std::cout<< "usage: doppler [spd src (m/s)] [spd recv (m/s)] [distX (m)] [distY (m)] [input file (wav)] [output file (wav)] [0/1: no mix/mix]\n";
    std::cout<< "NOTE: if source and receiver are diverging, fix simulation duration to 5s\n";
    return 0;
  }
  else {
    std::cout<<"INPUT OK...\n parsed values:";
    velSource = atof(argv[1]);
    velReciver = atof(argv[2]);
    distX = atof(argv[3]);
    distY = atof(argv[4]);
    //visual check to make sure input was parsed properly
    std::cout<<"velSource=" << velSource << " velReciver=" << velReciver << " distX=" << distX << " distY=" <<distY << " if=" << argv[5] << "\n\n";
    
    velRelativeX = velSource-velReciver;
    
    double distAbs = sqrt(distX*distX + distY*distY);
    
    totalTime = 2*distX/velRelativeX; //total simulation time, if not diverging/constant
    
    //do some basic input checking:
    if ((velSource - velReciver) <= 0) {
      //this is for the "not so interesting" case where
      //the source and receiver will never "cross"
      // will just have a boring, constant frequency...
      //so we force a fixed simulation time here.
      std::cout<<"DIVERGING! clamping time to 5 seconds\n";
      totalTime = 5.0;
    }
    numSamples = totalTime * SAMPLE_RATE;
    std::cout<<"relative speedX = " << velRelativeX<<"; simulation time = " << totalTime<< "s; numSamples = "<< numSamples<<"\n";
  }
  
  Stk::setSampleRate( SAMPLE_RATE );
  
  //file i/o
  FileWvOut outputFile;
  FileLoop inputLoop;

  try {
	  outputFile.openFile( argv[6], 1, FileWrite::FILE_WAV, Stk::STK_SINT16 );
    inputLoop.openFile(argv[5]);
  }
  catch (StkError &) {
    exit( 1 );
  }
  
  //the doppler effect's frequency shift is implemented using a delay line whose write
  // pointer is incremented sample by sample and read using a variable delay.
  DelayL delay(1.0, DELAY_SIZE);

  //initialize: since read pointer can increment faster than write pointer, we should
  // fill up the delay line with initial values for continuity's sake...
    // since our input is a loop, it doesn't matter too much where the "starting" point
  // in the audio sample is...
  for (int i=0; i<DELAY_SIZE; i++) {
    delay.tick(inputLoop.tick());
  }
  
  stk::StkFrames frames( numSamples, 1);
  double readPtr = 0;
  unsigned int writePtr = 0;
  double g=0;

  // set initial locations
  double xSrcCurr = 0, xSrcPrev = 0;
  double xRcvCurr = distX, xRcvPrev = distX;
  double dT = totalTime/numSamples;
  
  for (int i=0; i<numSamples; i++) {
    
    //basic kinematic model for estimating current relative speed:
    double timeElapsed = (double)totalTime*i/numSamples;
    xSrcCurr+=dT*velSource;
    xRcvCurr+=dT*velReciver;
    
    //distY is constant!
    
    //velRelative = ( sqrt((xSrcCurr-xSrcPrev)*(xSrcCurr-xSrcPrev)+distY*distY) +
    //               sqrt((xRcvPrev-xRcvCurr)*(-xRcvCurr)+distY*distY) ) / dT;
    
    //double velRelative = ( (xRcvCurr-xSrcCurr) - (xRcvPrev-xSrcPrev) ) / dT;
    

    //bit of a hack to get the right behaviour, as my quickly cobbled together model
    // was bit sloppy when it came to managing the sign
    double velRelative = -fabs( sqrt((xRcvCurr-xSrcCurr)*(xRcvCurr-xSrcCurr)+distY*distY)
                          - sqrt((xRcvPrev-xSrcPrev)*(xRcvPrev-xSrcPrev)+distY*distY) ) / dT;
    
    if (xRcvCurr < xSrcCurr)
      velRelative = -velRelative;
    
    //std::cout <<xSrcCurr<<"  "<<xRcvCurr<< " "<< velRelative <<"\n";

    xSrcPrev = xSrcCurr;
    xRcvPrev = xRcvCurr;
    
    //if (i < numSamples/2)
    //  g = - velRelative/SPEED_OF_SOUND; // "growth parameter"
    //else
    g = + velRelative/SPEED_OF_SOUND;
    readPtr+=g; //update read pointer with variable delay

    //wraparound checks:
    if (readPtr < 0)
      readPtr+=DELAY_SIZE;
    if (readPtr>DELAY_SIZE-1)
      readPtr-=DELAY_SIZE;
    
    //write pointer simply increments...
    writePtr++;
    if (writePtr>DELAY_SIZE)
      writePtr-=DELAY_SIZE;
    
    //where the magic happens: the variable delay creates the frequency shift
    delay.setDelay(1+readPtr);
    
    //User Selectable switching of the "Mixing": gets rid of discontinuities
    // when the read pointer crosses the write location by
    // linearly mixing between half a delay line away.
    // maximum amplitude is respected, but at the cost
    // of additional artifacts. better mixing techniques could reduce these...
    double mix = ( (int)(readPtr-writePtr) % (DELAY_SIZE) ) / (double) DELAY_SIZE ;
    if (atoi(argv[7]) == 0)
      mix = 1; // overrides the mixing calculation. i.e. no mixing.
    
    //update output sample
    float outsample = mix*delay.tick(inputLoop.tick());
    
    //apply mixing (if any)
    outsample += (1-mix)*delay.tapOut(DELAY_SIZE/2);

    //put into frames
    frames[i] = outsample;
    //std::cout<<"tE = " << timeElapsed<<"\n";
  }
  

  //dump frames into outputFile file. (could have incrementally done it directly above...)
  outputFile.tick(frames);

  return 0;
}
