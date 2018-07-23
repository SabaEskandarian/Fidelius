import argparse
import hashlib
from ecdsa import SigningKey, NIST256p
import binascii

parser = argparse.ArgumentParser()
parser.add_argument('message', help="message to sign")
parser.add_argument('--make_keys', default=False, help="set to true to generate new private/public keys")
parser.add_argument('--secret_key', default='7df4b0d136bfb597e179b2eec87a7be253b1be2ca82f342e7a3fd196a7e58be5', help="key to sign with")
args=parser.parse_args()

message = args.message

if(args.make_keys):
	sk = SigningKey.generate(curve=NIST256p)
	vk = sk.get_verifying_key()
else:
	sk = args.secret_key
	sk = [sk[idx:idx+2] for idx,val in enumerate(sk) if idx%2 == 0]
	sk = "".join([binascii.unhexlify(s) for s in sk])
	sk = SigningKey.from_string(sk, curve=NIST256p)

signiture = sk.sign(message)
print("Signiture: " + str([ord(i) for i in signiture]))
if(args.make_keys):
	print("Private key: " + str([ord(i) for i in sk.to_string()]))
	print("Public key: " + str([ord(i) for i in vk.to_string()]))