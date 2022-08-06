CROSS_COMPILE	?= 

CC			?= $(CROSS_COMPILE)gcc
CFLAGS		?= -W -Wall -g -O3
LDFLAGS		?= -g

OBJFILES = src/uvc-gadget/configfs.o                   \
			src/uvc-gadget/data_buffers.o              \
			src/uvc-gadget/processing.o                \
			src/uvc-gadget/v4l2_endpoint.o             \
			src/uvc-gadget/fb_endpoint.o               \
			src/uvc-gadget/image_endpoint.o            \
			src/uvc-gadget/stdin_endpoint.o            \
			src/uvc-gadget/processing_actions.o        \
			src/uvc-gadget/processing_fb_uvc.o         \
			src/uvc-gadget/processing_image_uvc.o      \
			src/uvc-gadget/processing_v4l2_uvc.o       \
			src/uvc-gadget/processing_stdin_uvc.o      \
			src/uvc-gadget/uvc_device_detect.o         \
			src/uvc-gadget/uvc_endpoint.o              \
			src/uvc-gadget/uvc_control.o               \
			src/uvc-gadget/uvc_events.o                \
			src/uvc-gadget/uvc_names.o                 \
			src/uvc-gadget/v4l2_names.o                \
			src/uvc-gadget/system.o                    \
			src/uvc-gadget/uvc-gadget.o

TARGET = uvc-gadget

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~
