//Oh my gosh this was hard
#include <nds.h>
#include <maxmod9.h>
#include <stdio.h>
#include <stdlib.h>
#include <nds/registers_alt.h>

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

#define NUM_STARS 40
#define UPLEFT (KEY_UP | KEY_LEFT)
#define UPRIGHT (KEY_UP | KEY_RIGHT)
#define DOWNLEFT (KEY_DOWN | KEY_LEFT)
#define DOWNRIGHT (KEY_DOWN | KEY_RIGHT)

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

typedef struct 
{
	int x;
	int y;
	int speed;
	unsigned short color;
 
}Star;
 

// Function Prototypes
int generateBarrelY();
bool checkCollision(Ship* ship, Barrel* barrel);


// Function Prototypes
void setStar(Star* star, int xSpeed, int ySpeed, int xStart, int yStart);
void MoveStar(Star* star);
void ClearScreen(u16* vidBuff);
void InitStars(void);
void DrawStar(Star* star, u16* vidBuff);
void EraseStar(Star* star, u16* vidBuff);

 
Star stars[NUM_STARS];
int lastKey;
 
void MoveStar(Star* star)
{
	setStar(star, -star->speed, 0, SCREEN_WIDTH, rand() % SCREEN_HEIGHT);
}

void setStar(Star* star, int xSpeed, int ySpeed, int xStart, int yStart)
{
	star->x += xSpeed;
	star->y += ySpeed;
 
	if(star->y < 0 || star->y > SCREEN_HEIGHT)
	{
		star->x = xStart;
		star->y = yStart;
		star->speed = rand() % 4 + 1;	
	}
	else if(star->x < 0 || star->x > SCREEN_WIDTH)
	{
		star->x = xStart;
		star->y = yStart;
		star->speed = rand() % 4 + 1;	
	}
}
 
void ClearScreen(u16* vidBuff)
{
     int i;    
     for(i = 0; i < 256 * 192; i++)
           vidBuff[i] = RGB15(0,0,0);
}
 
void InitStars(void)
{
	int i;
	for(i = 0; i < NUM_STARS; i++)
	{
		stars[i].color = rand(); // both color & rand() are a 16bit value
		stars[i].x = rand() % 256;
		stars[i].y = rand() % 192;
		stars[i].speed = rand() % 4 + 1;
	}
}
void DrawStar(Star* star, u16* vidBuff)
{
	vidBuff[star->x + star->y * SCREEN_WIDTH] = star->color;
}
 
void EraseStar(Star* star, u16* vidBuff)
{
	vidBuff[star->x + star->y * SCREEN_WIDTH] = RGB15(0,0,0);
}

int main(void){
	//Initialize the player and opening barrel
	Ship ship = {0, 0};
	Barrel barrel = {256, 0};
	Barrel barrel2 = {256, 0};
	Barrel barrel3 = {256, 0};


	//The score
	int score = 0;

	//pointer to virtual memory 0x6000000
	u16* vidBuff = (u16*)BG_BMP_RAM(0);

	//Display Mode 5 gives you 4 2d backgrounds to work with.
	//Set background 3 to active to display the bitmapped stars
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);


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
	barrel2.y = generateBarrelY();
	barrel3.y = generateBarrelY();

	//Offset the barrel's x position
	barrel.x = 300;
	barrel2.x = 350;

		//Setup BG3 to be the bitmapped representation of VRAM_A
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);	
	BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(0);

	BG3_XDX = 1 << 8;
	BG3_XDY = 0 ;
	BG3_YDX = 0;
	BG3_YDY = 1 << 8;

	//Clear the screen and initialize the background star pixels
	ClearScreen(vidBuff);
	InitStars();
	


    while(1) {
		//increment score
		score++;
		//check to see if a key is being held down
		scanKeys();
		int held = keysHeld();

		//fly the barrel towards the player
		barrel.x -= 1;
		barrel2.x -= 1;
		barrel3.x -= 1;

		
		//If the barrel hits the left side, generate a new Y value and reset the X position of the barrel
		if(barrel.x < -10){
			barrel.y = generateBarrelY();
			barrel.x = 256;
		}

		if(barrel2.x < -10){
			barrel2.y = generateBarrelY();
			barrel2.x = 256;
		}

		if(barrel3.x < -10){
			barrel3.y = generateBarrelY();
			barrel3.x = 256;
		}



		//Player Controls
		if(held &KEY_UP && ship.y > 0)
			ship.y -= 1;

		if(held & KEY_DOWN && ship.y < 165)
			ship.y += 1;
		
		//Check for a barrel to ship collision
		if(checkCollision(&ship, &barrel) || checkCollision(&ship, &barrel2) || checkCollision(&ship, &barrel3)){
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
					barrel2.y = generateBarrelY();
					barrel3.y = generateBarrelY();
					barrel.x = 300;
					barrel2.x = 350;
					barrel3.x = 256;

					//Continue the new game
					break;
				}
			}
		}


		//Prints the score
		iprintf("\x1b[10;0HScore = %d", score);

		//Display the two sprites on the main screen. Figuring out how to store both palettes separately took me a while.
		oamSet(&oamMain, 0, ship.x, ship.y, 1, 0, SpriteSize_32x32, SpriteColorFormat_256Color, 
			gfx, 0, false, false, false, false, false);

		oamSet(&oamMain, 1, barrel.x, barrel.y, 0, 1, SpriteSize_32x32, SpriteColorFormat_256Color, 
			gfx2, 0, false, false, false, false, false);

		oamSet(&oamMain, 2, barrel2.x, barrel2.y, 0, 1, SpriteSize_32x32, SpriteColorFormat_256Color, 
			gfx2, 0, false, false, false, false, false);

		oamSet(&oamMain, 3, barrel3.x, barrel3.y, 0, 1, SpriteSize_32x32, SpriteColorFormat_256Color, 
			gfx2, 0, false, false, false, false, false);

		for(i = 0; i < NUM_STARS; i++)
		{
			EraseStar(&stars[i], vidBuff);
			MoveStar(&stars[i]);
			DrawStar(&stars[i], vidBuff);
		}

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