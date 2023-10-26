
// ——————— DEVICE CODE ——————— //
#define DEVICE_CODE_GAVIN 0
#define DEVICE_CODE_DEV 1

#ifdef __APPLE__
int device_code = DEVICE_CODE_DEV;
#else
int device_code = DEVICE_CODE_GAVIN;
#endif
// ——————— DEVICE CODE ——————— //

#include "external/include/serial/serial.h"
#include <thread>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

using namespace std;

#define SCALE 1
#define OUTPUT_SIZE 16

#define SPEED_VECTOR_SIZE 25
#define RPM_VECTOR_SIZE 5

#define RPM_SIG_FIGS 2

void clear_array(char* array, int size);
void print_array(char* array, int size);
bool is_number(const std::string& s);

int main(int argc, char* argv[]){

	SDL_Window* window = nullptr;
	
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not be initialized: " << SDL_GetError() << std::endl;
	} else {
		std::cout << "SDL video system is ready to go\n" << std::endl;
	}

	switch (device_code) {
		case DEVICE_CODE_GAVIN:
			window = SDL_CreateWindow("Instrument Cluster", 0, 0, 1920, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
			SDL_ShowCursor(SDL_DISABLE);
			break;

		case DEVICE_CODE_DEV:
			window = SDL_CreateWindow("Instrument Cluster", 0, 0, 1920, 720, SDL_WINDOW_SHOWN);
			break;


	}

	SDL_Renderer* renderer = nullptr;
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	int flags = IMG_INIT_PNG;
	int initStatus = IMG_Init(flags);
	if ((initStatus & flags) != flags) {
		std::cout << "SDL2_Image format not available" << std::endl;
	}
	
	SDL_Surface* baseImage = IMG_Load("images/Base.png");
	SDL_Surface* needleImage = IMG_Load("images/Needle.png");
	SDL_Surface* needleCoverImage = IMG_Load("images/Needle_cover.png");
	SDL_Surface* overlayImage = IMG_Load("images/Overlay.png");
	
	SDL_Surface* gavinImages[8];
	gavinImages[0] = IMG_Load("images/Gavin/000.png");
	gavinImages[1] = IMG_Load("images/Gavin/001.png");
	gavinImages[2] = IMG_Load("images/Gavin/010.png");
	gavinImages[3] = IMG_Load("images/Gavin/011.png");
	gavinImages[4] = IMG_Load("images/Gavin/100.png");
	gavinImages[5] = IMG_Load("images/Gavin/101.png");
	gavinImages[6] = IMG_Load("images/Gavin/110.png");
	gavinImages[7] = IMG_Load("images/Gavin/111.png");
	
	
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
	
	TTF_Font *speedFont = TTF_OpenFont("fonts/conthrax-sb.ttf", 100);
	TTF_Font *rpmFont = TTF_OpenFont("fonts/conthrax-sb.ttf", 75);
	
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
	bool clusterRunning = true;
	// Main application loop
	
	
	int saved_speed = 0;
	int saved_rpm = 0;
	
	SDL_Event event;
	SDL_Rect dstRectSpeed;
	SDL_RendererFlip noFlip = SDL_FLIP_NONE;
	SDL_Point needleCenterSpeed;
	SDL_Rect dstRectRPM;
	SDL_Point needleCenterRPM;
	SDL_RendererFlip horizontalFlip = SDL_FLIP_HORIZONTAL;
	SDL_Rect dstRectGavin;
	SDL_Rect dstSpeedText;
	SDL_Rect dstRPMText;
	
	const Uint8* state;
	
	int frame = 0;

	vector<int> speedVector;
	vector<int> rpmVector;

	// Setting up serial
	serial::Serial connection("/dev/tty.usbmodem1101", 115200, serial::Timeout::simpleTimeout(3000));

    if (connection.isOpen()) {
        cout << "Port opened successfully" << endl;
    } else {
        cout << "Port failed to open" << endl;
        exit(1);
    }
    connection.flushOutput();

	string serial_response;

	double input_speed = 0;
	int input_rpm = 0;
	
	
	while (clusterRunning) {
		
		//std::cout << "\nFrame: " << ++i << std::endl;
		
		// Which device are we using
		switch (device_code) {
			case DEVICE_CODE_DEV: // DEVICE_CODE_GAVIN

				connection.flushInput();
				serial_response = connection.read(9);

				if (is_number(serial_response)) {
					input_rpm = stoi(serial_response.substr(5, -1));
					input_speed = stoi(serial_response.substr(0, 5));
					input_speed /= 100.0;
					// cout << "SPEED: [" << input_speed << "]" << endl;
					// cout << "RPM:   [" << input_rpm << "]\n" << endl;
				}

				break;
				
			case DEVICE_CODE_GAVIN: // DEVICE_CODE_DEV:

				SDL_GetMouseState(&mouseX, &mouseY);
				input_speed = ((double)(mouseX) / 7.5) + rand() % 3;
				input_rpm = ((double)(mouseY) * 8) + rand() % 20;

				break;
		}


		// Collect most recent speed and rpm data in vectors
		if (speedVector.size() >= SPEED_VECTOR_SIZE) {
			speedVector.erase(speedVector.begin());
		}

		if (rpmVector.size() >= RPM_VECTOR_SIZE) {
			rpmVector.erase(rpmVector.begin());
		}

		speedVector.push_back(input_speed);
		rpmVector.push_back(input_rpm);

		// Calculate average speed and rpm
		speed = 0;
		for (int i = 0; i < speedVector.size(); i++) {
			speed += speedVector[i];
		}
		speed /= speedVector.size();

		rpm = 0;
		for (int i = 0; i < rpmVector.size(); i++) {
			rpm += rpmVector[i];
		}
		rpm /= rpmVector.size();

		
		// (1) Handle Input
		// Start our event loop
		while (SDL_PollEvent(&event)){
			// Handle each specific event
			if (event.type == SDL_QUIT){
				clusterRunning = false;
			}
			
			
			state = SDL_GetKeyboardState(NULL);
			STATE_leftIndicator = 0;
			STATE_rightIndicator = 0;
			STATE_headlights = 0;
			
			if (state[SDL_SCANCODE_LEFT]) {
				STATE_leftIndicator = 1;
			}
			
			if (state[SDL_SCANCODE_RIGHT]) {
				STATE_rightIndicator = 1;
			}
			
			if (state[SDL_SCANCODE_UP]) {
				STATE_headlights = 1;
			}
			
			if (state[SDL_SCANCODE_SPACE]) {
				STATE_leftIndicator = 1;
				STATE_rightIndicator = 1;
			}

			if (state[SDL_SCANCODE_ESCAPE]) {
				return 0;
			}
			
			STATE_gavinTotal = 1 * STATE_headlights + \
							   2 * STATE_rightIndicator + \
							   4 * STATE_leftIndicator;

		}
		// (2) Handle Updates
		
		i++;

		// Debugging for freezing, remove once done
		STATE_headlights = (i % 20) > 10;

		STATE_gavinTotal = 1 * STATE_headlights + \
							   2 * STATE_rightIndicator + \
							   4 * STATE_leftIndicator;
		
		//speed = 240 * (sin((double)i / 50) + 1) / 2;
		//rpm = 240 * (sin((double)(i+80) / 50) + 1) / 2;
		
		// (3) Clear and Draw the Screen
		// Gives us a clear "canvas"
		SDL_SetRenderDrawColor(renderer, 0, 0, 0xFF, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		
		// Render Base
		SDL_RenderCopy(renderer, baseTexture, NULL, NULL);
		
		// Render Speed Needle
		dstRectSpeed.x = SCALE * 380;
		dstRectSpeed.y = SCALE * 10;
		dstRectSpeed.w = SCALE * 360;
		dstRectSpeed.h = SCALE * 720;
		
		needleCenterSpeed.x = SCALE * 0;
		needleCenterSpeed.y = SCALE * 360;
		
		
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

		dstRectRPM.x = SCALE * 1180;
		dstRectRPM.y = SCALE * 10;
		dstRectRPM.w = SCALE * 360;
		dstRectRPM.h = SCALE * 720;
		

		needleCenterRPM.x = SCALE * 360;
		needleCenterRPM.y = SCALE * 360;
		
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
		dstRectGavin.x = SCALE * 698.6105;
		dstRectGavin.y = SCALE * 175;
		dstRectGavin.w = SCALE * 522.779;
		dstRectGavin.h = SCALE * 459;
		
		SDL_RenderCopy(renderer, gavinTextures[STATE_gavinTotal], NULL, &dstRectGavin);
		
		
		// Render Overlay
		SDL_RenderCopy(renderer, overlayTexture, NULL, NULL);
		
		
		// Render Speed
		snprintf(speedChar, 10, "%.0f", speed);
		

		SDL_Surface* speedSurface = TTF_RenderText_Blended(speedFont, speedChar, SDL_fontColour);
		SDL_Texture* speedTexture = SDL_CreateTextureFromSurface(renderer, speedSurface);


		dstSpeedText.x = SCALE * 380.0 - (double)(speedSurface->w)/2;
		dstSpeedText.y = SCALE * 365.3 - (double)(speedSurface->h)/2;
		dstSpeedText.w = SCALE * speedSurface->w;
		dstSpeedText.h = SCALE * speedSurface->h;
		SDL_RenderCopy(renderer, speedTexture, NULL, &dstSpeedText);
		
		
		// Render RPM
		snprintf(rpmChar, 10, "%d", (((int)rpm / 10) * 10));
		

		rpmSurface = TTF_RenderText_Blended(rpmFont, rpmChar, SDL_fontColour);
		rpmTexture = SDL_CreateTextureFromSurface(renderer, rpmSurface);
	
		//frame++;

		dstRPMText.x = SCALE * 1540.0 - (double)(rpmSurface->w)/2;
		dstRPMText.y = SCALE * 365.3 - (double)(rpmSurface->h)/2;
		dstRPMText.w = SCALE * rpmSurface->w;
		dstRPMText.h = SCALE * rpmSurface->h;
		SDL_RenderCopy(renderer, rpmTexture, NULL, &dstRPMText);
		
		
		
		// Finally show what we've drawn
		SDL_RenderPresent(renderer);
		
		SDL_FreeSurface(speedSurface);
		SDL_DestroyTexture(speedTexture);
		
		SDL_FreeSurface(rpmSurface);
		SDL_DestroyTexture(rpmTexture);
		
		this_thread::sleep_for(chrono::milliseconds(16));
		
		
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


bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
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