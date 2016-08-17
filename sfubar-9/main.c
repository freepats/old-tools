#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sfubar.h"

void usage(void)
{
  fprintf(stderr, "\nsfubar version %d\n\n", SFUTIL_VERSION);
  fprintf(stderr, "%s",
    "Usage: sfubar [conversion] [infile] [outfile]\n"
    "\tconversion := --sf2txt | --txt2sf | --sfdebug\n"
    "\tconversion := --aif2txt | --txt2aif | --aifdebug\n"
    "\tconversion := --wav2txt | --txt2wav | --wavdebug\n\n");
  exit(0);
}

int main(int argc, char *argv[])
{
  if (argc != 4)
    usage();

  if (strcmp(argv[1], "--sf2txt") == 0) {
    sf_to_txt(argv[2], argv[3], 0);
  } else if (strcmp(argv[1], "--txt2sf") == 0) {
    txt_to_sf(argv[2], argv[3]);
  } else if (strcmp(argv[1], "--sfdebug") == 0) {
    sf_to_txt(argv[2], argv[3], 1);
  } else if (strcmp(argv[1], "--aif2txt") == 0) {
    aif_to_txt(argv[2], argv[3], 0);
  } else if (strcmp(argv[1], "--txt2aif") == 0) {
    txt_to_aif(argv[2], argv[3]);
  } else if (strcmp(argv[1], "--aifdebug") == 0) {
    aif_to_txt(argv[2], argv[3], 1);
  } else if (strcmp(argv[1], "--wav2txt") == 0) {
    wav_to_txt(argv[2], argv[3], 0);
  } else if (strcmp(argv[1], "--txt2wav") == 0) {
    txt_to_wav(argv[2], argv[3]);
  } else if (strcmp(argv[1], "--wavdebug") == 0) {
    wav_to_txt(argv[2], argv[3], 1);
  } else {
    usage();
  }
  exit(0);
}
