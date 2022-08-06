
#include "headers.h"

#include "configfs.h"
#include "processing.h"
#include "v4l2_endpoint.h"
#include "uvc_endpoint.h"
#include "fb_endpoint.h"
#include "image_endpoint.h"
#include "stdin_endpoint.h"
#include "uvc_events.h"
#include "uvc_control.h"
#include "uvc_device_detect.h"
#include "system.h"
#include "data_buffers.h"

struct processing processing;
struct stored_controls stored_controls;
bool show_fps = false;
bool debug = false;
bool streaming_status_onboard = false;
bool autodetect_uvc_device = false;
bool ignore_camera_controls = false;
bool jpeg_format_use = false;
const char *fb_device_name;
const char *uvc_device_name;
const char *v4l2_device_name;
const char *image_path;
unsigned int stdin_format;
unsigned int stdin_width;
unsigned int stdin_height;
char *streaming_status_pin;
int blink_on_startup = 0;
int framerate = 30;
int nbufs = 2;

static bool terminate = false;
static bool stopped = false;

struct option long_options[] =
{
    {"autodetect", no_argument, NULL, OPT_AUTODETECT},
    {"blink", required_argument,  NULL, OPT_BLINK},
    {"debug", no_argument, NULL, OPT_DEBUG},
    {"framebuffer", required_argument, NULL, OPT_FRAMEBUFFER},
    {"help", no_argument, NULL, OPT_HELP},
    {"image", required_argument, NULL, OPT_IMAGE},
    {"jpeg", no_argument, NULL, OPT_JPEG},
    {"led", no_argument, NULL, OPT_ONBOARD_LED},
    {"dimensions", required_argument, NULL, OPT_DIMENSIONS},
    {"buffers", required_argument, NULL, OPT_BUFFERS},
    {"pin", required_argument, NULL, OPT_GPIO_PIN},
    {"fps", required_argument, NULL, OPT_FRAMERATE},
    {"stdin", required_argument, NULL, OPT_STDIN},
    {"uvc", required_argument, NULL, OPT_UVC},
    {"v4l2", required_argument, NULL, OPT_V4L2},
    {"show-fps", no_argument, NULL, OPT_SHOW_FPS},
    {"ignore-controls", no_argument, NULL, OPT_IGNORE_CONTROLS},
    {"ocontrol", required_argument, NULL, OPT_CONTROL_APPLY_ONCE},
    {"acontrol", required_argument, NULL, OPT_CONTROL_APPLY_ALWAYS},
    {0, 0, 0, 0}
};

char *short_options = "adhlb:f:i:jm:n:p:r:s:u:v:xz";

void onSignal(int signum)
{
    switch (signum)
    {
    case SIGTERM:
    case SIGINT:
        printf("\nSIGNAL: Signal %s\n", (signum == SIGTERM) ? "TERMINATE" : "INTERRUPT");
        terminate = true;
        break;

    case SIGUSR1:
        printf("\nSIGNAL: Signal USER-DEFINED 1, STOP STREAMING%s\n", (stopped) ? " - Already stopped" : "");
        stopped = true;
        break;

    case SIGUSR2:
        printf("\nSIGNAL: Signal USER-DEFINED 2, RESUME STREAMING%s\n", (!stopped) ? " - Already running" : "");
        stopped = false;
        break;

    default:
        break;
    }
}

void cleanup()
{
    fb_close(&processing);
    image_close(&processing);
    v4l2_close(&processing);
    uvc_close(&processing);
    buffers_free(&processing);

    printf("UVC-GADGET: Exit\n");
}

int init()
{
    const char *uvc_device;

    signal(SIGTERM, onSignal);
    signal(SIGINT, onSignal);
    signal(SIGUSR1, onSignal);
    signal(SIGUSR2, onSignal);

    memset(&processing, 0, sizeof(struct processing));

    processing.stored_controls = &stored_controls;

    processing.events.terminate = &terminate;
    processing.events.stopped = &stopped;

    processing.source.type = ENDPOINT_NONE;
    processing.target.type = ENDPOINT_NONE;

    processing.events.stream = STREAM_NONE;
    processing.events.shutdown_requested = false;

    processing.settings.debug = debug;
    processing.settings.show_fps = show_fps;
    processing.settings.blink_on_startup = blink_on_startup;
    processing.settings.streaming_status_pin = streaming_status_pin;
    processing.settings.streaming_status_onboard = streaming_status_onboard;
    processing.settings.frame_interval = (1000 / framerate);
    processing.settings.ignore_camera_controls = ignore_camera_controls;

    printf("UVC-GADGET: Initialization\n");

    if (configfs_init(&processing) != 1)
    {
        goto err;
    }

    v4l2_init(&processing, v4l2_device_name, nbufs, jpeg_format_use);
    fb_init(&processing, fb_device_name);
    image_init(&processing, image_path);
    stdin_init(&processing, stdin_format, stdin_width, stdin_height);

    if (uvc_device_name)
    {
        uvc_init(&processing, uvc_device_name, nbufs);
    }
    else if (autodetect_uvc_device)
    {
        uvc_device = uvc_device_detect();
        if (uvc_device)
        {
            printf("UVC: Autodetect found UVC device: %s\n", uvc_device);
        }
        uvc_init(&processing, uvc_device, nbufs);
    }

    system_init(&processing);

    if (processing.source.type == ENDPOINT_NONE)
    {
        printf("ERROR: Missing source endpoint\n");
        goto err;
    }

    if (processing.target.type == ENDPOINT_NONE)
    {
        printf("ERROR: Missing target endpoint\n");
        goto err;
    }

    if (processing.target.type == ENDPOINT_UVC)
    {
        uvc_fill_streaming_control(&processing, &(processing.target.uvc.probe), STREAM_CONTROL_INIT, NULL);
        uvc_fill_streaming_control(&processing, &(processing.target.uvc.commit), STREAM_CONTROL_INIT, NULL);

        uvc_events_subscribe(&processing);
    }

    processing_loop(&processing);

    if (processing.target.type == ENDPOINT_UVC)
    {
        uvc_events_unsubscribe(&processing);
    }

    cleanup();
    return 0;

err:
    cleanup();
    return 1;
}

