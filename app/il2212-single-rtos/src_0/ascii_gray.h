/*
 * File   : ascii_gray.h
 * Date   : 01.03.2017
 * Author : George Ungureanu <ugeorge@kth.se>
 * 
 * This file holds an array with the recommended ASCII characters
 * corresponding to 16 gray levels (from 0 = background to 
 * 255 = character)
 */ 

unsigned char NR_ASCII_CHARS = 16;
char asciiChars[] = {' ','.',':','-','=','+','/','t','z','U','w','*','0','#','%','@'};

/**
 * @brief Prints out an image of gray values into ASCII art format
 * @param image pointer to an image
 * @param x_dim image X dimention
 * @param y_dim image Y dimention
 */
void printAscii(unsigned char* image, int x_dim, int y_dim) {
  int k = 0;
  int l = 0;
  for(k = 0; k < y_dim; k++) {
    for(l = 0; l < x_dim; l++) {
      unsigned char pixel = image[k * y_dim + l];
      // Clamp pixel value to 255
      unsigned char c_pixel = pixel > 255 ? 255 : pixel;
      // Print normalized value as ASCII character
      printf("%4c", asciiChars[((NR_ASCII_CHARS - 1) * c_pixel) / 255]);
    }
    printf("\n");
  }
}

void printAscii_int(unsigned int* image, int x_dim, int y_dim) {
  int k = 0;
  int l = 0;
  for(k = 0; k < y_dim; k++) {
    for(l = 0; l < x_dim; l++) {
      unsigned int pixel = image[k * y_dim + l];
      // Clamp pixel value to 255
      unsigned int c_pixel = pixel > 255 ? 255 : pixel;
      // Print normalized value as ASCII character
      printf("%4c", asciiChars[((NR_ASCII_CHARS - 1) * c_pixel) / 255]);
    }
    printf("\n");
  }
}

/**
 * @brief Prints out an image of gray values into ASCII art format, hiding a 
 *        patch around an arbitrary position.
 * @param image pointer to an image
 * @param x_dim image X dimention
 * @param y_dim image Y dimention
 * @param x_pos X coordinate for center of hidden patch
 * @param y_pos Y coordinate for center of hidden patch
 * @param size radius of hidden patch
 * @param gray_value gray value of the hiding patch
 */
void printAsciiHidden(unsigned char* image, int x_dim, int y_dim,
		      int x_pos, int y_pos,
		      int size, unsigned int gray_value) {
  int k = 0;
  int l = 0;
  for(k = 0; k < y_dim; k++) {
    for(l = 0; l < x_dim; l++) {	
      if ((k >= y_pos - size) && (k < y_pos + size) && (l >= x_pos - size) && (l < x_pos + size)) {
	unsigned char pixel = gray_value;
	unsigned char c_pixel = pixel > 255 ? 255 : pixel;
	printf("%4c", asciiChars[((NR_ASCII_CHARS - 1) * c_pixel) / 255]);
      } else {
	unsigned char pixel = image[k * y_dim + l];
	unsigned char c_pixel = pixel > 255 ? 255 : pixel;
	printf("%4c", asciiChars[((NR_ASCII_CHARS - 1) * c_pixel) / 255]);	  
      }
    }
    printf("\n");
  }
}


/**
 * @brief It grays the rgb image, then it prints gray values into ASCII art format
 * @param image pointer to an image
 * @param x_dim image X dimention
 * @param y_dim image Y dimention
 * @param x_pos X coordinate for center of hidden patch
 * @param y_pos Y coordinate for center of hidden patch
 * @param size radius of hidden patch
 * @param gray_value gray value of the hiding patch
 */
void gray_displayAscii(unsigned char* image)
{
	unsigned char x, y;
	int r, g, b;
	unsigned char sizeX, sizeY;	
	unsigned char pixel;

	/*----------------- Read from SRAM information about size of the image  -----------------*/
	sizeX = *image++; 						//read sizeX of image
	sizeY = *image++; 						//read sizeY of image
	image++;							//maxcol
	
	/*---------------- Write on ON-CHIP MEMORY the grayed and cropped image ---------------- */	
	for(y = 0; y < sizeY; y++)
	{
		for (x = 0; x < sizeX; x++)
		{
			r = *image++;
			g = *image++;
			b = *image++; 
			pixel=(r*3125)/10000 + (g*5625)/10000 + (b*125)/1000;
			printf("%c ", asciiChars[((NR_ASCII_CHARS - 1) * pixel) / 255]);
		}
		printf("\n");
	}
	printf("\n");	
}

void gray_displayAsciiHidden(unsigned char* image, unsigned char coordX, unsigned char coordY)
{
	unsigned char x, y;
	int r, g, b;
	unsigned char sizeX, sizeY;	
	unsigned char pixel;

	/*----------------- Read from SRAM information about size of the image  -----------------*/
	sizeX = *image++;						//read sizeX of image
	sizeY = *image++;						//read sizeY of image
	image++;							//maxcol	
	
	/*---------------- Write on ON-CHIP MEMORY the grayed and cropped image ---------------- */	
	for(y = 0; y < sizeY; y++)
	{
		for (x = 0; x < sizeX; x++)
		{	
			r = *image++;
			g = *image++;
			b = *image++;
			if((x-coordX<5)&&(x-coordX>=0)&&(y-coordY<5)&&(y-coordY>=0)) pixel = 0;
			else pixel=(r*3125)/10000 + (g*5625)/10000 + (b*125)/1000;
			printf("%c ", asciiChars[((NR_ASCII_CHARS - 1) * pixel) / 255]);
		}
		printf("\n");
	}
	printf("\n");
}
