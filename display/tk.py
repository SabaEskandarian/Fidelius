server_addr = '/tmp/overlay.sock'

class Form:
    def __init__(self, id, x, y, img, img2):
        self.id = id
        self.x = x
        self.y = y
        self.img = img
	self.img2 = img2
        self.fields = []
	self.inputBuff = ""

class Field:
    def __init__(self, x, y, img):
        self.x = x
        self.y = y
        self.img = img
