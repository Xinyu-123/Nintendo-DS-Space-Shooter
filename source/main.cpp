#include <gl2d.h>
#include <nds.h>

#include <stdio.h>

volatile int frame = 0;

//---------------------------------------------------------------------------------
void Vblank() {
//---------------------------------------------------------------------------------
	frame++;
}

int main(void){
	touchPosition touchXY;
	float spaceShipPosY = 90;
	videoSetMode(MODE_5_3D);
	glScreen2D();

	irqSet(IRQ_VBLANK, Vblank);

	consoleDemoInit();

    while(1) {
		glBegin2D();

		
		
		swiWaitForVBlank();
		scanKeys();
		int keys = keysDown();
		if (keys & KEY_START) break;

		touchRead(&touchXY);

		glBoxFilled(10, spaceShipPosY, 27, spaceShipPosY + 12, RGB15(255, 255, 0));

		glEnd2D();
		glFlush(0);


		// print at using ansi escape sequence \x1b[line;columnH 
		iprintf("\x1b[10;0HFrame = %d",frame);
		iprintf("\x1b[16;0HWassup bois I'm here x = %04X, %04X\n", touchXY.rawx, touchXY.px);
		iprintf("Touch y = %04X, %04X\n", touchXY.rawy, touchXY.py);		
	
	}

	return 0;
}