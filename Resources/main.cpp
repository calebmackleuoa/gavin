

// ——————— DEVICE CODE ——————— //
#define DEVICE_CODE_GAVIN 0
#define DEVICE_CODE_DEV 1
/*
#ifdef __APPLE__
int device_code = DEVICE_CODE_DEV;
#else
int device_code = DEVICE_CODE_GAVIN;
#endif
*/
// ——————— DEVICE CODE ——————— //

#include "external/include/serial/serial.h"
#include <thread>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <vector>
#include <chrono>
#include <stdio.h>
#include <filesystem>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

using namespace std;

#define SCALE 1
#define OUTPUT_SIZE 16

#define SPEED_VECTOR_SIZE 25
#define RPM_VECTOR_SIZE 5

#define RPM_SIG_FIGS 2

#define MILES_TO_KM 1.609344

#define TEXT_DEBUG true


void clear_array(char* array, int size);
void print_array(char* array, int size);
bool is_number(const std::string& s);
void debug(string message);

int main(int argc, char* argv[]){

	auto pathname = "/dev/ttyACM0";

	int device_code = DEVICE_CODE_DEV;
	if (std::filesystem::exists(pathname)) {
		device_code = DEVICE_CODE_GAVIN;
	}

	SDL_Window* window = nullptr;
	
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not be initialized: " << SDL_GetError() << std::endl;
		exit(1);
	} else {
		std::cout << "SDL video system is ready to go\n" << std::endl;
	}

	switch (device_code) {
		case DEVICE_CODE_GAVIN:
			window = SDL_CreateWindow("Instrument Cluster", 0, 0, 1920, 720, \
			SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
			SDL_ShowCursor(SDL_DISABLE);
			break;

		case DEVICE_CODE_DEV:
			window = SDL_CreateWindow("Instrument Cluster", 0, 0, 1920, 720, SDL_WINDOW_SHOWN);
			break;


	}

	if (window == nullptr) {
		std::cout << "SDL window creation failed: " << SDL_GetError() << std::endl;
		exit(1);
	} else {
		std::cout << "SDL window created successfully\n" << std::endl;
	}



	SDL_Renderer* renderer = nullptr;
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (renderer == nullptr) {
		std::cout << "SDL renderer creation failed: " << SDL_GetError() << std::endl;
		exit(1);
	} else {
		std::cout << "SDL renderer created successfully\n" << std::endl;
	}


	int flags = IMG_INIT_PNG;
	int initStatus = IMG_Init(flags);
	if ((initStatus & flags) != flags) {
		std::cout << "SDL2_Image format not available" << std::endl;
	}
	
	SDL_Surface* baseImage = IMG_Load("images/Base.png");
	SDL_Surface* needleImage = IMG_Load("images/Needle.png");
	SDL_Surface* needleCoverImage = IMG_Load("images/Needle_cover.png");
	SDL_Surface* overlayImage = IMG_Load("images/Overlay.png");
	SDL_Surface* shadeImage = IMG_Load("images/Gavin/shade.png");
	
	SDL_Surface* gavinImages[8];
	gavinImages[0] = IMG_Load("images/Gavin/000.png");
	gavinImages[1] = IMG_Load("images/Gavin/001.png");
	gavinImages[2] = IMG_Load("images/Gavin/010.png");
	gavinImages[3] = IMG_Load("images/Gavin/011.png");
	gavinImages[4] = IMG_Load("images/Gavin/100.png");
	gavinImages[5] = IMG_Load("images/Gavin/101.png");
	gavinImages[6] = IMG_Load("images/Gavin/110.png");
	gavinImages[7] = IMG_Load("images/Gavin/111.png");
	
	
	if (!baseImage || !needleImage || !needleCoverImage || !overlayImage || !shadeImage){
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
	SDL_Texture* shadeTexture = SDL_CreateTextureFromSurface(renderer, shadeImage);

	SDL_Texture* gavinTextures[8];
	for (int i = 0; i < 8; i++) {
		
		gavinTextures[i] = SDL_CreateTextureFromSurface(renderer, gavinImages[i]);
		
	}


	int width;
	int height;
	SDL_GetWindowSize(window, &width, &height);

	
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
	TTF_Font *odoFont = TTF_OpenFont("fonts/conthrax-sb.ttf", 25);
	
	if (!speedFont | !rpmFont)
        std::cout << "Couldn't find/init open a ttf font." << std::endl;
	
	SDL_Color SDL_fontColour_white = {255, 255, 255};
	SDL_Color SDL_fontColour_grey = {255, 255, 255, 100};
	
	SDL_Surface* speedSurface;
	SDL_Surface* rpmSurface;
	SDL_Surface* odoSurface;
	SDL_Surface* tripSurface;
	
	SDL_Texture* speedTexture;
	SDL_Texture* rpmTexture;
	SDL_Texture* odoTexture;
	SDL_Texture* tripTexture;

	
	char speedChar[10];
	char rpmChar[10];
	
	
	// Infinite loop
	bool clusterRunning = true;
	// Main application loop
	
	
	int saved_speed = 0;
	int saved_rpm = 0;
	
	SDL_Event event;

	SDL_RendererFlip noFlip = SDL_FLIP_NONE;
	SDL_RendererFlip horizontalFlip = SDL_FLIP_HORIZONTAL;

	SDL_Point needleCenterSpeed;
	SDL_Point needleCenterRPM;

	SDL_Rect dstRectSpeed;
	SDL_Rect dstRectRPM;
	SDL_Rect dstRectGavin;
	SDL_Rect dstSpeedText;
	SDL_Rect dstRPMText;
	SDL_Rect dstOdoText;
	SDL_Rect dstTripText;
	
	const Uint8* state;
	
	int frame = 0;

	vector<int> speedVector;
	vector<int> rpmVector;

	// Setting up serial connection
	string connection_location;

	if (device_code == DEVICE_CODE_DEV) {
		connection_location = "";
	} else {
		connection_location = "/dev/ttyACM0";
	}

	serial::Serial connection(connection_location, 115200, serial::Timeout::simpleTimeout(3000));

    if (connection.isOpen()) {
        std::cout << "Port opened successfully" << endl;
		connection.flushOutput();
    } else {
        std::cout << "Port did not open" << endl;
    }

	string serial_response;

	double input_speed = 0;
	int input_rpm = 0;

	auto odo_time_old = std::chrono::high_resolution_clock::now();
	auto odo_time_new = std::chrono::high_resolution_clock::now();
	double odo_period;

	double odometer = 160000 * 1000 * MILES_TO_KM;
	char odometer_char_array[30];
	char odometer_char[10] = "0";
	
	double trip = 0;
	char trip_char_array[30];
	
	string odometer_string;

	std::cout << "Loop started" << std::endl;

	while (clusterRunning) {

		debug("Beginning of loop");

		// Which device are we using
		switch (device_code) {
			case DEVICE_CODE_GAVIN:

				debug("Host is gavin");

				connection.flushInput();
				serial_response = connection.read(9);

				debug("Connection read");

				if (is_number(serial_response)) {
					input_rpm = stoi(serial_response.substr(5, -1));
					input_speed = stoi(serial_response.substr(0, 5));
					input_speed /= 100.0;
				}

				break;
				
			case DEVICE_CODE_DEV:

				debug("Host is dev machine");

				SDL_GetMouseState(&mouseX, &mouseY);
				input_speed = ((double)(mouseX) / 7.5) + rand() % 3;
				input_rpm = ((double)(mouseY) * 8) + rand() % 20;

				break;
		}


		debug("Collecting data in vectors...");
		// Collect most recent speed and rpm data in vectors
		if (speedVector.size() >= SPEED_VECTOR_SIZE) {
			speedVector.erase(speedVector.begin());
		}

		if (rpmVector.size() >= RPM_VECTOR_SIZE) {
			rpmVector.erase(rpmVector.begin());
		}

		speedVector.push_back(input_speed);
		rpmVector.push_back(input_rpm);


		debug("Calculating speed and rpm...");
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
		
		debug("Calculating odometer...");
		// Calculate Odo
		odo_time_new = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> odo_duration_calc = odo_time_new - odo_time_old;
		odo_period = odo_duration_calc.count();
		odo_time_old = odo_time_new;

		odometer += (1000.0/3600.0) * speed * odo_period;
		trip += (1000.0/3600.0) * speed * odo_period;

		debug("Printing speed and rpm to char arrays...");
		if (trip < 1000) {
			snprintf(trip_char_array, 15, "%dm", (int)(trip));
		} else {
			snprintf(trip_char_array, 15, "%.3fkm", (double)(trip/1000));
		}

		debug("Registering input events");
		// Handle Inputs
		// Start event loop
		while (SDL_PollEvent(&event)){
			// Handle each specific event
			if (event.type == SDL_QUIT){
				clusterRunning = false;
				debug("Quit button pressed");
			}
			
			
			state = SDL_GetKeyboardState(NULL);
			STATE_leftIndicator = 0;
			STATE_rightIndicator = 0;
			STATE_headlights = 0;
			
			if (state[SDL_SCANCODE_LEFT]) {
				STATE_leftIndicator = 1;
				debug("Left indicator key pressed");
			}
			
			if (state[SDL_SCANCODE_RIGHT]) {
				STATE_rightIndicator = 1;
				debug("Right indicator key pressed");
			}
			
			if (state[SDL_SCANCODE_UP]) {
				STATE_headlights = 1;
				debug("Headlights key pressed");
			}
			
			if (state[SDL_SCANCODE_SPACE]) {
				STATE_leftIndicator = 1;
				STATE_rightIndicator = 1;
				debug("Hazards key pressed");
			}

			if (state[SDL_SCANCODE_ESCAPE]) {
				debug("Quit key pressed");
				return 0;
				
			}
			
			STATE_gavinTotal = 1 * STATE_headlights + \
							   2 * STATE_rightIndicator + \
							   4 * STATE_leftIndicator;

		}
		
		i++;

		// Debugging for freezing, remove once done
		STATE_headlights = (i % 50) > 25;

		STATE_gavinTotal = 1 * STATE_headlights + \
							   2 * STATE_rightIndicator + \
							   4 * STATE_leftIndicator;
		
		debug("Renderer setup...");

		SDL_SetRenderDrawColor(renderer, 0, 0, 0xFF, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		
		debug("Rendering base texture...");
		// Render Base
		SDL_RenderCopy(renderer, baseTexture, NULL, NULL);
		
		// Render Speed Needle
		dstRectSpeed.x = SCALE * 380;
		dstRectSpeed.y = SCALE * 10;
		dstRectSpeed.w = SCALE * 360;
		dstRectSpeed.h = SCALE * 720;
		
		needleCenterSpeed.x = SCALE * 0;
		needleCenterSpeed.y = SCALE * 360;
		
		debug("Rendering speed needle...");
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

		debug("Rendering RPM needle...");
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
		
		debug("Rendering Gavin image...");
		SDL_RenderCopy(renderer, gavinTextures[STATE_gavinTotal], NULL, &dstRectGavin);

		debug("Rendering Gavin shade...");
		// Render Shade (over gavin)
		SDL_RenderCopy(renderer, shadeTexture, NULL, &dstRectGavin);
		
		debug("Rendering overlay...");
		// Render Overlay
		SDL_RenderCopy(renderer, overlayTexture, NULL, NULL);
		
		debug("Printing speed to char array...");
		// Render Speed
		snprintf(speedChar, 10, "%.0f", speed);
		
		debug("Rendering speed...");
		SDL_Surface* speedSurface = TTF_RenderText_Blended(speedFont, speedChar, SDL_fontColour_white);
		SDL_Texture* speedTexture = SDL_CreateTextureFromSurface(renderer, speedSurface);


		dstSpeedText.x = SCALE * 380.0 - (double)(speedSurface->w)/2;
		dstSpeedText.y = SCALE * 365.3 - (double)(speedSurface->h)/2;
		dstSpeedText.w = SCALE * speedSurface->w;
		dstSpeedText.h = SCALE * speedSurface->h;
		SDL_RenderCopy(renderer, speedTexture, NULL, &dstSpeedText);
	

		debug("Printing speed to char array...");
		// Render RPM
		snprintf(rpmChar, 10, "%d", (((int)rpm / 10) * 10));
		
		debug("Rendering speed...");
		rpmSurface = TTF_RenderText_Blended(rpmFont, rpmChar, SDL_fontColour_white);
		rpmTexture = SDL_CreateTextureFromSurface(renderer, rpmSurface);

		dstRPMText.x = SCALE * 1540.0 - (double)(rpmSurface->w)/2;
		dstRPMText.y = SCALE * 365.3 - (double)(rpmSurface->h)/2;
		dstRPMText.w = SCALE * rpmSurface->w;
		dstRPMText.h = SCALE * rpmSurface->h;
		SDL_RenderCopy(renderer, rpmTexture, NULL, &dstRPMText);

		
		// Render Trip
		debug("Rendering trip...");
		tripSurface = TTF_RenderText_Blended(odoFont, trip_char_array, SDL_fontColour_white);
		tripTexture = SDL_CreateTextureFromSurface(renderer, tripSurface);

		dstTripText.x = SCALE * (1920/2) - (double)(tripSurface->w) + 400;
		dstTripText.y = SCALE * 652 + (double)(tripSurface->h)/2;
		dstTripText.w = SCALE * tripSurface->w;
		dstTripText.h = SCALE * tripSurface->h;

		SDL_RenderCopy(renderer, tripTexture, NULL, &dstTripText);
		
		
		// Render Odometer
		debug("Rendering odometer...");
		odometer_string = to_string((int)odometer/1000);
		
		for (int i = 0; i < 8; i++) {
			
			if (i < 3) {
				odometer_char_array[i] = odometer_string[i];
			} else if (i == 3) {
				odometer_char_array[i] = ' ';
			} else if (i >= 7) {
				odometer_char_array[i] = '\0';
			} else {
				odometer_char_array[i] = odometer_string[i-1];
			}
			
		}
		
		odoSurface = TTF_RenderText_Blended(odoFont, odometer_char_array, SDL_fontColour_grey);
		odoTexture = SDL_CreateTextureFromSurface(renderer, odoSurface);

		dstOdoText.x = SCALE * (1920/2) - (double)(odoSurface->w)/2;
		dstOdoText.y = SCALE * 652 + (double)(odoSurface->h)/2;
		dstOdoText.w = SCALE * odoSurface->w;
		dstOdoText.h = SCALE * odoSurface->h;
		
		SDL_RenderCopy(renderer, odoTexture, NULL, &dstOdoText);
		
		
		// Present Renderer
		debug("Presenting renderer...");
		SDL_RenderPresent(renderer);
	
		debug("Freeing surfaces and textures...");
		SDL_FreeSurface(speedSurface);
		SDL_DestroyTexture(speedTexture);
		
		SDL_FreeSurface(rpmSurface);
		SDL_DestroyTexture(rpmTexture);
		
		SDL_FreeSurface(odoSurface);
		SDL_DestroyTexture(odoTexture);

		SDL_FreeSurface(tripSurface);
		SDL_DestroyTexture(tripTexture);
		
		this_thread::sleep_for(chrono::milliseconds(16));
		
		
	}

	debug("Destroying window...");
	SDL_DestroyWindow(window);
	
	debug("Freeing image surfaces...");
	// Free png image surfaces
	SDL_FreeSurface(baseImage);
	SDL_FreeSurface(needleImage);
	SDL_FreeSurface(needleCoverImage);
	
	debug("Freeing image textures...");
	// Destroy textures
	SDL_DestroyTexture(baseTexture);
	SDL_DestroyTexture(needleTexture);
	SDL_DestroyTexture(needleCoverTexture);

	// Quit
	debug("Quitting SDL...");
	IMG_Quit();
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


void debug(string message) {
    
    if (TEXT_DEBUG) { std::cout << message << std::endl; }
    
}
