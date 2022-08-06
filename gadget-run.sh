#!/usr/bin/bash

GADGET_NAME="g1"
SERIAL_NUMBER="100000000d2386db"
MANUFACTURER="USB-Gadget"
PRODUCT_NAME="USB-Gadget"
# UVC_DEVICE="/dev/video1"
UVC_DEVICE_AUTODETECT=true
UVC_GADGET_PATH="${PWD}"

echo "INFO:  --- Gadget init ---"

if [ $(id -u) -ne 0 ]
then
    echo "Please run as root"
    exit
fi

CONFIGFS_PATH=$(findmnt -t configfs -n --output=target)
GADGET_PATH="${CONFIGFS_PATH}/usb_gadget/${GADGET_NAME}"

echo "INFO:  Gadget config path: ${GADGET_PATH}"

UDC_INTERFACE=$(ls /sys/class/udc)
if [ -z "${UDC_INTERFACE}" ]; then
    echo "ERROR: UDC interface: NOT FOUND"
else
    echo "INFO:  UDC interface: ${UDC_INTERFACE}"
fi

if [ -e "/sys/class/udc/$UDC_INTERFACE/uevent" ]; then
    . "/sys/class/udc/$UDC_INTERFACE/uevent"
fi

# Include configuration files
for CONFIG_PATH in "$@"
do

    if [ "${CONFIG_PATH}" = "init" ]; then
        INIT_ONLY=true

    elif [ "${CONFIG_PATH}" = "fps" ]; then
        SHOW_FPS=true

    elif [ -e "${CONFIG_PATH}" ]; then
        echo "INFO:  Read config: ${CONFIG_PATH}"
        source "${CONFIG_PATH}"
    else
        echo "ERROR: Config '${CONFIG_PATH}' NOT FOUND"
    fi
done

uvc_config_frame () {
    FORMAT=$1
    NAME=$2
    FPS=$3
    WIDTH=$4
    HEIGHT=$5

    FRAMEDIR=$GADGET_PATH/functions/uvc.usb0/streaming/$FORMAT/$NAME/${WIDTH}x${HEIGHT}p

    mkdir -p $FRAMEDIR

    FRAME_INTERVAL=$((10000000 / $FPS))

    echo $WIDTH > $FRAMEDIR/wWidth
    echo $HEIGHT > $FRAMEDIR/wHeight
    echo $FRAME_INTERVAL > $FRAMEDIR/dwDefaultFrameInterval
    echo $(($WIDTH * $HEIGHT * 80)) > $FRAMEDIR/dwMinBitRate
    echo $(($WIDTH * $HEIGHT * 160)) > $FRAMEDIR/dwMaxBitRate
    echo $(($WIDTH * $HEIGHT * 2)) > $FRAMEDIR/dwMaxVideoFrameBufferSize
    cat <<EOF > $FRAMEDIR/dwFrameInterval
${FRAME_INTERVAL}
EOF
}

# Gadget configfs initialization

