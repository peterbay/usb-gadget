
#include "headers.h"

char *uvc_device_detect()
{
    int fd;
    struct dirent *ep;
    struct v4l2_capability cap;
    char path[80];
    char *uvc_device = NULL;
    DIR *dp = opendir("/dev");

    if (dp != NULL)
    {
        while ((ep = readdir(dp)))
        {
            if (strncmp(ep->d_name, "video", 5))
            {
                continue;
            }

            strcpy(path, "/dev/");
            strcat(path, ep->d_name);

            fd = open(path, O_RDWR | O_NONBLOCK);
            if (!fd)
            {
                continue;
            }

            if (ioctl(fd, VIDIOC_QUERYCAP, &cap) > -1)
            {
                if ((cap.capabilities & V4L2_CAP_VIDEO_OUTPUT))
                {
                    if (!strncmp((char *)cap.bus_info, "gadget", 6))
                    {
                        uvc_device = strdup(path);
                        break;
                    }
                }
            }

            close(fd);
        }
        closedir(dp);
    }

    return uvc_device;
}
