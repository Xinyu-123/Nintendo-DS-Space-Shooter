//Oh my gosh this was hard
#include <nds.h>
#include <maxmod9.h>
#include <stdio.h>

#include <nds/arm9/console.h> //basic print funcionality
#include <nds/arm9/trig_lut.h>

//Importing the sounds files generated from the audio folder
#include "soundbank.h"
#include "soundbank_bin.h"

//Importing the sprites and background image from the gfx folder
#include "ship.h"
#include "barrel.h"
//I decided to use a png for the background because i can't figure out how to set the VRAM_A_LCD as the background
//If you could send me some help as to how to display the VRAM_A_LCD in MODE_5_2D I would love to see how it's done
//Or if that's not how it's suppose to be displayed then just how to display the starfield.c in MODE_5
#include "starfield.h" 

//Using time.h to help with the random function for the y placement of the barrel
#include <time.h>

//This stores the location of the player ship
typedef struct 
{
	int x;
	int y;
}Ship;

//This stores the location of the barrel
typedef struct 
{
	int x;
	int y;
}Barrel;

// Function Prototypes
int generateBarrelY();
bool checkCollision(Ship* ship, Barrel* barrel);

int main(void){
	//Initialize the player and opening barrel
	Ship ship = {0, 0};
	Barrel barrel = {256, 0};

	//The score
	int score = 0;

	//Display Mode 5 gives you 4 2d backgrounds to work with.
	videoSetMode(MODE_5_2D);

	//The scroll values for the background
	s16 scrollX = 64;
	s16 scrollY = 128;

	//The scale for the background
	s16 scaleX = 1 << 8;
	s16 scaleY = 1 << 8;

	//I tried increasing the scale of the background but I'm pretty sure this doesn't do anything
	scaleX -= 100;
	scaleY -= 100;

	//this is the screen pixel that the image will rotate about
	s16 rcX = 128;
	s16 rcY = 96;
	
	//Store the sprite tiles in VRAM A
	vramSetBankA(VRAM_A_MAIN_SPRITE);

	oamInit(&oamMain, SpriteMapping_1D_32, true);
	
	// unlock vram (cannot write to vram while it is mapped as a palette)
	vramSetBankF(VRAM_F_LCD); 

	//This stores the different palletes in different memory locations
	int i;
	for (i=0;i<=shipPalLen;i++)
		VRAM_F_EXT_SPR_PALETTE[0][i] = shipPal[i];
	for (i=0;i<=barrelPalLen;i++)
		VRAM_F_EXT_SPR_PALETTE[1][i] = barrelPal[i];


	// set vram to ex palette
	vramSetBankF(VRAM_F_SPRITE_EXT_PALETTE);
	
	u16* gfx = oamAllocateGfx(&oamMain, SpriteSize_32x32,SpriteColorFormat_256Color);//make room for the sprite
	u16* gfx2 = oamAllocateGfx(&oamMain, SpriteSize_32x32,SpriteColorFormat_256Color);//make room for the sprite

	// Copy the sprites into the allocated room
	dmaCopy(shipTiles, gfx, shipTilesLen);
	dmaCopy(barrelTiles, gfx2, barrelTilesLen);

	// This enables the use of the sub screen
	consoleDemoInit();

	//Setting up and playing the song using maxmod
	mmInitDefaultMem((mm_addr)soundbank_bin);
	mmLoad( MOD_FLATOUTLIES );
	mmStart( MOD_FLATOUTLIES, MM_PLAY_LOOP );

	//Generates a unique random value whenever rand() is called now
	srand(time(NULL));

	//Give the barrel a random Y value within the screen
	barrel.y = generateBarrelY();

	//Set the background
 	vramSetBankA(VRAM_A_MAIN_BG);

	//Initialize and store the background information in VRAM_A_MAIN_BG
	int bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0,0);
	dmaCopy(starfieldTiles, bgGetGfxPtr(bg3), 256*256);
	dmaCopy(starfieldPal, BG_PALETTE, 256*2);

	//Set the values of background that don't change
	bgSetCenter(bg3, rcX, rcY);
	bgSetRotateScale(bg3, 0, scaleX, scaleY);
	bgSetPriority(bg3, 3);

    while(1) {
		//increment score
		score++;
		//check to see if a key is being held down
		scanKeys();
		int held = keysHeld();

		//fly the barrel towards the player
		barrel.x -= 1;
		
		//If the barrel hits the left side, generate a new Y value and reset the X position of the barrel
		if(barrel.x < -10){
			barrel.y = generateBarrelY();
			barrel.x = 256;
		}


		//Player Controls
		if(held &KEY_UP && ship.y > 0)
			ship.y -= 1;

		if(held & KEY_DOWN && ship.y < 165)
			ship.y += 1;

		// Background Scrolling
		// I'm aware this looks really janky
		scrollX++;
		if(scrollX > 150)
			scrollX = 64;
		
		//Check for a barrel to ship collision
		if(checkCollision(&ship, &barrel)){
			//Display the you lose screen
			while(1){
				scanKeys();
				int held2 = keysHeld();
				iprintf("\x1b[16;0HYou have died.\n");
				iprintf("\x1b[17;0HPress Start to restart\n");	
				//Restart with the start key
				if(held2 & KEY_START){
					//Clear the strings from the subscreen
					//I'm pretty sure there's a better way to clear the sub oam but I didnt find it
					iprintf("\x1b[10;0H                                 ");
					iprintf("\x1b[16;0H                                 ");
					iprintf("\x1b[17;0H                                 ");
					//Reset the score
					score = 0;
					//Reset the barrel
					barrel.y = generateBarrelY();
					barrel.x = 256;
					//Continue the new game
					break;
				}
			}
		}


		//Prints the score
		iprintf("\x1b[10;0HScore = %d", score);

		//Sets the new scroll of the background and updates the background
		bgSetScroll(bg3, scrollX, scrollY);
		bgUpdate();

		//Display the two sprites on the main screen. Figuring out how to store both palettes separately took me a while.
		oamSet(&oamMain, 0, ship.x, ship.y, 1, 0, SpriteSize_32x32, SpriteColorFormat_256Color, 
			gfx, 0, false, false, false, false, false);

		oamSet(&oamMain, 1, barrel.x, barrel.y, 0, 1, SpriteSize_32x32, SpriteColorFormat_256Color, 
			gfx2, 0, false, false, false, false, false);




		swiWaitForVBlank();


		oamUpdate(&oamMain);
		oamUpdate(&oamSub);
	}

	return 0;
}

//Generate a random Y position within the screen and set it to the barrel
int generateBarrelY(){
	return rand() % 180;
}

//Check to see a collision between the barrel and ship
bool checkCollision(Ship* ship, Barrel* barrel){
	int dy = ship->y - barrel->y;

	if(barrel->x <= 30 && (-15 < dy && dy < 15))
		return true;
	return false;
}