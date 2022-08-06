
#include "uvc_names.h"

char *uvc_request_code_name(unsigned int control)
{
    switch (control)
    {
    case UVC_RC_UNDEFINED:
        return "RC_UNDEFINED";

    case UVC_SET_CUR:
        return "SET_CUR";

    case UVC_GET_CUR:
        return "GET_CUR";

    case UVC_GET_MIN:
        return "GET_MIN";

    case UVC_GET_MAX:
        return "GET_MAX";

    case UVC_GET_RES:
        return "GET_RES";

    case UVC_GET_LEN:
        return "GET_LEN";

    case UVC_GET_INFO:
        return "GET_INFO";

    case UVC_GET_DEF:
        return "GET_DEF";

    default:
        return "UNKNOWN";
    }
}

char *uvc_vs_interface_control_name(unsigned int interface)
{
    switch (interface)
    {
    case UVC_VS_CONTROL_UNDEFINED:
        return "CONTROL_UNDEFINED";

    case UVC_VS_PROBE_CONTROL:
        return "PROBE";

    case UVC_VS_COMMIT_CONTROL:
        return "COMMIT";

    case UVC_VS_STILL_PROBE_CONTROL:
        return "STILL_PROBE";

    case UVC_VS_STILL_COMMIT_CONTROL:
        return "STILL_COMMIT";

    case UVC_VS_STILL_IMAGE_TRIGGER_CONTROL:
        return "STILL_IMAGE_TRIGGER";

    case UVC_VS_STREAM_ERROR_CODE_CONTROL:
        return "STREAM_ERROR_CODE";

    case UVC_VS_GENERATE_KEY_FRAME_CONTROL:
        return "GENERATE_KEY_FRAME";

    case UVC_VS_UPDATE_FRAME_SEGMENT_CONTROL:
        return "UPDATE_FRAME_SEGMENT";

    case UVC_VS_SYNC_DELAY_CONTROL:
        return "SYNC_DELAY";

    default:
        return "UNKNOWN";
    }
}

char *uvc_processing_control_name(unsigned int control)
{
    switch (control)
    {

    case UVC_PU_CONTROL_UNDEFINED:
        return "UVC_PU_CONTROL_UNDEFINED";

    case UVC_PU_BACKLIGHT_COMPENSATION_CONTROL:
        return "UVC_PU_BACKLIGHT_COMPENSATION_CONTROL";

    case UVC_PU_BRIGHTNESS_CONTROL:
        return "UVC_PU_BRIGHTNESS_CONTROL";

    case UVC_PU_CONTRAST_CONTROL:
        return "UVC_PU_CONTRAST_CONTROL";

    case UVC_PU_GAIN_CONTROL:
        return "UVC_PU_GAIN_CONTROL";

    case UVC_PU_POWER_LINE_FREQUENCY_CONTROL:
        return "UVC_PU_POWER_LINE_FREQUENCY_CONTROL";

    case UVC_PU_HUE_CONTROL:
        return "UVC_PU_HUE_CONTROL";

    case UVC_PU_SATURATION_CONTROL:
        return "UVC_PU_SATURATION_CONTROL";

    case UVC_PU_SHARPNESS_CONTROL:
        return "UVC_PU_SHARPNESS_CONTROL";

    case UVC_PU_GAMMA_CONTROL:
        return "UVC_PU_GAMMA_CONTROL";

    case UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
        return "UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL";

    case UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
        return "UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL";

    case UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL:
        return "UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL";

    case UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL:
        return "UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL";

    case UVC_PU_DIGITAL_MULTIPLIER_CONTROL:
        return "UVC_PU_DIGITAL_MULTIPLIER_CONTROL";

    case UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL:
        return "UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL";

    case UVC_PU_HUE_AUTO_CONTROL:
        return "UVC_PU_HUE_AUTO_CONTROL";

    case UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL:
        return "UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL";

    case UVC_PU_ANALOG_LOCK_STATUS_CONTROL:
        return "UVC_PU_ANALOG_LOCK_STATUS_CONTROL";

    default:
        return "UNKNOWN";
    }
}

char *uvc_terminal_control_name(unsigned int control)
{
    switch (control)
    {
    case UVC_CT_CONTROL_UNDEFINED:
        return "UVC_CT_CONTROL_UNDEFINED";

    case UVC_CT_SCANNING_MODE_CONTROL:
        return "UVC_CT_SCANNING_MODE_CONTROL";

    case UVC_CT_AE_MODE_CONTROL:
        return "UVC_CT_AE_MODE_CONTROL";

    case UVC_CT_AE_PRIORITY_CONTROL:
        return "UVC_CT_AE_PRIORITY_CONTROL";

    case UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
        return "UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL";

    case UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL:
        return "UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL";

    case UVC_CT_FOCUS_ABSOLUTE_CONTROL:
        return "UVC_CT_FOCUS_ABSOLUTE_CONTROL";

    case UVC_CT_FOCUS_RELATIVE_CONTROL:
        return "UVC_CT_FOCUS_RELATIVE_CONTROL";

    case UVC_CT_FOCUS_AUTO_CONTROL:
        return "UVC_CT_FOCUS_AUTO_CONTROL";

    case UVC_CT_IRIS_ABSOLUTE_CONTROL:
        return "UVC_CT_IRIS_ABSOLUTE_CONTROL";

    case UVC_CT_IRIS_RELATIVE_CONTROL:
        return "UVC_CT_IRIS_RELATIVE_CONTROL";

    case UVC_CT_ZOOM_ABSOLUTE_CONTROL:
        return "UVC_CT_ZOOM_ABSOLUTE_CONTROL";

    case UVC_CT_ZOOM_RELATIVE_CONTROL:
        return "UVC_CT_ZOOM_RELATIVE_CONTROL";

    case UVC_CT_PANTILT_ABSOLUTE_CONTROL:
        return "UVC_CT_PANTILT_ABSOLUTE_CONTROL";

    case UVC_CT_PANTILT_RELATIVE_CONTROL:
        return "UVC_CT_PANTILT_RELATIVE_CONTROL";

    case UVC_CT_ROLL_ABSOLUTE_CONTROL:
        return "UVC_CT_ROLL_ABSOLUTE_CONTROL";

    case UVC_CT_ROLL_RELATIVE_CONTROL:
        return "UVC_CT_ROLL_RELATIVE_CONTROL";

    case UVC_CT_PRIVACY_CONTROL:
        return "UVC_CT_PRIVACY_CONTROL";

    default:
        return "UNKNOWN";
    }
}

char *uvc_control_name(unsigned int control_type,
                       unsigned int control)
{
    if (control_type == UVC_VC_INPUT_TERMINAL)
    {
        return uvc_terminal_control_name(control);
    }
    else if (control_type == UVC_VC_PROCESSING_UNIT)
    {
        return uvc_processing_control_name(control);
    }
    return "UNKNOWN TYPE";
}