if [ -z "${USB_UDC_DRIVER}" ]; then

    mkdir -p $GADGET_PATH

    echo 0x1d6b > $GADGET_PATH/idVendor
    echo 0x0104 > $GADGET_PATH/idProduct
    echo 0x0100 > $GADGET_PATH/bcdDevice
    echo 0x0200 > $GADGET_PATH/bcdUSB

    echo 0xEF   > $GADGET_PATH/bDeviceClass
    echo 0x02   > $GADGET_PATH/bDeviceSubClass
    echo 0x01   > $GADGET_PATH/bDeviceProtocol

    mkdir -p $GADGET_PATH/strings/0x409
    echo "${SERIAL_NUMBER}" > $GADGET_PATH/strings/0x409/serialnumber
    echo "${MANUFACTURER}"  > $GADGET_PATH/strings/0x409/manufacturer
    echo "${PRODUCT_NAME}"  > $GADGET_PATH/strings/0x409/product

    mkdir -p $GADGET_PATH/configs/c.1
    mkdir -p $GADGET_PATH/configs/c.1/strings/0x409
    echo 500 > $GADGET_PATH/configs/c.1/MaxPower

    if [ "${INIT_UVC}" = true ]; then

        echo "INFO:  UVC init"

        MJPEG=false
        UNCOMPRESSED=false

        UVC_FUNCTION_PATH="${GADGET_PATH}/functions/uvc.usb0"

        mkdir -p "${UVC_FUNCTION_PATH}"

        mkdir -p "${UVC_FUNCTION_PATH}/control/header/h"
        ln -s "${UVC_FUNCTION_PATH}/control/header/h" "${UVC_FUNCTION_PATH}/control/class/fs/h"
        ln -s "${UVC_FUNCTION_PATH}/control/header/h" "${UVC_FUNCTION_PATH}/control/class/ss/h"

        if [ -z "${UVC_FORMATS}" ]; then
            echo "ERROR: Missing UVC_FORMATS"
            exit 2
        fi

        if [ ! -z "${STREAMING_MAXPACKET}" ]; then
            echo "${STREAMING_MAXPACKET}" > "${UVC_FUNCTION_PATH}/streaming_maxpacket"
        fi


        for FORMAT in "${UVC_FORMATS[@]}"; do
            FORMAT_ARRAY=($(echo "${FORMAT}" | tr ':' '\n'))

            echo "INFO:  Frame format - format: ${FORMAT_ARRAY[0]}, fps: ${FORMAT_ARRAY[2]}, width: ${FORMAT_ARRAY[3]}, height: ${FORMAT_ARRAY[4]}"

            uvc_config_frame "${FORMAT_ARRAY[0]}" "${FORMAT_ARRAY[1]}" "${FORMAT_ARRAY[2]}" "${FORMAT_ARRAY[3]}" "${FORMAT_ARRAY[4]}"

            if [ "${FORMAT_ARRAY[0]}" = "mjpeg" ]; then
                MJPEG=true
            fi

            if [ "${FORMAT_ARRAY[0]}" = "uncompressed" ]; then
                UNCOMPRESSED=true
            fi
        done

        mkdir -p "${UVC_FUNCTION_PATH}/streaming/header/h"
        cd "${UVC_FUNCTION_PATH}/streaming/header/h"

        if [ "${MJPEG}" = true ]; then
            ln -s "${UVC_FUNCTION_PATH}/streaming/mjpeg/m"
        fi

        if [ "${UNCOMPRESSED}" = true ]; then
            ln -s "${UVC_FUNCTION_PATH}/streaming/uncompressed/u"
        fi

        cd "${UVC_FUNCTION_PATH}/streaming/class/fs"
        ln -s ../../header/h

        cd "${UVC_FUNCTION_PATH}/streaming/class/hs"
        ln -s ../../header/h

        cd "${UVC_FUNCTION_PATH}/streaming/class/ss"
        ln -s ../../header/h

        ln -s "${UVC_FUNCTION_PATH}" $GADGET_PATH/configs/c.1/uvc.usb0

    fi

    if [ "${INIT_AUDIO}" = true ]; then

        echo "INFO:  AUDIO init"

        UAC2_FUNCTION_PATH="${GADGET_PATH}/functions/uac2.usb0"

        mkdir -p "${UAC2_FUNCTION_PATH}"

        if [ ! -z "${AUDIO_CHANNEL_MASK}" ]; then
            echo "${AUDIO_CHANNEL_MASK}" > "${UVC_FUNCTION_PATH}/c_chmask"
            echo "${AUDIO_CHANNEL_MASK}" > "${UVC_FUNCTION_PATH}/p_chmask"
        else
            echo "INFO: Default AUDIO_CHANNEL_MASK = 2"
            echo "2" > "${UVC_FUNCTION_PATH}/c_chmask"
            echo "2" > "${UVC_FUNCTION_PATH}/p_chmask"
        fi

        if [ ! -z "${AUDIO_SAMPLE_RATES}" ]; then
            echo "${AUDIO_SAMPLE_RATES}" > "${UVC_FUNCTION_PATH}/c_srate"
            echo "${AUDIO_SAMPLE_RATES}" > "${UVC_FUNCTION_PATH}/p_srate"
        else
            echo "INFO: Default AUDIO_SAMPLE_RATES = 2"
            echo "44100" > "${UVC_FUNCTION_PATH}/c_srate"
            echo "44100" > "${UVC_FUNCTION_PATH}/p_srate"
        fi

        if [ ! -z "${AUDIO_SAMPLE_SIZE}" ]; then
            echo "${AUDIO_SAMPLE_SIZE}" > "${UVC_FUNCTION_PATH}/c_ssize"
            echo "${AUDIO_SAMPLE_SIZE}" > "${UVC_FUNCTION_PATH}/p_ssize"
        else
            echo "INFO: Default AUDIO_SAMPLE_SIZE = 2"
            echo "2" > "${UVC_FUNCTION_PATH}/c_srate"
            echo "2" > "${UVC_FUNCTION_PATH}/p_srate"
        fi
 
        ln -s "${UAC2_FUNCTION_PATH}" $GADGET_PATH/configs/c.1/uac2.usb0

    fi

    if [ "${INIT_MIDI}" = true ]; then

        echo "INFO:  MIDI init"

        MIDI_FUNCTION_PATH="${GADGET_PATH}/functions/midi.usb0"

        mkdir -p "${MIDI_FUNCTION_PATH}"

        if [ ! -z "${MIDI_ID}" ]; then
            echo "${MIDI_ID}" > "${UVC_FUNCTION_PATH}/id"
        else
            echo "INFO: Default MIDI_ID = MIDI"
            echo "MIDI" > "${UVC_FUNCTION_PATH}/id"
        fi

        # index     - index value for the USB MIDI adapter
        # id        - ID string for the USB MIDI adapter
        # buflen    - MIDI buffer length
        # qlen      - USB read request queue length
        # in_ports  - number of MIDI input ports
        # out_ports - number of MIDI output ports


        ln -s "${MIDI_FUNCTION_PATH}" ${GADGET_PATH}/configs/c.1/midi.usb0

    fi

    # if [ "${INIT_MOUSE}" = true ]; then

    #     echo "INFO:  MOUSE init"

    #     MOUSE_FUNCTION_PATH="${GADGET_PATH}/functions/hid.usb0"

    #     mkdir -p "${MOUSE_FUNCTION_PATH}"

    #     # echo 0x1d6b > $GADGET_PATH/idVendor
    #     # echo 0x0104 > $GADGET_PATH/idProduct
    #     # echo 0x0001 > $GADGET_PATH/bcdDevice
    #     # echo 0x0200 > $GADGET_PATH/bcdUSB

    #     # echo 0xEF   > $GADGET_PATH/bDeviceClass
    #     # echo 0x00   > $GADGET_PATH/bDeviceSubClass
    #     # echo 0x00   > $GADGET_PATH/bDeviceProtocol

    #     # echo 0x40 > $GADGET_PATH/bMaxPacketSize0

    #     echo 1 > "${MOUSE_FUNCTION_PATH}/protocol"
    #     echo 0 > "${MOUSE_FUNCTION_PATH}/subclass"
    #     echo 8 > "${MOUSE_FUNCTION_PATH}/report_length"

    #     echo -ne \\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x75\\x08\\x81\\x03\\x95\\x05\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x95\\x01\\x75\\x03\\x91\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\x65\\x05\\x07\\x19\\x00\\x29\\x65\\x81\\x00\\xc0 > "${MOUSE_FUNCTION_PATH}/report_desc"
    #     # echo -ne \\x05\\x01\\x09\\x02\\xa1\\x01\\x09\\x01\\xa1\\x00\\x05\\x09\\x19\\x01\\x29\\x03\\x15\\x00\\x25\\x01\\x95\\x03\\x75\\x01\\x81\\x02\\x95\\x01\\x75\\x05\\x81\\x03\\x05\\x01\\x09\\x30\\x09\\x31\\x15\\x81\\x25\\x7f\\x75\\x08\\x95\\x02\\x81\\x06\\xc0\\xc0 > "${MOUSE_FUNCTION_PATH}/report_desc"

    #     # ln -s "${MOUSE_FUNCTION_PATH}" ${GADGET_PATH}/configs/c.1/hid.usb0
    #     ln -s "${MOUSE_FUNCTION_PATH}" ${GADGET_PATH}/configs/c.1/

    # fi

    if [ "${INIT_KEYBOARD}" = true ]; then

        echo "INFO:  KEYBOARD init"

        KEYBOARD_FUNCTION_PATH="${GADGET_PATH}/functions/hid.usb0"

        mkdir -p "${KEYBOARD_FUNCTION_PATH}"

        echo 1 > "${KEYBOARD_FUNCTION_PATH}/protocol"
        echo 0 > "${KEYBOARD_FUNCTION_PATH}/subclass"
        echo 8 > "${KEYBOARD_FUNCTION_PATH}/report_length"

        echo -ne \\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x75\\x08\\x81\\x03\\x95\\x05\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x95\\x01\\x75\\x03\\x91\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\x65\\x05\\x07\\x19\\x00\\x29\\x65\\x81\\x00\\xc0 > "${KEYBOARD_FUNCTION_PATH}/report_desc"

        ln -s "${KEYBOARD_FUNCTION_PATH}" ${GADGET_PATH}/configs/c.1/hid.usb0

    fi

    if [ "${INIT_CDROM}" = true ]; then

        echo "INFO:  CDROM init"

        CDROM_FUNCTION_PATH="${GADGET_PATH}/functions/mass_storage.usb0"

        mkdir -p "${CDROM_FUNCTION_PATH}"

        if [ ! -z "${ISO_FILE_0}" ]; then
            LUN0_FUNCTION_PATH="${CDROM_FUNCTION_PATH}/lun.0"
            mkdir -p "${LUN0_FUNCTION_PATH}"

            echo 0 > "${LUN0_FUNCTION_PATH}/nofua"
            echo 1 > "${LUN0_FUNCTION_PATH}/cdrom"
            echo 1 > "${LUN0_FUNCTION_PATH}/removable"
            echo 1 > "${LUN0_FUNCTION_PATH}/ro"
            echo "${ISO_FILE_0}" > "${LUN0_FUNCTION_PATH}/file"

        fi

        if [ ! -z "${ISO_FILE_1}" ]; then
            LUN1_FUNCTION_PATH="${CDROM_FUNCTION_PATH}/lun.1"
            mkdir -p "${LUN1_FUNCTION_PATH}"

            echo 0 > "${LUN1_FUNCTION_PATH}/nofua"
            echo 1 > "${LUN1_FUNCTION_PATH}/cdrom"
            echo 1 > "${LUN1_FUNCTION_PATH}/removable"
            echo 1 > "${LUN1_FUNCTION_PATH}/ro"
            echo "${ISO_FILE_1}" > "${LUN1_FUNCTION_PATH}/file"

        fi
    fi

    if [ "${INIT_MASS_STORAGE}" = true ]; then

        echo "INFO:  MASS_STORAGE init"

        MASS_STORAGE_FUNCTION_PATH="${GADGET_PATH}/functions/mass_storage.usb0"

        mkdir -p "${MASS_STORAGE_FUNCTION_PATH}"

        if [ ! -z "${MASS_STORAGE_FILE_0}" ]; then
            LUN0_FUNCTION_PATH="${MASS_STORAGE_FUNCTION_PATH}/lun.0"
            mkdir -p "${LUN0_FUNCTION_PATH}"

            echo 0 > "${LUN0_FUNCTION_PATH}/nofua"
            echo 0 > "${LUN0_FUNCTION_PATH}/cdrom"
            echo 1 > "${LUN0_FUNCTION_PATH}/removable"
            echo 0 > "${LUN0_FUNCTION_PATH}/ro"
            echo "${MASS_STORAGE_FILE_0}" > "${LUN0_FUNCTION_PATH}/file"

        fi

        if [ ! -z "${MASS_STORAGE_FILE_1}" ]; then
            LUN1_FUNCTION_PATH="${CDROM_FUNCTION_PATH}/lun.1"
            mkdir -p "${LUN1_FUNCTION_PATH}"

            echo 0 > "${LUN1_FUNCTION_PATH}/nofua"
            echo 0 > "${LUN1_FUNCTION_PATH}/cdrom"
            echo 1 > "${LUN1_FUNCTION_PATH}/removable"
            echo 0 > "${LUN1_FUNCTION_PATH}/ro"
            echo "${MASS_STORAGE_FILE_1}" > "${LUN1_FUNCTION_PATH}/file"

        fi
    fi

    if [ "${INIT_CDROM}" = true ] || [ "${INIT_MASS_STORAGE}" = true ]; then
        ln -s "${GADGET_PATH}/functions/mass_storage.usb0" ${GADGET_PATH}/configs/c.1/mass_storage.usb0

    fi

    if [ "${INIT_SERIAL}" = true ]; then

        echo "INFO:  SERIAL init"

        mkdir -p $GADGET_PATH/functions/acm.usb0
        ln -s $GADGET_PATH/functions/acm.usb0 $GADGET_PATH/configs/c.1/acm.usb0

    fi

    udevadm settle -t 5 || :
    ls /sys/class/udc > $GADGET_PATH/UDC

