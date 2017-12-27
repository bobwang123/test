#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

class Scheduler;

extern void
compute(Scheduler *sh);

extern void
upload(const char *api);

/* Read characters from the pipe and echo them to stdout. */

void
read_from_pipe (int file)
{
  FILE *stream;
  int c;
  stream = fdopen (file, "r");
  while ((c = fgetc (stream)) != EOF)
    putchar (c);
  fclose (stream);
}

/* Write some random text to the pipe. */

void
write_to_pipe (int file)
{
  FILE *stream;
  stream = fdopen (file, "w");
  fprintf (stream, "Done!\n");
  fclose (stream);
}

int
multi_proc_main(const char *api, Scheduler *sh)
{
  pid_t pid;
  int mypipe[2];

  /* Create the pipe. */
  if (pipe (mypipe))
  {
    fprintf (stderr, "Pipe failed.\n");
    return EXIT_FAILURE;
  }

  /* Create the child process. */
  pid = fork ();
  if (pid == (pid_t) 0)
  {
    /* This is the child process.
       Close other end first. */
    close (mypipe[0]);
    // TODO: put cm and sh here
    compute(sh);
    write_to_pipe (mypipe[1]);
    return EXIT_SUCCESS;
  }
  else if (pid < (pid_t) 0)
  {
    /* The fork failed. */
    fprintf (stderr, "Fork failed.\n");
    return EXIT_FAILURE;
  }
  else
  {
    /* This is the parent process.
       Close other end first. */
    close (mypipe[1]);
    read_from_pipe (mypipe[0]);
    upload(api);
    return EXIT_SUCCESS;
  }
}

