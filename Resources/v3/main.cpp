// COMPILE COMMANDS:
/*

g++ main.cpp -o output -I/Library/Frameworks/SDL2.framework/Headers -I/Library/Frameworks/SDL2_image.framework/Headers -I/Library/Frameworks/SDL2_ttf.framework/Headers -F/Library/Frameworks -framework SDL2 -framework SDL2_Image -framework SDL2_ttf

g++ main.cpp -o output -lSDL2 -lSDL2_image -lSDL2_ttf

*/

#include <iostream>
#include <cmath>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <algorithm>
#include <iterator>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#define SCALE 1
#define OUTPUT_SIZE 16

int open_serial();
int close_serial(int serial_port);
int read_serial(int serial_port, char* output);
void clear_array(char* array, int size);
void print_array(char* array, int size);


int main(int argc, char* argv[]){

	SDL_Window* window = nullptr;
	
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not be initialized: " << SDL_GetError() << std::endl;
	} else {
		std::cout << "SDL video system is ready to go\n" << std::endl;
	}

	window = SDL_CreateWindow("Instrument Cluster", 0, 0, 1920, 720, SDL_WINDOW_SHOWN); // | SDL_WINDOW_FULLSCREEN);

	SDL_Renderer* renderer = nullptr;
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	int flags = IMG_INIT_PNG;
	int initStatus = IMG_Init(flags);
	if ((initStatus & flags) != flags) {
		std::cout << "SDL2_Image format not available" << std::endl;
	}
	
	SDL_Surface* baseImage = IMG_Load("./images/Base.png");
	SDL_Surface* needleImage = IMG_Load("./images/Needle.png");
	SDL_Surface* needleCoverImage = IMG_Load("./images/Needle_cover.png");
	SDL_Surface* overlayImage = IMG_Load("./images/Overlay.png");
	
	SDL_Surface* gavinImages[8];
	gavinImages[0] = IMG_Load("./images/Gavin/000.png");
	gavinImages[1] = IMG_Load("./images/Gavin/001.png");
	gavinImages[2] = IMG_Load("./images/Gavin/010.png");
	gavinImages[3] = IMG_Load("./images/Gavin/011.png");
	gavinImages[4] = IMG_Load("./images/Gavin/100.png");
	gavinImages[5] = IMG_Load("./images/Gavin/101.png");
	gavinImages[6] = IMG_Load("./images/Gavin/110.png");
	gavinImages[7] = IMG_Load("./images/Gavin/111.png");
	
	
	if (!baseImage || !needleImage || !needleCoverImage || !overlayImage){
		std::cout << "Image loading error..." << std::endl;
	}
	
	for (int i = 0; i < 8; i++) {
		if (!gavinImages[i]) {
			std::cout << "Image loading error..." << std::endl;
		}
	}
	

	SDL_Texture* baseTexture = SDL_CreateTextureFromSurface(renderer, baseImage);
	SDL_Texture* needleTexture = SDL_CreateTextureFromSurface(renderer, needleImage);
	SDL_Texture* needleCoverTexture = SDL_CreateTextureFromSurface(renderer, needleCoverImage);
	SDL_Texture* overlayTexture = SDL_CreateTextureFromSurface(renderer, overlayImage);

	SDL_Texture* gavinTextures[8];
	for (int i = 0; i < 8; i++) {
		
		gavinTextures[i] = SDL_CreateTextureFromSurface(renderer, gavinImages[i]);
		
	}
	
	//SDL_DisplayMode DM;
	//SDL_GetCurrentDisplayMode(0, &DM);
	//int Width = DM.w;
	//int Height = DM.h;

	int width;
	int height;
	SDL_GetWindowSize(window, &width, &height);
	//SDL_GetRendererOutputSize(renderer, &width, &height);
	
	//std::cout << "Width: " << width << std::endl;
	//std::cout << "Height: " << height << std::endl;
	
	int i = 0;
	int mouseX;
	int mouseY;
	
	double speed;
	double rpm;
	
	bool STATE_leftIndicator;
	bool STATE_rightIndicator;
	bool STATE_headlights;
	int STATE_gavinTotal;
	
	
	// Fonts
	TTF_Init();
	
	TTF_Font *speedFont = TTF_OpenFont("/fonts/conthrax-sb.ttf", 100);
	TTF_Font *rpmFont = TTF_OpenFont("/fonts/conthrax-sb.ttf", 75);
	TTF_Font *debugFont = TTF_OpenFont("/fonts/conthrax-sb.ttf", 25);
	
	if (!speedFont | !rpmFont)
        std::cout << "Couldn't find/init open a ttf font." << std::endl;
	
	SDL_Color SDL_fontColour = {255, 255, 255};
	
	SDL_Surface* speedSurface;
	SDL_Surface* rpmSurface;
	
	SDL_Texture* speedTexture;
	SDL_Texture* rpmTexture;
	
	char speedChar[10];
	char rpmChar[10];
	
	
	// Infinite loop for our application
	bool gameIsRunning = true;
	// Main application loop
	
	// Open serial port
	int port = open_serial();
	char serial_output[OUTPUT_SIZE];
	int out_length;
	
	while (gameIsRunning){
		
		//std::cout << "\nFrame: " << ++i << std::endl;
		
		// Read latest serial
		out_length = read_serial(port, serial_output);
		
		if (out_length != -1) {
			
			sscanf(serial_output, "%lf|%lf", &speed, &rpm);
			//std::cout << "Speed = " << speed << "\n  RPM = " << rpm << "\n\n\n" << std::endl;
		}
			
		//std::cout << "X: " << mouseX << "\nY: " << mouseY << std::endl;
		
		SDL_Event event;

		// (1) Handle Input
		// Start our event loop
		while (SDL_PollEvent(&event)){
			// Handle each specific event
			if (event.type == SDL_QUIT){
				gameIsRunning = false;
			}
			
			
			const Uint8* state = SDL_GetKeyboardState(NULL);
			STATE_leftIndicator = 0;
			STATE_rightIndicator = 0;
			STATE_headlights = 0;
			
			if (state[SDL_SCANCODE_LEFT]){
				STATE_leftIndicator = 1;
			}
			
			if (state[SDL_SCANCODE_RIGHT]){
				STATE_rightIndicator = 1;
			}
			
			if (state[SDL_SCANCODE_UP]){
				STATE_headlights = 1;
			}
			
			if (state[SDL_SCANCODE_SPACE]){
				STATE_leftIndicator = 1;
				STATE_rightIndicator = 1;
			}
			
			STATE_gavinTotal = 1 * STATE_headlights + \
							   2 * STATE_rightIndicator + \
							   4 * STATE_leftIndicator;

		}
		// (2) Handle Updates
		
		i++;
		
		//speed = 240 * (sin((double)i / 50) + 1) / 2;
		//rpm = 240 * (sin((double)(i+80) / 50) + 1) / 2;
		
		// (3) Clear and Draw the Screen
		// Gives us a clear "canvas"
		SDL_SetRenderDrawColor(renderer, 0, 0, 0xFF, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		
		// Render Base
		SDL_RenderCopy(renderer, baseTexture, NULL, NULL);
		
		// Render Speed Needle
		SDL_Rect dstRectSpeed;
		dstRectSpeed.x = SCALE * 380;
		dstRectSpeed.y = SCALE * 10;
		dstRectSpeed.w = SCALE * 360;
		dstRectSpeed.h = SCALE * 720;
		
		SDL_Point needleCenterSpeed;
		needleCenterSpeed.x = SCALE * 0;
		needleCenterSpeed.y = SCALE * 360;
		
		SDL_RendererFlip noFlip = SDL_FLIP_NONE;
		
		// Calculate speed
		//speed = (double)(mouseX)/5;
		
		// Render front of needle
		SDL_RenderCopyEx(renderer, needleTexture, NULL, &dstRectSpeed, 1 * speed, &needleCenterSpeed, noFlip);

		if (speed <= 180) {
			// Cover the exposed needle
			SDL_RenderCopy(renderer, needleCoverTexture, NULL, &dstRectSpeed);
		} else {
			// Render back of needle
			SDL_RenderCopyEx(renderer, needleTexture, NULL, &dstRectSpeed, 180, &needleCenterSpeed, noFlip);
		}
		
		
		
		
		// Render RPM Needle
		SDL_Rect dstRectRPM;
		dstRectRPM.x = SCALE * 1180;
		dstRectRPM.y = SCALE * 10;
		dstRectRPM.w = SCALE * 360;
		dstRectRPM.h = SCALE * 720;
		
		SDL_Point needleCenterRPM;
		needleCenterRPM.x = SCALE * 360;
		needleCenterRPM.y = SCALE * 360;
		
		SDL_RendererFlip horizontalFlip = SDL_FLIP_HORIZONTAL;
		
		// Calculate speed
		//rpm = (double)(mouseY)/3;
		
		// Render front of needle
		SDL_RenderCopyEx(renderer, needleTexture, NULL, &dstRectRPM, -0.04 * rpm, &needleCenterRPM, horizontalFlip);

		
		if (rpm <= 4500) {
			// Cover the exposed needle
			SDL_RenderCopyEx(renderer, needleCoverTexture, NULL, &dstRectRPM, 0, NULL, horizontalFlip);
		} else {
			// Render back of needle
			SDL_RenderCopyEx(renderer, needleTexture, NULL, &dstRectRPM, 180, &needleCenterRPM, horizontalFlip);
		}
		
		
		// Render Gavin
		SDL_Rect dstRectGavin;
		dstRectGavin.x = SCALE * 698.6105;
		dstRectGavin.y = SCALE * 175;
		dstRectGavin.w = SCALE * 522.779;
		dstRectGavin.h = SCALE * 459;
		
		SDL_RenderCopy(renderer, gavinTextures[STATE_gavinTotal], NULL, &dstRectGavin);
		
		
		// Render Overlay
		SDL_RenderCopy(renderer, overlayTexture, NULL, NULL);
		
		
		// Render Speed
		snprintf(speedChar, 10, "%.0f", speed);
		
		speedSurface = TTF_RenderText_Blended(speedFont, speedChar, SDL_fontColour);
		speedTexture = SDL_CreateTextureFromSurface(renderer, speedSurface);
		
		SDL_Rect dstSpeedText;
		dstSpeedText.x = SCALE * 380.0 - (double)(speedSurface->w)/2;
		dstSpeedText.y = SCALE * 365.3 - (double)(speedSurface->h)/2;
		dstSpeedText.w = SCALE * speedSurface->w;
		dstSpeedText.h = SCALE * speedSurface->h;
		SDL_RenderCopy(renderer, speedTexture, NULL, &dstSpeedText);
		
		
		// Render RPM
		snprintf(rpmChar, 10, "%.0f", rpm);
		
		rpmSurface = TTF_RenderText_Blended(rpmFont, rpmChar, SDL_fontColour);
		rpmTexture = SDL_CreateTextureFromSurface(renderer, rpmSurface);
		
		SDL_Rect dstRPMText;
		dstRPMText.x = SCALE * 1540.0 - (double)(rpmSurface->w)/2;
		dstRPMText.y = SCALE * 365.3 - (double)(rpmSurface->h)/2;
		dstRPMText.w = SCALE * rpmSurface->w;
		dstRPMText.h = SCALE * rpmSurface->h;
		SDL_RenderCopy(renderer, rpmTexture, NULL, &dstRPMText);
		
		
		// Finally show what we've drawn
		SDL_RenderPresent(renderer);
	}

	// We destroy our window. We are passing in the pointer
	// that points to the memory allocated by the 
	// 'SDL_CreateWindow' function. Remember, this is
	// a 'C-style' API, we don't have destructors.
	SDL_DestroyWindow(window);
	
	// Free our png image surfaces
	SDL_FreeSurface(baseImage);
	SDL_FreeSurface(needleImage);
	SDL_FreeSurface(needleCoverImage);
	
	// And destroy our textures
	SDL_DestroyTexture(baseTexture);
	SDL_DestroyTexture(needleTexture);
	SDL_DestroyTexture(needleCoverTexture);

	IMG_Quit();

	// Quit our program.
	SDL_Quit();
	return 0;
}

int open_serial() {

	// Form termios structure
	struct termios tty;
	
	// Open the serial port
	// ls /dev | grep -e "cu.usbmodem*"
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
	
	while (size != -1) {
		
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


