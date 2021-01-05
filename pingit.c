
#include <alsa/asoundlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#define die(msg) { fprintf(stderr, "[%s:%d] Err: " msg "\n", __FILE__, __LINE__); exit(1); }

// headers are for wusses
int prog_main(char* cmd, uint64_t delay_ms, char* input_dev_name, char* output_dev_name);
void help();
void list_hw();
void detect_hw();

int main(int argc, char** argv) {
  // Args go on the stack
  char* cmd = "help";
  uint64_t delay_ms = 1000;
  char* input_dev_name = "";
  char* output_dev_name = "";

  // Parse args
  if (argc > 1) {
    cmd = argv[1];
  }
  char* strtoul_ptr; // records end of parsing, unused.
  int c;
  opterr = 0;
  while ((c = getopt (argc, argv, "dio:")) != -1) {
    switch (c) {
      case 'd':
        delay_ms = strtoul(optarg, &strtoul_ptr, 10);
        break;

      case 'i':
        input_dev_name = optarg;
        break;

      case 'o':
        output_dev_name = optarg;
        break;

      case '?':
        if (optopt == 'd') {
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        }
        else if (isprint (optopt)) {
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
          return 1;
        }
        else {
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
          return 1;
        }
      default:
        abort();
    }
  }

  // Pass parsed args to prog_main
  return prog_main(cmd, delay_ms, input_dev_name, output_dev_name);

}

int prog_main(char* cmd, uint64_t delay_ms, char* input_dev_name, char* output_dev_name) {
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
    printf("ping delay_ms=%lu\n", delay_ms);

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
    "  -d ms         Set the delay between pings\n"\
    "  -i dev-name   Name of input audio device (mic)\n"\
    "  -o dev-name   Name of output audio device (speaker)\n"\
    "\n"\
    "\n"\
    "\n"\
    "\n"
  );
}

void list_hw() {
  char** hints;
  char** n;
  char*  name;
  char*  desc;
  char*  ioid;
  int err;
  err = snd_device_name_hint(-1, "pcm", (void***)&hints);
  if (err != 0) {
      die("Cannot get device names");
  }

  n = hints;
  while (*n != NULL) {

      name = snd_device_name_get_hint(*n, "NAME");
      desc = snd_device_name_get_hint(*n, "DESC");
      ioid = snd_device_name_get_hint(*n, "IOID");

      for (int i=0; desc && i<strlen(desc); i++) {
        if (desc[i] == '\n') {
          desc[i] = ' '; // I don't want line breaks in my strings
        }
      }

      printf("%s\n   - %s, %s\n", name, ioid, desc);

      // Wierd that this is necessary
      if (name && strcmp("null", name)) free(name);
      if (desc && strcmp("null", desc)) free(desc);
      if (ioid && strcmp("null", ioid)) free(ioid);

      n++;

  }

  // Free hint buffer 
  snd_device_name_free_hint((void**)hints);

}

void detect_hw() {

  size_t seen_devices_len = 32 * 1024;
  size_t seen_devices_remaining = seen_devices_len; // used for buffer overflow tracking
  char* seen_devices = calloc(seen_devices_len, sizeof(char));
  bool first_run_done = false;
  bool seen_new_devices = false;

  while (true) {
    // Delay 500ms
    usleep(500 * 1000);

    // Query audio HW, reporting new devices.
    char** hints = NULL;
    char** n = NULL;
    char*  name = NULL;
    char*  desc = NULL;
    char*  ioid = NULL;
    int err = 1;
    err = snd_device_name_hint(-1, "pcm", (void***)&hints);
    if (err != 0) {
        die("Cannot get device names");
    }

    n = hints;
    while (*n != NULL) {

        name = snd_device_name_get_hint(*n, "NAME");
        desc = snd_device_name_get_hint(*n, "DESC");
        ioid = snd_device_name_get_hint(*n, "IOID");

        for (int i=0; desc && i<strlen(desc); i++) {
          if (desc[i] == '\n') {
            desc[i] = ' '; // I don't want line breaks in my strings
          }
        }

        if (first_run_done) {
          // Is this new?
          char* substring_ptr = strstr(seen_devices, name);
          if (substring_ptr == NULL) {
            // tell the user!
            printf("%s\n   - %s, %s\n", name, ioid, desc);
            seen_new_devices = true;
          }
        }
        else {
          // First run, add it to the ignore list
          strncat(seen_devices, name, seen_devices_remaining);
          seen_devices_remaining -= strlen(name);
        }

        // Wierd that this is necessary
        if (name && strcmp("null", name)) free(name);
        if (desc && strcmp("null", desc)) free(desc);
        if (ioid && strcmp("null", ioid)) free(ioid);

        n++;

    }

    // Free hint buffer 
    snd_device_name_free_hint((void**)hints);

    if (!first_run_done && strlen(seen_devices) > 5) {
      first_run_done = true;
      printf("Connect new audio hardware now...\n");
    }

    if (seen_new_devices) {
      // We should probably exit
      printf("Exiting because we saw new devices!\n");
      break;
    }

  }

}