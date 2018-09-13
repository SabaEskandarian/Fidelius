all:
	g++ -Wall -Werror -c keyboard_driver.cpp -I. 

test:
	g++ -Wall -Werror keyboard_driver_t.cpp -I. keyboard_driver.o -o test

clean:
	rm -f *.o test
