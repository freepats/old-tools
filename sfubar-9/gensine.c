#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <riffraff/bits.h>
#include <riffraff/rifftypes.h>

double fpart(double x)
{
  double retval;
  retval = x - floor(x);
  return retval;
}

int main(int argc, char *argv[])
{
  int sr = 44100;
  int sample_count = 2205;
  int iamp = 10000;
  int freq = 440;
  int s;
  double sample_period = (double)sr / (double)freq;
  double arg;
  mySHORT value;
  char buffer[2];

#define OUTFD 1

#ifdef __MINGW32__
  _setmode(OUTFD, _O_BINARY);
#endif

  for (s = 0; s < sample_count; s++) {
    arg = (double)s / sample_period;
    arg = fpart(arg);
    arg = 2 * M_PI * arg;
    value = sin(arg) * iamp;
    my_put_SHORT_LE(&value, buffer);
    write(OUTFD, buffer, 2);
  }
  exit(0);
}
