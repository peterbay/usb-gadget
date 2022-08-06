
#ifndef HEADERS
#define HEADERS

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <ftw.h>
#include <dirent.h>
#include <ctype.h>

#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/usb/ch9.h>
#include <linux/usb/video.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <linux/uvcvideo.h>

#define max(a, b) (((a) > (b)) ? (a) : (b))

#define clamp(val, min, max)                   \
    ({                                         \
        typeof(val) __val = (val);             \
        typeof(min) __min = (min);             \
        typeof(max) __max = (max);             \
        (void)(&__val == &__min);              \
        (void)(&__val == &__max);              \
        __val = __val < __min ? __min : __val; \
        __val > __max ? __max : __val;         \
    })

#define pixfmtstr(x) (x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, ((x) >> 24) & 0xff

#define OPT_AUTODETECT 'a'
#define OPT_BLINK 'b'
#define OPT_DEBUG 'd'
#define OPT_FRAMEBUFFER 'f'
#define OPT_HELP 'h'
#define OPT_IMAGE 'i'
#define OPT_JPEG 'j'
#define OPT_ONBOARD_LED 'l'
#define OPT_DIMENSIONS 'm'
#define OPT_BUFFERS 'n'
#define OPT_GPIO_PIN 'p'
#define OPT_FRAMERATE 'r'
#define OPT_STDIN 's'
#define OPT_UVC 'u'
#define OPT_V4L2 'v'
#define OPT_SHOW_FPS 'x'
#define OPT_IGNORE_CONTROLS 'z'
#define OPT_CONTROL_APPLY_ONCE 260
#define OPT_CONTROL_APPLY_ALWAYS 261

#define UVC_INTF_CONTROL 0
#define UVC_INTF_STREAMING 1

#define UVC_EVENT_FIRST (V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_CONNECT (V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_DISCONNECT (V4L2_EVENT_PRIVATE_START + 1)
#define UVC_EVENT_STREAMON (V4L2_EVENT_PRIVATE_START + 2)
#define UVC_EVENT_STREAMOFF (V4L2_EVENT_PRIVATE_START + 3)
#define UVC_EVENT_SETUP (V4L2_EVENT_PRIVATE_START + 4)
#define UVC_EVENT_DATA (V4L2_EVENT_PRIVATE_START + 5)
#define UVC_EVENT_LAST (V4L2_EVENT_PRIVATE_START + 5)

// UVC - Request Error Code Control
#define REQEC_NO_ERROR 0x00
#define REQEC_NOT_READY 0x01
#define REQEC_WRONG_STATE 0x02
#define REQEC_POWER 0x03
#define REQEC_OUT_OF_RANGE 0x04
#define REQEC_INVALID_UNIT 0x05
#define REQEC_INVALID_CONTROL 0x06
#define REQEC_INVALID_REQUEST 0x07
#define REQEC_INVALID_VALUE 0x08

struct uvc_request_data
{
    __s32 length;
    __u8 data[60];
};

struct uvc_event
{
    union
    {
        enum usb_device_speed speed;
        struct usb_ctrlrequest request;
        struct uvc_request_data data;
    };
};

#define UVCIOC_SEND_RESPONSE _IOW('U', 1, struct uvc_request_data)

/* Buffer representing one video frame */
struct buffer
{
    struct v4l2_buffer buf;
    void *start;
    size_t length;
};

struct data_buffer {
    void *start;
    size_t length;
    unsigned int bytesused;
    unsigned int index;
    bool filled;
};

struct data_buffers {
    unsigned int buffer_size;
    bool fill_buffer;
    struct data_buffer *buffer_use;
    struct data_buffer *buffer_fill;
    struct data_buffer *buffers;
};

enum endpoint_type
{
    ENDPOINT_NONE,
    ENDPOINT_V4L2,
    ENDPOINT_UVC,
    ENDPOINT_FB,
    ENDPOINT_IMAGE,
    ENDPOINT_STDIN
};

enum stream_action
{
    STREAM_NONE,
    STREAM_ON,
    STREAM_OFF
};

enum stream_control_action
{
    STREAM_CONTROL_INIT,
    STREAM_CONTROL_MIN,
    STREAM_CONTROL_MAX,
    STREAM_CONTROL_SET,
};

// CONTROLS
struct control_mapping_pair
{
    unsigned int type;
    unsigned int uvc;
    unsigned int v4l2;
    bool enabled;
    unsigned int control_type;
    unsigned int value;
    unsigned int length;
    unsigned int minimum;
    unsigned int maximum;
    unsigned int step;
    unsigned int default_value;
    int v4l2_minimum;
    int v4l2_maximum;
};

struct controls
{
    struct control_mapping_pair mapping[50];
    int size;
    int index;
};

// ENDPOINTS
struct endpoint_v4l2
{
    const char *device_name;
    int fd;
    int is_streaming;
    int stream_on_count;
    struct buffer *mem;
    unsigned int nbufs;
    unsigned long long int dqbuf_count;
    unsigned long long int qbuf_count;
    bool jpeg_format_use;
    bool jpeg_format_available;
};

struct endpoint_fb
{
    const char *device_name;
    int fd;
    int framerate;
    unsigned int bpp;
    unsigned int height;
    unsigned int line_length;
    unsigned int mem_size;
    unsigned int screen_size;
    unsigned int width;
    void *memory;
};

