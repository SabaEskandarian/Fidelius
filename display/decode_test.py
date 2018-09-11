import io
import sys
import logging
import signal
import binascii
from struct import *
from PIL import Image

def bits2byte(bits8):
	result = 0;
	for bit in bits8:
		result <<=1
		result |= bit
	return result

def byteToBits(byte):
	return [1 if digit=='1' else 0 for digit in bin(byte)[2:]]

def runTest():
	encodedString = "\xff"*360
	encodedbytes = bytearray(encodedString)
	bitlists = [byteToBits(byte) for byte in encodedbytes]
	bits = [a for b in bitlists for a in b]
			
	octets = [bits[i:i+8] for i in range(0, len(bits), 8)]
	data = [bits2byte(octet) for octet in octets]

	Image.frombytes('1', (width, height), data).show()

if __name__ == "__main__":
	runTest()
	
