server_addr = '/tmp/overlay.sock'

class Form:
    def __init__(self, id, x, y, img):
        self.id = id
        self.x = x
        self.y = y
        self.img = img
        self.fields = []

class Field:
    def __init__(self, x, y, img):
        self.x = x
        self.y = y
        self.img = img