struct endpoint_uvc
{
    const char *device_name;
    int fd;
    struct buffer *mem;
    struct buffer *dummy_buf;
    unsigned int nbufs;

    unsigned long long int qbuf_count;
    unsigned long long int dqbuf_count;

    double last_time_video_process;
    int buffers_processed;

    int is_streaming;

    struct uvc_streaming_control probe;
    struct uvc_streaming_control commit;
};

struct endpoint_image
{
    const char *image_path;
    int inotify_fd;
};

struct endpoint_stdin
{
    unsigned int stdin_format;
    unsigned int width;
    unsigned int height;
};

struct endpoint
{
    enum endpoint_type type;
    struct endpoint_v4l2 v4l2;
    struct endpoint_uvc uvc;
    struct endpoint_fb fb;
    struct endpoint_image image;
    struct endpoint_stdin stdin;
    int state;
};

// UVC REQUEST
struct uvc_request
{
    struct v4l2_event event;
    struct uvc_request_data response;
    struct uvc_request_data data;
    unsigned char response_code;

    uint8_t request;
    uint8_t type;
    uint8_t interface;
    uint8_t control;
    uint8_t length;

    unsigned int set_control;
};

// FRAMEFORMAT
struct frame_format
{
    bool defined;

    enum usb_device_speed usb_speed;
    int video_format;
    const char *format_name;

    unsigned int bFormatIndex;
    unsigned int bFrameIndex;

    unsigned int dwDefaultFrameInterval;
    unsigned int dwMaxVideoFrameBufferSize;
    unsigned int dwMaxBitRate;
    unsigned int dwMinBitRate;
    unsigned int wHeight;
    unsigned int wWidth;
    unsigned int bmCapabilities;
    unsigned int dwFrameInterval;
};

enum frame_format_getter
{
    FORMAT_INDEX_MIN,
    FORMAT_INDEX_MAX,
    FRAME_INDEX_MIN,
    FRAME_INDEX_MAX,
};

// EVENTS
struct events
{
    enum stream_action stream;
    bool shutdown_requested;
    struct frame_format *apply_frame_format;
    struct control_mapping_pair *control;
    bool apply_control;
    bool get_next_frame;
    bool *terminate;
    bool *stopped;
};

// CONFIGFS
struct streaming
{
    unsigned int maxburst;
    unsigned int maxpacket;
    unsigned int interval;
};

struct configfs
{
    char mount_point[255];
    bool mount_point_found;
    struct streaming streaming;
    struct frame_format frame_format[30];
    unsigned int frame_format_size;
};

// SETTINGS
struct settings
{
    bool debug;
    bool show_fps;
    bool streaming_status_onboard;
    bool streaming_status_onboard_enabled;
    bool streaming_status_enabled;
    bool uvc_buffer_required;
    bool ignore_camera_controls;
    char *streaming_status_pin;
    int frame_interval;
    unsigned int blink_on_startup;
    unsigned int uvc_buffer_size;
};

// INTERNALS
struct internals
{
    double last_time_blink;
    bool blink_state;
};

// STORED CONTROLS
enum control_apply_type
{
    CONTROL_NOT_APPLY,
    CONTROL_APPLY_ONCE,
    CONTROL_APPLY_ALWAYS
};

struct stored_control
{
    char *args;
    char *control_name;
    char *control_value;
    unsigned int v4l2;
    int value;
    enum control_apply_type apply_type;
};

struct stored_controls
{
    struct stored_control controls[256];
    int length;
};

// PROCESSING
struct processing
{
    struct endpoint source;
    struct endpoint target;
    struct configfs configfs;
    struct controls controls;
    struct stored_controls *stored_controls;
    struct events events;
    struct settings settings;
    struct internals internals;
    struct uvc_request uvc_request;
    struct data_buffers data_buffers;
};

// RGB to YUYV

struct px24 {
    uint8_t ignore1 : 1;
    uint8_t r : 7;
    uint8_t ignore2 : 1;
    uint8_t g : 7;
    uint8_t ignore3 : 1;
    uint8_t b : 7;
};

struct py24 {
    uint8_t ignore1 : 2;
    uint8_t r : 6;
    uint8_t ignore2 : 1;
    uint8_t g : 7;
    uint8_t ignore3 : 3;
    uint8_t b : 5;
};

struct rgb_pixel24 {
    union {
        struct px24 x1;
        struct py24 y1;
        unsigned int rgba1;
    };
    union {
        struct px24 x2;
        struct py24 y2;
        unsigned int rgba2;
    };
};

struct px32 {
    uint8_t ignore1 : 1;
    uint8_t r : 7;
    uint8_t ignore2 : 1;
    uint8_t g : 7;
    uint8_t ignore3 : 1;
    uint8_t b : 7;
    uint8_t a;
};

struct py32 {
    uint8_t ignore1 : 2;
    uint8_t r : 6;
    uint8_t ignore2 : 1;
    uint8_t g : 7;
    uint8_t ignore3 : 3;
    uint8_t b : 5;
    uint8_t a;
};

struct rgb_pixel32 {
    union {
        struct px32 x1;
        struct py32 y1;
        unsigned int rgba1;
    };
    union {
        struct px32 x2;
        struct py32 y2;
        unsigned int rgba2;
    };
};

struct yuyv_pixel {
    union {
        struct z {
            uint8_t y1;
            uint8_t u;
            uint8_t y2;
            uint8_t v;
        } z;
        unsigned int yuyv;
    };
};


#endif // end HEADERS
