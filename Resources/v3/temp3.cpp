#include <iostream> // Write to terminal
#include <algorithm>
#include <iterator>

#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#define OUTPUT_SIZE 10

int open_serial();
int close_serial(int serial_port);
int read_serial(int serial_port, char* output);
void clear_array(char* array, int size);
void print_array(char* array, int size);


int main() {
	
	// Open serial port
	int port = open_serial();
	
	int counter = 0;
	
	char char_array[OUTPUT_SIZE];
	
	// Loop
	while (true) {

		// Delay
		usleep(200 * 1000);
		
		// Initialise return char array
		
		
		std::cout << "\nReading Serial " << counter << "..." << std::endl;
		
		// Read latest serial
		int out_length = read_serial(port, char_array);
		
		print_array(char_array, out_length);
		
		std::cout << "\tSerial length = " << out_length << "\n\n\n" << std::endl;
		
		counter++;
	}
	
	close_serial(port);

}

int open_serial() {

	// Form termios structure
	struct termios tty;
	
	// Open the serial port
	int serial_port = open("/dev/cu.usbmodem101", O_RDONLY | O_NONBLOCK);

	// Check for errors
	if(tcgetattr(serial_port, &tty) != 0) {
		
		std::cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
		return -1;
	}


	// Set flags
	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
	
	tty.c_lflag |= ICANON; // Canonical mode
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
	
	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	
	tty.c_cc[VTIME] = 0; // Wait time
	tty.c_cc[VMIN] = 0; // Minimum received bytes
	
	cfsetispeed(&tty, B115200); // Baud input
	cfsetospeed(&tty, B115200); // Baud output

	// Set flags and report any errors
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
		
		std::cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
		return -1;
	}
	
	// Flush what is currently in the buffer
	tcflush(serial_port, TCIFLUSH);
	
	// Return port
	return serial_port;
	
}

int close_serial(int serial_port) {
	
	// Close the serial port
	close(serial_port);
	return 0;
	
}

int read_serial(int serial_port, char* output) {
	
	int size = 0;
	int saved_size = 1;
	
	while(size != -1) {
		
		size = read(serial_port, output, OUTPUT_SIZE);
		
		// If read() found data, overwrite previous size information
		if (size != -1) {saved_size = size;}
		
	}
	
	return saved_size - 2;
	
}


void clear_array(char* array, int size) {
	
	// Clear the serial storage
	for (int i = 0; i < size; i++) {
		array[i] = '\0';
	}
	
}


void print_array(char* array, int size) {
	
	if (size == 0) {return;}
	
	// Print the array
	for (int i = 0; i < size; i++) {
		
		if (isdigit(array[i])) {
			std::cout << array[i];
		}
	}
	
	std::cout << std::endl;
	
}