
import time
import serial
#import mouse
#import pyautogui
import uinput
import evdev
import sys

# devices = [evdev.InputDevice(path) for path in evdev.list_devices()]
# for device in devices:
# 	print(device.path, device.name, device.phys)
# sys.exit()

class RemoteControl:

	keyMapping = [
		uinput.KEY_BACKSPACE, uinput.KEY_TAB, uinput.KEY_ENTER, uinput.KEY_LEFTSHIFT, uinput.KEY_RIGHTSHIFT,
		uinput.KEY_LEFTCTRL, uinput.KEY_RIGHTCTRL, uinput.KEY_LEFTALT, uinput.KEY_RIGHTALT, uinput.KEY_PAUSE,
		uinput.KEY_CAPSLOCK, uinput.KEY_ESC, uinput.KEY_SPACE, uinput.KEY_PAGEUP, uinput.KEY_PAGEDOWN,
		uinput.KEY_END, uinput.KEY_HOME, uinput.KEY_LEFT, uinput.KEY_UP, uinput.KEY_RIGHT,
		uinput.KEY_DOWN, uinput.KEY_PRINT, uinput.KEY_INSERT, uinput.KEY_DELETE, uinput.KEY_0,
		uinput.KEY_1, uinput.KEY_2, uinput.KEY_3, uinput.KEY_4, uinput.KEY_5,
		uinput.KEY_6, uinput.KEY_7, uinput.KEY_8, uinput.KEY_9, uinput.KEY_A,
		uinput.KEY_B, uinput.KEY_C, uinput.KEY_D, uinput.KEY_E, uinput.KEY_F,
		uinput.KEY_G, uinput.KEY_H, uinput.KEY_I, uinput.KEY_J, uinput.KEY_K,
		uinput.KEY_L, uinput.KEY_M, uinput.KEY_N, uinput.KEY_O, uinput.KEY_P,
		uinput.KEY_Q, uinput.KEY_R, uinput.KEY_S, uinput.KEY_T, uinput.KEY_U,
		uinput.KEY_V, uinput.KEY_W, uinput.KEY_X, uinput.KEY_Y, uinput.KEY_Z,
		uinput.KEY_LEFTMETA, uinput.KEY_RIGHTMETA, uinput.KEY_CONTEXT_MENU, uinput.KEY_KP0, uinput.KEY_KP1,
		uinput.KEY_KP2, uinput.KEY_KP3, uinput.KEY_KP4, uinput.KEY_KP5, uinput.KEY_KP6,
		uinput.KEY_KP7, uinput.KEY_KP8, uinput.KEY_KP9, uinput.KEY_F1, uinput.KEY_F2,
		uinput.KEY_F3, uinput.KEY_F4, uinput.KEY_F5, uinput.KEY_F6, uinput.KEY_F7,
		uinput.KEY_F8, uinput.KEY_F9, uinput.KEY_F10, uinput.KEY_F11, uinput.KEY_F12,
		uinput.KEY_NUMLOCK, uinput.KEY_SCROLLLOCK, uinput.KEY_SEMICOLON, uinput.KEY_EQUAL, uinput.KEY_COMMA,
		uinput.KEY_MINUS, uinput.KEY_BACKSLASH, uinput.KEY_SLASH, uinput.KEY_LEFTBRACE, uinput.KEY_RIGHTBRACE,
		uinput.KEY_KPASTERISK, uinput.KEY_KPPLUS, uinput.KEY_KPMINUS, uinput.KEY_KPDOT, uinput.KEY_KPSLASH,
		uinput.KEY_DOT, uinput.KEY_GRAVE, uinput.KEY_APOSTROPHE,
		uinput.ABS_X + (0, 800, 0, 0),
		uinput.ABS_Y + (0, 600, 0, 0),
		uinput.BTN_LEFT,
		uinput.BTN_RIGHT,
		uinput.REL_WHEEL
	]

	def __init__(self):
		self.ser = serial.Serial('/dev/ttyGS0', baudrate=115200)  # open serial port
		print(self.ser.name) # check which port was really used
		self.buffer = None

		self.packetLengths = {
			0x4B: {
				'length': 6,
				'callback': self.applyKeyboard
			},
			0x4D: {
				'length': 10,
				'callback': self.applyMouse
			},
			0x57: {
				'length': 6,
				'callback': self.applyWheel
			},
		}

	def decode7bitNumber(self, data):
		if len(data) < 2:
			print('wrong 7bit data', data)
			return None

		return ((data[0] & 0x7f) << 7) + (data[1] & 0x7f)

	def applyModifiers(self, modifiers):
		print(modifiers)

	def applyMouse(self, data):
		eventId = data[0]
		button = data[1]
		modifiers = data[2]
		x = self.decode7bitNumber(data[3:5])
		y = self.decode7bitNumber(data[5:7])

		if x is None or y is None:
			print('broken mouse data', data)
			return

		self.device.emit(uinput.ABS_X, x, syn=False)
		self.device.emit(uinput.ABS_Y, y)

		if eventId == 1:
			if button == 0:
				self.device.emit(uinput.BTN_LEFT, 1)
			if button == 2:
				self.device.emit(uinput.BTN_RIGHT, 1)

		elif eventId == 2:
			if button == 0:
				self.device.emit(uinput.BTN_LEFT, 0)
			if button == 2:
				self.device.emit(uinput.BTN_RIGHT, 0)

	def applyKeyboard(self, data):
		eventId = data[0]
		index = data[1]
		modifiers = data[2]
		if index < len(self.keyMapping):
			keyCode = self.keyMapping[index]
			# print(index, eventId, keyCode)
			if eventId == 0:
				self.device.emit(keyCode, 1)

			if eventId == 1:
				self.device.emit(keyCode, 0)

	def applyWheel(self, data):
		direction = data[0]
		delta = self.decode7bitNumber(data[1:3])
		if direction:
			self.device.emit(uinput.REL_WHEEL, -1)
		else:
			self.device.emit(uinput.REL_WHEEL, 1)

	def parseBuffer(self):
		status = False

		while len(self.buffer) and self.buffer[0] != 0xff:
			print('drop', hex(self.buffer[0]))
			self.buffer = self.buffer[1:]
		
		dataLen = len(self.buffer)
		if dataLen > 3:
			packetLength = self.buffer[1]
			if packetLength > dataLen:
				return

			packetType = self.buffer[2]
			
			if packetType in self.packetLengths:
				processing = self.packetLengths[packetType]

				if 'length' in processing:
					if packetLength != processing['length']:
						print('wrong packet length')

				if 'callback' in processing:
					if callable(processing['callback']):
						processing['callback'](self.buffer[3:packetLength])

			else:
				print('unknown packet type', hex(packetType))


			status = True

			self.buffer = self.buffer[packetLength:]

		return status

	def processing(self):
		with uinput.Device(self.keyMapping) as device:
			self.device = device
			self.buffer = self.ser.read(1)

			while True:
				try:
					bufferBytes =  self.ser.inWaiting()
					if bufferBytes or len(self.buffer):
						self.buffer = self.buffer + self.ser.read(bufferBytes)

						status = True
						while status:
							# print('processing')
							status = self.parseBuffer()

					else:
						time.sleep(0.001)

				except KeyboardInterrupt: 
					print('Exit: KeyboardInterrupt')
					raise

# ser.close() # close port

control = RemoteControl()
control.processing()