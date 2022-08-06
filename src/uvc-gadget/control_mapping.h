
#ifndef CONTROL_MAPPING
#define CONTROL_MAPPING

#include "headers.h"

struct control_mapping_pair control_mapping[] = {
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_BACKLIGHT_COMPENSATION_CONTROL,
        .v4l2 = V4L2_CID_BACKLIGHT_COMPENSATION,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_BRIGHTNESS_CONTROL,
        .v4l2 = V4L2_CID_BRIGHTNESS,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_CONTRAST_CONTROL,
        .v4l2 = V4L2_CID_CONTRAST,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_GAIN_CONTROL,
        .v4l2 = V4L2_CID_GAIN,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_POWER_LINE_FREQUENCY_CONTROL,
        .v4l2 = V4L2_CID_POWER_LINE_FREQUENCY,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_HUE_CONTROL,
        .v4l2 = V4L2_CID_HUE,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_SATURATION_CONTROL,
        .v4l2 = V4L2_CID_SATURATION,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_SHARPNESS_CONTROL,
        .v4l2 = V4L2_CID_SHARPNESS,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_GAMMA_CONTROL,
        .v4l2 = V4L2_CID_GAMMA,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL,
        .v4l2 = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL,
        .v4l2 = V4L2_CID_RED_BALANCE,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL,
        .v4l2 = V4L2_CID_AUTO_WHITE_BALANCE,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_DIGITAL_MULTIPLIER_CONTROL,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_HUE_AUTO_CONTROL,
        .v4l2 = V4L2_CID_HUE_AUTO,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL,
    },
    {
        .type = UVC_VC_PROCESSING_UNIT,
        .uvc = UVC_PU_ANALOG_LOCK_STATUS_CONTROL,
    },
    // {
    //     .type = UVC_VC_PROCESSING_UNIT,
    //     .uvc = UVC_PU_CONTRAST_AUTO_CONTROL,
    // },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_CONTROL_UNDEFINED,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_SCANNING_MODE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_AE_MODE_CONTROL,
        .v4l2 = V4L2_CID_EXPOSURE_AUTO,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_AE_PRIORITY_CONTROL,
        .v4l2 = V4L2_CID_EXPOSURE_AUTO_PRIORITY,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL,
        .v4l2 = V4L2_CID_EXPOSURE_ABSOLUTE,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_FOCUS_ABSOLUTE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_FOCUS_RELATIVE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_FOCUS_AUTO_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_IRIS_ABSOLUTE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_IRIS_RELATIVE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_ZOOM_ABSOLUTE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_ZOOM_RELATIVE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_PANTILT_ABSOLUTE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_PANTILT_RELATIVE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_ROLL_ABSOLUTE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_ROLL_RELATIVE_CONTROL,
    },
    {
        .type = UVC_VC_INPUT_TERMINAL,
        .uvc = UVC_CT_PRIVACY_CONTROL,
    }};

int control_mapping_size = sizeof(control_mapping) / sizeof(*control_mapping);

#endif // end CONTROL_MAPPING