fi

echo "INFO:  --- Gadget initialization finish ---"

if [ "${INIT_UVC}" = true ]; then
    echo "INFO:  --- Start UVC-GADGET ---"

    UVC_GADGET_PARAMS=()

    if [ "${FB_DEVICE}" ]; then
        echo "INFO:  Framebuffer device: ${FB_DEVICE}"
        UVC_GADGET_PARAMS+=("-f")
        UVC_GADGET_PARAMS+=("${FB_DEVICE}")

    fi

    if [ "${IMAGE_PATH}" ]; then
        echo "INFO:  Image path: ${IMAGE_PATH}"
        UVC_GADGET_PARAMS+=("-i")
        UVC_GADGET_PARAMS+=("${IMAGE_PATH}")

    fi

    if [ "${UVC_DEVICE_AUTODETECT}" = true ]; then
        echo "INFO:  Autodetect UVC device"
       UVC_GADGET_PARAMS+=("-a")

    fi

    if [ "${UVC_DEVICE}" ]; then
        echo "INFO:  UVC device: ${UVC_DEVICE}"
       UVC_GADGET_PARAMS+=("-u")
       UVC_GADGET_PARAMS+=("${UVC_DEVICE}")

    fi

    if [ "${V4L2_DEVICE}" ]; then
        echo "INFO:  V4L2 device: ${V4L2_DEVICE}"
        UVC_GADGET_PARAMS+=("-v")
        UVC_GADGET_PARAMS+=("${V4L2_DEVICE}")

    fi

    if [ "${STDIN_STREAM}" ]; then
        echo "INFO:  Stream: ${STDIN_STREAM}"
        UVC_GADGET_PARAMS+=("-s")
        UVC_GADGET_PARAMS+=("${STDIN_STREAM}")

    fi

    if [ "${STDIN_WIDTH}" ]; then
        echo "INFO:  Stream width x height: ${STDIN_WIDTH}x${STDIN_HEIGHT}"
        UVC_GADGET_PARAMS+=("-m")
        UVC_GADGET_PARAMS+=("${STDIN_WIDTH}x${STDIN_HEIGHT}")

    fi

    if [ "${SET_FPS}" ]; then
        echo "INFO:  Set FPS: ${SET_FPS}"
        UVC_GADGET_PARAMS+=("-r")
        UVC_GADGET_PARAMS+=("${SET_FPS}")

    fi

    if [ "${SHOW_FPS}" ]; then
        echo "INFO:  Show FPS: ${SHOW_FPS}"
        UVC_GADGET_PARAMS+=("-x")

    fi

    cd "${UVC_GADGET_PATH}"

    if [ "${INIT_ONLY}" ]; then
        echo "EXECUTE: ./uvc-gadget" "${UVC_GADGET_PARAMS[@]}"

    else
        ./uvc-gadget -n 10 "${UVC_GADGET_PARAMS[@]}" --acontrol video_bitrate=25000000

    fi

fi

echo "INFO:  --- Gadget finish ---"
