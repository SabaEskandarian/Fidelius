import os
import binascii
import sys

from cryptography.hazmat.primitives.ciphers.aead import AESGCM

def encrypt(iv, plaintext):
	"""
	Inputs: nonce (a string containing hex), plaintext (string)
	Output: ciphertext with 16 bytes tag appended and 12 bytes iv appended(hex representation)
	"""

	secret_key = "24a3e5ad48a7a6b198fe35fbe16c6685"
	aesgcm = AESGCM(binascii.unhexlify(secret_key))
	ciphertext = aesgcm.encrypt(binascii.unhexlify(iv), plaintext ,None) #assuming no AAD
	return (binascii.hexlify(ciphertext) + iv)

def decrypt(iv, ciphertext):
 	"""
 	Inputs: nonce (a string containing hex), ciphertext with 16 bytes tag appended (hex representation
 	Output: plaintext (string)
 	"""
	secret_key = "24a3e5ad48a7a6b198fe35fbe16c6685"
	aesgcm = AESGCM(binascii.unhexlify(secret_key))
	plaintext = aesgcm.decrypt(binascii.unhexlify(iv),binascii.unhexlify(ciphertext[:(len(ciphertext) - 24)]),None)
	return plaintext



if __name__ == "__main__":
	if len(sys.argv) < 3:
		#print "python AES.py <iv> <plaintext>"
		sample = ("000000000000", "a")
		#print "running with iv = %s, plaintext= %s" % sample
		print encrypt(sample[0], sample[1])
	else:
		print encrypt(sys.argv[1], sys.argv[2])

	#Example on how to use
	"""
	nonce = os.urandom(12)
	hex_nonce = binascii.hexlify(nonce)
	c  = encrypt(hex_nonce, "LOL")
	message = decrypt(hex_nonce, c)
	print message
	"""



""" SOME SAMPLE CODE
nonce = os.urandom(12) #96 bit IV, now chosen randomly
	#alternatively, use pickle 
	#counter_file = 'counter.pk'
	#with open(counter_file, 'wb') as fi:
	#	pickle.dump(counter, fi)
	#with open(counter_file, 'rb') as fi:
	#	counter = counter.load(fi)
hex_nonce = binascii.hexlify(nonce)

print hex_nonce


data = "haha"
key = AESGCM.generate_key(bit_length=128)
hex_key = binascii.hexlify(key)
print hex_key


aesgcm = AESGCM(key)
ciphertext = aesgcm.encrypt(nonce,data,None)
hex_cipher = binascii.hexlify(ciphertext)
print hex_cipher

#print(aesgcm.decrypt(nonce,ciphertext,extra))

iv = binascii.unhexlify(hex_nonce)

cipher = binascii.unhexlify(hex_cipher)
print(aesgcm.decrypt(iv,cipher,None))

#HEX_STRING = "727d1a31b604eda2a407cf53e3489ced9a82c0e1a098524d2006f1f11b1676b1"
#encoded = HEX_STRING.decode("hex").encode("base64")
#print encoded
"""



"""
iv = "000000000000000000000000"
key = "24a3e5ad48a7a6b198fe35fbe16c6685"
ciphertext = "e91369b4ce90d962882854d2f1f8e90c06"
aesgcm = AESGCM(binascii.unhexlify(key))

c = aesgcm.encrypt(binascii.unhexlify(iv), "What is happening?" ,None)
print (binascii.hexlify(c))
"""
