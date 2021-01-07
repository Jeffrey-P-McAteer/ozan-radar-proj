
#include <alsa/asoundlib.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define die(msg) { fprintf(stderr, "[%s:%d] Err: " msg "\n", __FILE__, __LINE__); exit(1); }

// headers are for wusses
int prog_main(char* cmd, int v, uint64_t delay_ms, float sensitivity, char* input_dev_name, char* output_raw_pcm_file, char* output_dev_name);
void help();
void list_hw();
void detect_hw();
void ping_loop(int v, uint64_t delay_ms, char* wave_type, float sensitivity, char* input_dev_name, char* output_raw_pcm_file, char* output_dev_name);

int main(int argc, char** argv) {
  // Args go on the stack
  char* cmd = "help";
  int v = 0;
  uint64_t delay_ms = 1000;
  float sensitivity = 2.0;
  char* input_dev_name = "";
  char* output_raw_pcm_file = "";
  char* output_dev_name = "";

  // Parse args
  if (argc > 1) {
    cmd = argv[1];
  }
  int c;
  opterr = 0;
  while ((c = getopt (argc, argv, "d:i:o:s:w:v")) != -1) {
    switch (c) {
      case 'v':
        v += 1;
        break;

      case 'd':
        delay_ms = strtoul(optarg, NULL, 10);
        break;

      case 's':
        sensitivity = strtod(optarg, NULL);
        break;

      case 'w':
        output_raw_pcm_file = optarg;
        break;

      case 'i':
        input_dev_name = optarg;
        break;

      case 'o':
        output_dev_name = optarg;
        break;

      case '?':
        if (optopt == 'd' || optopt == 's' || optopt == 'w' || optopt == 'i' || optopt == 'o') {
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
  return prog_main(cmd, v, delay_ms, sensitivity, input_dev_name, output_raw_pcm_file, output_dev_name);

}

int prog_main(char* cmd, int v, uint64_t delay_ms, float sensitivity, char* input_dev_name, char* output_raw_pcm_file, char* output_dev_name) {
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
    // TODO let user specify wave type, once we support those
    ping_loop(v, delay_ms, "square", sensitivity, input_dev_name, output_raw_pcm_file, output_dev_name);
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
    "  -s factor     Set signal detection sensitivity as percent over average noise (default 2.0, 1.0 means bg noise will trigger and 4.0 requires very loud noise)\n"\
    "  -i dev-name   Name of input audio device (mic)\n"\
    "  -w raw_pcm_file  Name of file to dump 32-bit float PCM data to (convert to .wav with TODO FFMPEG )"
    "  -o dev-name   Name of output audio device (speaker)\n"\
    "  -v            Be verbose (repeat for more verbosity)\n"\
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

// Returns positive value if end > start
uint64_t ns_diff(struct timespec* end, struct timespec* start) {
  uint64_t sec_diff;
  uint64_t nsec_diff;

  if ((end->tv_nsec - start->tv_nsec) < 0) {
    sec_diff = end->tv_sec - start->tv_sec - 1;
    nsec_diff = end->tv_nsec - start->tv_nsec + 1000000000;
  }
  else {
    sec_diff = end->tv_sec - start->tv_sec;
    nsec_diff = end->tv_nsec - start->tv_nsec;
  }

  return (sec_diff * 1000000) + nsec_diff;

}

#include <byteswap.h>
#include <limits.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define LE_SHORT(v)   (v)
#define LE_INT(v)   (v)
#define BE_SHORT(v)   bswap_16(v)
#define BE_INT(v)   bswap_32(v)
#else /* __BIG_ENDIAN */
#define COMPOSE_ID(a,b,c,d) ((d) | ((c)<<8) | ((b)<<16) | ((a)<<24))
#define LE_SHORT(v)   bswap_16(v)
#define LE_INT(v)   bswap_32(v)
#define BE_SHORT(v)   (v)
#define BE_INT(v)   (v)
#endif


// https://gist.github.com/albanpeignier/104902
// Also https://gist.github.com/ghedo/963382/815c98d1ba0eda1b486eb9d80d9a91a81d995283
void ping_loop(int v, uint64_t delay_ms, char* wave_type, float sensitivity, char* input_dev_name, char* output_raw_pcm_file, char* output_dev_name) {
  // How verbose are we today?
  if (v > 1) {
    printf("verbosity level = %d\n", v);
  }

  // Input locals
  int i;
  int err;
  
  unsigned int rate = 44100;
  int in_buffer_frames = 256;
  float* input_buffer;

  snd_pcm_t* capture_handle;
  snd_pcm_hw_params_t* hw_params;
  snd_pcm_format_t format = SND_PCM_FORMAT_FLOAT;

  { // mic HW init
    if ((err = snd_pcm_open(&capture_handle, input_dev_name, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
      fprintf(stderr, "cannot open audio device %s (%s)\n",  input_dev_name, snd_strerror(err));
      exit(1);
    }

    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
      fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
      exit(1);
    }

    if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
      fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
      exit(1);
    }

    if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
      fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
      exit(1);
    }

    if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format)) < 0) {
      fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
      exit(1);
    }

    if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0)) < 0) {
      fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
      exit(1);
    }

    if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, 2)) < 0) {
      fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
      exit(1);
    }

    if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
      fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
      exit(1);
    }

    snd_pcm_hw_params_free(hw_params);

    if ((err = snd_pcm_prepare(capture_handle)) < 0) {
      fprintf(stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err));
      exit(1);
    }

    input_buffer = calloc(in_buffer_frames, snd_pcm_format_width(format) / 8 * 2);

  }

  // Output locals
  unsigned int pcm, tmp, dir;
  int output_rate, channels;
  snd_pcm_t *out_pcm_handle;
  snd_pcm_hw_params_t *params;
  snd_pcm_uframes_t out_buffer_frames;
  float* out_buffer; // Float 32 bit CPU endian 
  int out_buffer_size, loops;

  output_rate = rate;
  channels = 1;

  { // speaker HW init
    /* Open the PCM device in playback mode */
    if (pcm = snd_pcm_open(&out_pcm_handle, output_dev_name, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
      printf("ERROR: Can't open \"%s\" PCM device. %s\n", output_dev_name, snd_strerror(pcm));
      exit(1);
    }

    /* Allocate parameters object and fill it with default values*/
    snd_pcm_hw_params_alloca(&params);

    snd_pcm_hw_params_any(out_pcm_handle, params);

    /* Set parameters */
    if (pcm = snd_pcm_hw_params_set_access(out_pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
      printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));
      exit(1);
    }

    if (pcm = snd_pcm_hw_params_set_format(out_pcm_handle, params, SND_PCM_FORMAT_FLOAT) < 0) {
      printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));
      exit(1);
    }

    if (pcm = snd_pcm_hw_params_set_channels(out_pcm_handle, params, channels) < 0) {
      printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));
      exit(1);
    }

    if (pcm = snd_pcm_hw_params_set_rate_near(out_pcm_handle, params, &output_rate, 0) < 0) {
      printf("ERROR: Can't set output_rate. %s\n", snd_strerror(pcm));
      exit(1);
    }

    /* Write parameters */
    if (pcm = snd_pcm_hw_params(out_pcm_handle, params) < 0) {
      printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(pcm));
      exit(1);
    }

    /* Resume information */
    if (v > 0) {
      printf("PCM name: '%s'\n", snd_pcm_name(out_pcm_handle));

      printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(out_pcm_handle)));
    }

    snd_pcm_hw_params_get_channels(params, &tmp);
    if (v > 0) {
      printf("channels: %i ", tmp);

      if (tmp == 1)
        printf("(mono)\n");
      else if (tmp == 2)
        printf("(stereo)\n");
    }

    snd_pcm_hw_params_get_rate(params, &tmp, 0);

    if (v > 0) {
      printf("out rate: %d bps\n", tmp);
    }

    /* Allocate out_bufferer to hold single period */
    snd_pcm_hw_params_get_period_size(params, &out_buffer_frames, 0);

    out_buffer_size = out_buffer_frames * channels * 2 /* 2 -> sample size */;
    out_buffer = calloc(out_buffer_size, sizeof(float));
    
    // TODO generate output sine wave in out_buffer
    // See https://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html

    // We use SND_PCM_FORMAT_FLOAT
    // Do not produce sound <50_000 hertz!
    int f = 50000;                //frequency (aim for 50khz, see https://www.fisheries.noaa.gov/topic/marine-mammal-protection)
    int fs = 48000;             //sampling frequency
    for (int i=0; i<out_buffer_size; i++) {
      out_buffer[i] = (sin(2*M_PI*f/fs*i));
    }

    snd_pcm_hw_params_get_period_time(params, &tmp, NULL);

  }

  FILE* raw_pcm_file = open(output_raw_pcm_file, "wb");

  // The business logic loop
  uint64_t delay_ns = delay_ms * 1000;
  struct timespec last_ping_out = {0,0};
  struct timespec last_ping_in = {0,0};
  struct timespec now = {0,0};
  
  float avg_amplitude = 0.0;
  int num_readings_to_keep_avg_of = 4096; // the higher this is the longer the avg window's timespan

  clock_gettime(CLOCK_MONOTONIC, &last_ping_out);
  clock_gettime(CLOCK_MONOTONIC, &last_ping_in);
  clock_gettime(CLOCK_MONOTONIC, &now);

  while (true) {

    //usleep(10 * 1000); // 10 * 1000 * 1000 nanoseconds
    clock_gettime(CLOCK_MONOTONIC, &now);
    // double now_s = (((double) now.tv_sec) / 1000000.0) + (double) now.tv_nsec;

    if (v > 0) {
      printf(".");
      fflush(stdout);
    }

    uint64_t d = ns_diff(&now, &last_ping_out);

    if (d > delay_ns) {
      // Send another ping, reset timers
      clock_gettime(CLOCK_MONOTONIC, &last_ping_out);

      if (v > 0) {
        printf("!");
        fflush(stdout);
      }

      if ((err = snd_pcm_writei(out_pcm_handle, out_buffer, out_buffer_frames)) != out_buffer_frames) {
        fprintf(stderr, "Can't write to PCM device (%s)\n", snd_strerror(err));
        break;
      }
      else {
        // Write success, reset out_pcm_handle
        snd_pcm_prepare(out_pcm_handle);
      }

    }

    // record incoming audio to a buffer
    if ((err = snd_pcm_readi(capture_handle, input_buffer, in_buffer_frames)) != in_buffer_frames) {
      fprintf(stderr, "read from audio interface failed (%s)\n", snd_strerror(err));
      break;
    }

    // Save to debug file
    // if (raw_pcm_file != NULL) {
    //   fwrite(input_buffer, 1, in_buffer_frames, raw_pcm_file);
    // }
    // ^^ TODO debug segfault?

    if (v > 1) {
      printf("input_buffer=");
      for (int i=0; i<in_buffer_frames; i++) {
        printf("%.2f ", input_buffer[i]);
      }
      printf("\n");
      printf("avg_amplitude=%.4f\n", avg_amplitude);
    }

    // Update avg amplitude
    for (int i=0; i<in_buffer_frames; i++) {
      avg_amplitude -= (avg_amplitude / num_readings_to_keep_avg_of);
      avg_amplitude += fabs(input_buffer[i] / num_readings_to_keep_avg_of);
    }

    // Detect a loud noise (anything X% over the running average)
    float highest_amplitude = -999.0;
    for (int i=0; i<in_buffer_frames; i++) {
      if (fabs(input_buffer[i]) > highest_amplitude) {
        highest_amplitude = fabs(input_buffer[i]);
      }
    }

    if (v > 0) {
      printf("highest_amplitude=%.4f\n", highest_amplitude);
    }

    if (fabs(highest_amplitude) > fabs(avg_amplitude * sensitivity)) {
      
      if (v > 0) {
        printf("\nNOISE!\n\n");
      }

      // Distance calculation
      uint64_t ns_since_last_ping = ns_diff(&now, &last_ping_out);

      // TODO let user specify speed of sound in medium as meters/sec
      double sound_speed = 343.0;
      double sound_speed_meters_per_ns = sound_speed * (1.0 / 1000000000.0);

      float distance_m = sound_speed_meters_per_ns * (ns_since_last_ping);
      printf("distance_m=%.3f\n", distance_m);

    }


  }

  printf("Exiting...\n");

  if (raw_pcm_file != NULL) {
    fclose(raw_pcm_file);
  }

  free(input_buffer);
  snd_pcm_close(capture_handle);

  snd_pcm_drain(out_pcm_handle);
  snd_pcm_close(out_pcm_handle);
  free(out_buffer);

}
