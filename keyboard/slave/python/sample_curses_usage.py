import curses
from time import sleep

# def mainApp(stdscr):            
#     stdscr.clear()
#     stdscr.addstr("hello")

# curses.wrapper(mainApp)


def main(stdscr):
    # Clear screen
    stdscr.clear()

    # This raises ZeroDivisionError when i == 10.
    for i in range(0, 11):
        v = i-10
        stdscr.addstr(i, 0, '10 divided by {} is {}'.format(v, 10/v))
        stdscr.refresh()
        sleep(1)

    stdscr.getkey()

#main(curses.initscr())
curses.wrapper(main)
