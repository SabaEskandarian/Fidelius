import curses

def printToScreen(stdscr, keyName):
    stdscr.addstr(keyName)
    stdscr.refresh()

def sendKey(keyName):
    pass

def main(stdscr):
    # Clear screen
    stdscr.clear()

    while True:
        #key = stdscr.getkey()
        #stdscr.addstr(i, 0, '10 divided by {} is {}'.format(v, 10/v))
        
        keyId = stdscr.getch()
        keyName = curses.keyname(keyId)
        printToScreen(stdscr, keyName)
        sendKey(keyName)

#main(curses.initscr())
curses.wrapper(main)
