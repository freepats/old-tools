#include <stdio.h>
#include <stdlib.h>

#include <trap.h>

void trap_default(int severity)
{
  /* print severity */
  if (severity == 0) {
    fprintf(stderr, "warning: ");
  } else {
    fprintf(stderr, "error: ");
  }

  /* print line number of last line read */
  if (trap_txt != NULL)
    fprintf(stderr, "%d: ", trap_txt->l);

  /* print error message */
  fprintf(stderr, "%s\n", trap_message);

  /* exit */
  if (severity != 0)
    exit(1);
}

char trap_message[TRAP_BUFLEN];
void (*trap_global)(int) = trap_default;
txt_t *trap_txt = NULL;
