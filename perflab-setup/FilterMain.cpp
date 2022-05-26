#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"



using namespace std;

#include "rdtsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);
    for (int i=0; i < size; i++) {
      for (int j=0; j < size; j++) {
	int value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
      
    return filter;
  } else {
    cerr << "Bad input in readFilter:" << filename << endl;
    exit(-1);
  }
}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;  

  cycStart = rdtscll();

  output -> width = input -> width;
    
  short int in_width = input->width-1;  //removed from loop 
//   short int in_width1 = input->width;  
  output -> height = input -> height;
    
  short int in_height = input->height -1; // removed from loop 
    
  short int filter_div = filter ->getDivisor();// no longer calculated in loop
    
  char filt_00 = filter->get(0, 0);
  char filt_01 = filter->get(0, 1); 
  char filt_02 = filter->get(0, 2);
  char filt_10 = filter->get(1, 0);
  char filt_11 = filter->get(1, 1); 
  char filt_12 = filter->get(1, 2);
  char filt_20 = filter->get(2, 0);
  char filt_21 = filter->get(2, 1); 
  char filt_22 = filter->get(2, 2);


    for (short int plane = 0; plane < 3; plane++)
    {

      for (short int row = 1; row < in_height; row++)
      {
        for (short int col = 1; col < in_width; col++)
        {

          output->color[plane][row][col] = 0;

          short int output_accum = 0; 


          output_accum = output_accum + (input->color[plane][row - 1][col - 1] * filt_00);
          output_accum = output_accum + (input->color[plane][row - 1][col] * filt_01);
          output_accum = output_accum + (input->color[plane][row - 1][col + 1] * filt_02);

          output_accum = output_accum + (input->color[plane][row][col - 1] * filt_10);
          output_accum = output_accum + (input->color[plane][row][col] * filt_11);
          output_accum = output_accum + (input->color[plane][row][col + 1] * filt_12);

          output_accum = output_accum + (input->color[plane][row + 1][col - 1] * filt_20);
          output_accum = output_accum + (input->color[plane][row + 1][col] * filt_21);
          output_accum = output_accum + (input->color[plane][row + 1][col + 1] * filt_22);         
              
          output_accum = output_accum / filter_div;
            
          if (output_accum < 0)
          {
            output_accum = 0;
          }

          else if (output_accum > 255)
          {
            output_accum = 255;
          }
            
          output->color[plane][row][col] = output_accum;
        }
      }
    }

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}
