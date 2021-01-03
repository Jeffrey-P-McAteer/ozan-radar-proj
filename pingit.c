
#include <alsa/asoundlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#define die(msg) { fprintf(stderr, "[%s:%d] Err: " msg "\n", __FILE__, __LINE__); exit(1); }

// headers are for wusses
int prog_main(char* cmd, int delay_ms);
void help();
void list_hw();
void detect_hw();

int main(int argc, char** argv) {
  // Args go on the stack
  char* cmd = "help";
  uint64_t delay_ms = 1000;

  // Parse args
  if (argc > 1) {
    cmd = argv[1];
  }
  char* strtoul_ptr; // records end of parsing, unused.
  int c;
  opterr = 0;
  while ((c = getopt (argc, argv, "d:")) != -1) {
    switch (c) {
      case 'd':
        delay_ms = strtoul(optarg, &strtoul_ptr, 10);
        break;
      case '?':
        if (optopt == 'd')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort();
      }
  }

  // Pass parsed args to prog_main
  return prog_main(cmd, delay_ms);

}

int prog_main(char* cmd, int delay_ms) {
  if (strcmp(cmd, "help") == 0) {
    help();
  }
  else if (strcmp(cmd, "list-hw") == 0) {
    list_hw();
  }
  else if (strcmp(cmd, "detect-hw") == 0) {
    detect_hw();
  }
  else if (strcmp(cmd, "ping") == 0) {
    printf("ping delay_ms=%d\n", delay_ms);

  }
  else {
    help();
  }
  return 0;
}

void help() {
  printf(
    "Usage: pingit cmd [options]\n"\
    "\n"\
    "Where cmd is one of:\n"\
    "\n"\
    "  help      print this help text\n"\
    "  list-hw   List ALSA audio devices\n"\
    "  detect-hw Builds a list of ALSA devices, prints an instruction to connect new devices, prints the new device names.\n"\
    "  ping      Send autio to speaker + record from mic in a loop. See 'ping args' below for details.\n"\
    "\n"\
    "ping args:\n"\
    "  -d ms     Set the delay between pings\n"\
    "  \n"\
    "\n"\
    "\n"\
    "\n"\
    "\n"
  );
}

void listdev(char *devname);

void list_hw() {
  char** hints;
  int err;
  err = snd_device_name_hint(-1, "pcm", (void***)&hints);
  if (err != 0) {
      die("Cannot get device names");
  }

  listdev("pcm");

}

void detect_hw() {
  char** hints;
  int err;
  err = snd_device_name_hint(-1, "pcm", (void***)&hints);
  if (err != 0) {
      die("Cannot get device names");
  }

  listdev("pcm");

}

void listdev(char *devname)
{

    char** hints;
    int    err;
    char** n;
    char*  name;
    char*  desc;
    char*  ioid;

    /* Enumerate sound devices */
    err = snd_device_name_hint(-1, devname, (void***)&hints);
    if (err != 0) {

        fprintf(stderr, "*** Cannot get device names\n");
        exit(1);

    }

    n = hints;
    while (*n != NULL) {

        name = snd_device_name_get_hint(*n, "NAME");
        desc = snd_device_name_get_hint(*n, "DESC");
        ioid = snd_device_name_get_hint(*n, "IOID");

        printf("Name of device: %s\n", name);
        printf("Description of device: %s\n", desc);
        printf("I/O type of device: %s\n", ioid);
        printf("\n");

        if (name && strcmp("null", name)) free(name);
        if (desc && strcmp("null", desc)) free(desc);
        if (ioid && strcmp("null", ioid)) free(ioid);
        n++;

    }

    //Free hint buffer too
    snd_device_name_free_hint((void**)hints);

}