static void usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s [options]\n", argv0);
    fprintf(stderr, "\nOptions\n");
    fprintf(stderr, " -a, --autodetect                Find UVC device automatically\n");
    fprintf(stderr, " -b, --blink <value>             Blink X times on startup\n");
    fprintf(stderr, "                                   (b/w 1 and 20 with led0 or GPIO pin if defined)\n");
    fprintf(stderr, " -d, --debug                     Enable debug messages\n");
    fprintf(stderr, " -f, --framebuffer <device>      Framebuffer as input device\n");
    fprintf(stderr, " -h, --help                      Print this help screen and exit\n");
    fprintf(stderr, " -i, --image <path>              Path to MJPEG/YUYV image\n");
    fprintf(stderr, " -j, --jpeg                      Use JPEG format instead of MJPEG for V4L2 device\n");
    fprintf(stderr, " -l, --led                       Use onboard led0 for streaming status indication\n");
    fprintf(stderr, " -m, --dimensions <value>        STDIN stream dimension (WIDTHxHEIGHT like 800x600)\n");
    fprintf(stderr, " -n, --buffers <value>           Number of Video buffers (b/w 2 and 32)\n");
    fprintf(stderr, " -p, --pin <value>               GPIO pin number for streaming status indication\n");
    fprintf(stderr, " -r, --fps <value>               Framerate for framebuffer (b/w 1 and 200)\n");
    fprintf(stderr, " -s, --stdin <value>             STDIN stream type (MJPEG/YUYV)\n");
    fprintf(stderr, " -u, --uvc <device>              UVC Video Output device\n");
    fprintf(stderr, " -v, --v4l2 <device>             V4L2 Video Capture device\n");
    fprintf(stderr, " -x, --show-fps                  Show fps information\n");
    fprintf(stderr, " -z, --ignore-controls           Ignore camera controls\n");
    fprintf(stderr, "     --acontrol <control=value>  Apply control to V4L2 device ALWAYS when the stream starts\n");
    fprintf(stderr, "                                   (example: video_bitrate=25000000)\n");
    fprintf(stderr, "     --ocontrol <control=value>  Apply control to V4L2 device ONCE when the stream starts\n");
    fprintf(stderr, "                                   (example: brightness=50)\n");
}

int main(int argc, char *argv[])
{
    int opt;
    int option_index = 0;

    memset(&stored_controls, 0, sizeof(struct stored_controls));

    while ((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'a':
            autodetect_uvc_device = true;
            break;

        case 'b':
            if (atoi(optarg) < 1 || atoi(optarg) > 20)
            {
                fprintf(stderr, "ERROR: Blink x times on startup\n");
                goto err;
            }
            blink_on_startup = atoi(optarg);
            break;

        case 'd':
            debug = true;
            break;

        case 'f':
            fb_device_name = optarg;
            break;

        case 'h':
            usage(argv[0]);
            return 1;

        case 'i':
            image_path = optarg;
            break;

        case 'j':
            jpeg_format_use = true;
            break;

        case 'l':
            streaming_status_onboard = true;
            break;

        case 'm':
            if (2 != sscanf(optarg, "%dx%d", &stdin_width, &stdin_height))
            {
                fprintf(stderr, "ERROR: Wrong input stream dimension - %s\n", optarg);
                goto err;
            }
            break;

        case 'n':
            if (atoi(optarg) < 2 || atoi(optarg) > 32)
            {
                fprintf(stderr, "ERROR: Number of Video buffers value out of range\n");
                goto err;
            }
            nbufs = atoi(optarg);
            break;

        case 'p':
            streaming_status_pin = optarg;
            break;

        case 'r':
            if (atoi(optarg) < 1 || atoi(optarg) > 200)
            {
                fprintf(stderr, "ERROR: Framerate value out of range\n");
                goto err;
            }
            framerate = atoi(optarg);
            break;

        case 's':
            if (!strncmp(optarg, "MJPEG", 5))
            {
                stdin_format = V4L2_PIX_FMT_MJPEG;
            }
            else if (!strncmp(optarg, "YUYV", 4))
            {
                stdin_format = V4L2_PIX_FMT_YUYV;
            }
            else
            {
                fprintf(stderr, "ERROR: Wrong input stream type\n");
                goto err;
            }
            break;

        case 'u':
            uvc_device_name = optarg;
            break;

        case 'v':
            v4l2_device_name = optarg;
            break;

        case 'x':
            show_fps = true;
            break;

        case 'z':
            ignore_camera_controls = true;
            break;

        case OPT_CONTROL_APPLY_ONCE:
            stored_controls.controls[stored_controls.length].args = optarg;
            stored_controls.controls[stored_controls.length].apply_type = CONTROL_APPLY_ONCE;
            stored_controls.length += 1;
            break;

        case OPT_CONTROL_APPLY_ALWAYS:
            stored_controls.controls[stored_controls.length].args = optarg;
            stored_controls.controls[stored_controls.length].apply_type = CONTROL_APPLY_ALWAYS;
            stored_controls.length += 1;
            break;

        default:
            printf("ERROR: Invalid option '-%c'\n", opt);
            return 1;
        }
    }
    return init();

err:
    usage(argv[0]);
    return 1;
}
