
/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec:112901  Name:Tiago Albuquerque
// NMex:114192  Name:Diogo Domingues
// 
// 
// Date:24/11/2023
//

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "instrumentation.h"

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
// 
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image {
  int width;
  int height;
  int maxval;   // maximum gray value (pixels with maxval are pure WHITE)
  uint8* pixel; // pixel data (a raster scan)
};


// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
// 
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char* errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char* ImageErrMsg() { ///
  return errCause;
}


// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success = 
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
// 
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
// 
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
// 
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)


// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char* failmsg) {
  errCause = (char*)(condition ? "" : failmsg);
  return condition;
}


/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) { ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  InstrName[1] = "count";   // InstrCount[1] will count function comparsions
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
#define COUNT  InstrCount[1]

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!


/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCreate(int width, int height, uint8 maxval) {
  //Verificar se a largura e a altura da imagem sao positivas
  assert(width >= 0);
  assert(height >= 0);
  //Verificar se o maxval da imagem é maior que 0 e menor ou igual ao PixMax
  assert(0 < maxval && maxval <= PixMax);

  //Alocar memória para a imagem
  Image img = calloc(1,sizeof(struct image));
  //Verificar se a alocação de memória para a imagem foi bem sucedida
  if (img == NULL) {
    //Se não foi bem sucedida imprimir a mensagem de erro
    errCause = "Memory allocation failed";
    return NULL;
  }

  //Definir os valores da imagem
  img->width = width;
  img->height = height;
  img->maxval = maxval;

  //Alocar memoria para o array de pixeis da imagem (dados dos pixeis)
  img->pixel = (uint8*)malloc(width * height * sizeof(uint8));
  //Verificar se a alocação de memória para o array de pixeis foi bem sucedida
  if (img->pixel == NULL) {
    //Se não foi bem sucedida imprimir a mensagem de erro
    errCause = "Memory allocation failed";
    free(img);
    return NULL;
  }
  //Se as duas alocações de memória foram bem sucedidas, então retornar a imagem
  return img;
}

/// Destroy the image pointed to by (*imgp).
/// imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.
void ImageDestroy(Image* imgp) {
  //Verificar se o ponteiro para a imagem existe
  if (*imgp == NULL) {
    //Se não existir, não faz nada
    return;
  }

  //Libertar a memoria alocada para o array de pixeis da imagem
  free((*imgp)->pixel);
  //Libertar a memoria alocada para a imagem
  free(*imgp);
  //Definir o valor do ponteiro para a imagem como NULL
  *imgp = NULL;
}


/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char* filename) { ///
  int w, h;
  int maxval;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  int success = 
  check( (f = fopen(filename, "rb")) != NULL, "Open failed" ) &&
  // Parse PGM header
  check( fscanf(f, "P%c ", &c) == 1 && c == '5' , "Invalid file format" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &w) == 1 && w >= 0 , "Invalid width" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &h) == 1 && h >= 0 , "Invalid height" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int)PixMax , "Invalid maxval" ) &&
  check( fscanf(f, "%c", &c) == 1 && isspace(c) , "Whitespace expected" ) &&
  // Allocate image
  (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
  // Read pixels
  check( fread(img->pixel, sizeof(uint8), w*h, f) == w*h , "Reading pixels" );
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (!success) {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL) fclose(f);
  return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char* filename) { ///
  assert (img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE* f = NULL;

  int success =
  check( (f = fopen(filename, "wb")) != NULL, "Open failed" ) &&
  check( fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0, "Writing header failed" ) &&
  check( fwrite(img->pixel, sizeof(uint8), w*h, f) == w*h, "Writing pixels failed" ); 
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (f != NULL) fclose(f);
  return success;
}


/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img) { ///
  assert (img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(Image img) { ///
  assert (img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img) { ///
  assert (img != NULL);
  return img->maxval;
}

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8* min, uint8* max) { ///
  //Verificar se a imagem existe
  assert (img != NULL);
  //Verificar se os ponteiros min e max existem
  assert (min != NULL);
  assert (max != NULL);

  //Definir o valor inicial dos ponteiros min e max
  *min = PixMax;
  *max = 0;

  //Iterar sobre todas as linhas da imagem
  for (int i = 0; i < ImageHeight(img); i++) {
    //Iterar sobre cada pixel dessa linha
    for (int j = 0; j < ImageWidth(img); j++) {
      //Obter o valor de cinzento do pixel na posição (j, i)
      uint8 pixelValue = ImageGetPixel(img, j, i);
      //Verificar se o valor de cinzento do pixel é menor que o valor atual do min
      if (pixelValue < *min) {
        //Se for menor, o novo valor do min é o valor de cinzento do pixel
        *min = pixelValue;
      }
      //Verificar se o valor de cinzento do pixel é maior que o valor atual do max
      if (pixelValue > *max) {
        //Se for maior, o novo valor do max é o valor de cinzento do pixel
        *max = pixelValue;
      }
    }
  }
}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y) { ///
  assert (img != NULL);
  return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h) { ///
  //Verificar se a imagem existe
  assert (img != NULL);
  //Verificar se o retângulo esta dentro da imagem
  for (int i = x; i < x + w; i++) {
    for (int j = y; j < y + h; j++) {
      //Verificar se o pixel na posição (i, j) existe
      if (!ImageValidPos(img, i, j)) {
        //Se nao existir, entao o retângulo nao esta dentro da imagem
        return 0;
      }
    }
  }
  //Se existir, entao o retângulo esta dentro da imagem
  return 1;
}

/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to 
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel. 
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y) {
 //Calcular o índice do pixel nas coordenadas (x,y)
 int index = y * img->width + x;
 //Verificar se o índice do pixel esta dentro da img
 assert (0 <= index && index < img->width*img->height);
 return index;
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (read)
  return img->pixel[G(img, x, y)];
} 

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (store)
  img->pixel[G(img, x, y)] = level;
} 


/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.


/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img) {
  //Verificar se a imagem existe
  assert(img != NULL);

  //Iterar sobre todas as linhas da imagem
  for (int y = 0; y < ImageHeight(img); y++) {
    //Iterar sobre cada pixel dessa linha
    for (int x = 0; x < ImageWidth(img); x++) {
      //Obter o valor de cinzento do pixel na posição (x, y)
      uint8 currentLevel = ImageGetPixel(img, x, y);
      //Definir o valor de cinzento do pixel na posição (x, y) com o valor obtido subtraido ao maxval da imagem
      ImageSetPixel(img, x, y, ImageMaxval(img) - currentLevel);
    }
  }
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr) {
  //Verificar se a imagem existe
  assert (img != NULL);   
  //Iterar sobre todas as linhas da imagem
  for (int y = 0; y < ImageHeight(img); y++) {
    //Iterar sobre cada pixel dessa linha
    for (int x = 0; x < ImageWidth(img); x++) {
      //Obter o nivel de cinzento do pixel
      uint8 currentLevel = ImageGetPixel(img, x, y);//obter o valor do pixel em (x,y)

      //Verificar se o nivel de cinzento do pixel e menor que o threshold
      if (currentLevel < thr) {
        //Se for menor que o threshold, o pixel ficará preto
          ImageSetPixel(img, x, y, 0);
      } else {
        //Se for maior ou igual ao threshold, o pixel ficará branco
          ImageSetPixel(img, x, y, ImageMaxval(img));
      }
    }
  }
}

/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double fator) {
  //Verificar se a imagem existe
  assert(img != NULL);
  //Verificar se o fator é maior ou igual a 0
  assert(fator >= 0.0);

  //Iterar sobre todas as linhas da imagem
  for (int y = 0; y < ImageHeight(img); y++) {
    //Iterar sobre cada pixel dessa linha
    for (int x = 0; x < ImageWidth(img); x++) {
      //Obter o valor de cinzento do pixel na posição (x, y)
      uint8 pixelValue = ImageGetPixel(img, x, y);

      //Calcular o novo valor de cinzento do pixel (multiplicandpo o valor obtido de pixelValue pelo fator de brilho - somamos 0.5 para arredondar o valor)
      uint8 newPixelValue = (uint8)(pixelValue * fator + 0.5);   
      if (newPixelValue > ImageMaxval(img)) {
        //Se o novo valor de cinzento do pixel for maior que o maxval da imgOriginal, entao o novo valor do pixel é o seu maxval
        ImageSetPixel(img, x, y, ImageMaxval(img));
      } else {
        //Caso contrário, o novo valor do pixel é o newPixelValue
        ImageSetPixel(img, x, y, newPixelValue);
      }
    }
  }
}


/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
/// 
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint: 
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees clockwise.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageRotate(Image img) {
  //Verificar se a imagem existe
  assert(img != NULL);

  //Criar uma nova imagem com as dimensões da img, mas com a largura e altura trocadas (para rodar a imagem)
  Image rotatedImage = ImageCreate(img->height, img->width, img->maxval);
  if (rotatedImage == NULL) {
    //Verificar se a imagem foi criada
    return NULL;
  }

  //Iterar sobre todas as linhas da imagem
  for (int i = 0; i < ImageHeight(img); ++i) {
    //Iterar sobre cada pixel dessa linha
    for (int j = 0; j < ImageWidth(img); ++j) {
      //Obter o valor de cinzento do pixel na img original na posição (j, i)
      uint8 pixelValue = ImageGetPixel(img, i, j);
      //Definir o valor de cinzento do pixel na rotatedImage na posição (j, ImageHeight(img) - 1 - i) com o valor obtido da img
      ImageSetPixel(rotatedImage, j, ImageHeight(img) - 1 - i, pixelValue);
    }
  }
  //Retornar a imagem rodada
  return rotatedImage;
}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageMirror(Image img) {
  //Verificar se a imagem existe
  assert(img != NULL);

  //Criar uma nova imagem com as dimensões da img
  Image mirrorImg = ImageCreate(img->width, img->height, img->maxval);
  if (mirrorImg == NULL) {
    //Verificar se a imagem foi criada
    return NULL;
  }
  
  //Iterar sobre todas as linhas da imagem
  for (int i = 0; i < ImageHeight(img); i++) {
    //Iterar sobre cada pixel dessa linha
    for (int j = 0; j < ImageWidth(img); j++) {
      //Obter o valor de cinzento do pixel na img original na posição (j, i)
      uint8 pixelValue = ImageGetPixel(img, j, i);
      //Definir o valor de cinzento do pixel na mirrorImg na posição (ImageWidth(img) - 1 - j, i) com o valor obtido da img
      ImageSetPixel(mirrorImg, ImageWidth(img) - 1 - j, i, pixelValue);   
    }
  }
  //Retornar a imagem espelhada
  return mirrorImg;
}

/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCrop(Image img, int x, int y, int w, int h) {
  //Verificar se a imagem existe
  assert(img != NULL);
  //Verificar se o retângulo esta dentro da imagem
  assert(ImageValidRect(img, x, y, w, h));

  //Criar uma nova imagem com as dimensões do retângulo
  Image cropImg = ImageCreate(w, h, ImageMaxval(img));
  if (cropImg == NULL) {
    //Verificar se a imagem foi criada
    return NULL;
  }

  //Iterar sobre todas as linhas da cropImg
  for (int i = 0; i < h; i++) {
    //Iterar sobre cada pixel dessa cropImg
    for (int j = 0; j < w; j++) {
      //Obter o valor de cinzento do pixel na img original na posição (x + j, y + i)
      uint8 pixelValue = ImageGetPixel(img, x + j, y + i);
      //Definir o valor de cinzento do pixel na imagem recortada na posição (j, i) com o valor obtido da img original
      ImageSetPixel(cropImg, j, i, pixelValue);    
    }
  }
  //Retornar a imagem recortada
  return cropImg;
}


/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
void ImagePaste(Image img1, int x, int y, Image img2) {
  //Verificar se a img1 e a img2 existem
  assert(img1 != NULL);
  assert(img2 != NULL);

  //Verificar se a img2 cabe dentro da img1 na posiçao (x,y)
  assert(ImageValidRect(img1, x, y, ImageWidth(img2), ImageHeight(img2)));

  //Iterar sobre todas as linhas da img2
  for (int j = 0; j < ImageHeight(img2); j++) {
    //Iterar sobre cada pixel dessa linha
    for (int i = 0; i < ImageWidth(img2); i++) {
      //Obter o valor de cinzento do pixel na img2 na posição (i, j)
      uint8 pixelValue = ImageGetPixel(img2, i, j);
      //Definir o valor de cinzento do pixel na img1 na posição (x + i, y + j) com o valor obtido
      ImageSetPixel(img1, x + i, y + j, pixelValue);
    }
  }
}

/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha) {
    //Verificar se a img1 e a img2 existem
    assert(img1 != NULL);
    assert(img2 != NULL);
    //Verificar se a img2 cabe dentro da img1 na posiçao (x,y)
    assert(ImageValidRect(img1, x, y, ImageWidth(img2), ImageHeight(img2)));

    //Iterar sobre todas as linhas da img2
    for (int j = 0; j < ImageHeight(img2); j++) {
        //Iterar sobre cada pixel dessa linha
        for (int i = 0; i < ImageWidth(img2); i++) {
            //Obter o valor de cinzento do pixel na img1 na posição (x + i, y + j)
            uint8 pixelValue1 = ImageGetPixel(img1, x + i, y + j);
            //Obter o valor de cinzento do pixel na img2 na posição (i, j)
            uint8 pixelValue2 = ImageGetPixel(img2, i, j);
            //Calcular o novo valor de cinzento do pixel na img1 na posição (x + i, y + j)
            uint8 newPixelValue = (uint8)(pixelValue1 * (1 - alpha) + pixelValue2 * alpha + 0.5);
            //Verificar se o novo valor de cinzento do pixel na img1 na posição (x + i, y + j) é maior que o maxval da img1
            if (newPixelValue > ImageMaxval(img1)) {
                //Se for maior, o novo valor de cinzento do pixel na img1 na posição (x + i, y + j) é o maxval da img1
                ImageSetPixel(img1, x + i, y + j, ImageMaxval(img1));
            } else {
                //Se for menor, o novo valor de cinzento do pixel na img1 na posição (x + i, y + j) é o newPixelValue
                ImageSetPixel(img1, x + i, y + j, newPixelValue);
            }
        }
    }
}


/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.
int ImageMatchSubImage(Image img1, int x, int y, Image img2) {
  //Verificar se a img1 e a img2 existem
  assert (img1 != NULL);
  assert (img2 != NULL);  
  //Verificar se a img2 cabe dentro da img1 na posiçao (x,y)
  assert (ImageValidPos(img1, x, y));

  //Iterar sobre todas as linhas da img2
  for (int i = 0; i < img2->height; i++) {
    //Iterar sobre cada pixel dessa linha
    for (int j = 0; j < img2->width; j++) {
    
      COUNT += 1;  // Increment COUNT for each comparison
      
      //Verificar se o pixel na posição (x + j, y + i) da img1 é diferente do pixel na posição (j, i) da img2
      if (ImageGetPixel(img1, x + j, y + i) != ImageGetPixel(img2, j, i)) {
        //Se forem diferentes, entao nao encontramos a img2 dentro da img1
        return 0;
      }
    }
  }
  //Se forem iguais, entao encontramos a img2 dentro da img1
  return 1;
}

/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px, *py).
/// If no match is found, returns 0 and (*px, *py) are left untouched.
int ImageLocateSubImage(Image img1, int* px, int* py, Image img2) {
  //Verificar se a img1 e a img2 existem
  assert(img1 != NULL);
  assert(img2 != NULL);

  //Obter a largura e a altura da img1 e da img2 para não ter que chamar as funções ImageWidth e ImageHeight nos for loops
  int img1Width = ImageWidth(img1);
  int img1Height = ImageHeight(img1);
  int img2Width = ImageWidth(img2);
  int img2Height = ImageHeight(img2);

  //Iterar sobre todas as linhas da img1
  for (int i = 0; i < img1Height - img2Height; i++) {
    //Iterar sobre cada pixel dessa linha
    for (int j = 0; j < img1Width - img2Width; j++) {
      //Chamar a função ImageMatchSubImage para verificar se a img2 existe dentro da img1 na posição (j, i)
      if (ImageMatchSubImage(img1, j, i, img2)) {
        //Se existir, então definir os valores de px e py com os valores obtidos e retornar 1
        *px = j;
        *py = i;
        return 1;
      }
    }
  }
  //Se não existir, então retornar 0
  return 0;
}


/// Filtering

/// Blur an image by applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.



/*                                                    PRIMEIRA IMPLEMENTAÇAO
void ImageBlur(Image img, int dx, int dy) {
  //Verificar se a imagem existe
  assert(img != NULL);

  //Criar uma nova imagem temporaria
  Image blurImg = ImageCreate(ImageWidth(img), ImageHeight(img), ImageMaxval(img));

  //Verificar se a imagem temporaria foi criada
  if (blurImg == NULL) {
    return;
  }

  //Iterar sobre todas as linhas da imagem
  for (int i = 0; i < img->height; i++) {
    //Iterar sobre cada pixel dessa linha
    for (int j = 0; j < img->width; j++) {

      //Variável para guardar a soma dos pixeis
      int sum = 0;
      //Variável para guardar o número de pixeis
      int count = 0;

      //Iterar sobre os pixeis dentro do retangulo [y-dy, y+dy]
      for (int k = i - dy; k <= i + dy; k++) {
        //Iterar sobre os pixeis dentro do retangulo [x-dx, x+dx]
        for (int l = j - dx; l <= j + dx; l++) {

          //Verificar se o pixel na posição (l, k) existe
          if (ImageValidPos(img, l, k)) {
            //Se existir, somar o valor do pixel na posição (l, k) à variável sum e incrementar a variável count
            sum += ImageGetPixel(img, l, k);
            count++;
          }
        }
      }   

      //Calcular a média do valor dos pixeis dentro do retangulo
      uint8 pixelValue = (uint8)((sum + count * 0.5 )/ count);
      //Definir o valor do pixel na imagem blurImg na posição (j, i) com o valor obtido
      ImageSetPixel(blurImg, j, i, pixelValue);
    }
  }

  //Copiar a imagem blurImg para a imagem img:
  //Iterar sobre todas as linhas da imagem
  for (int i = 0; i < img->height; i++) {
    //Iterar sobre cada pixel dessa linha
    for (int j = 0; j < img->width; j++) {

      //Definir o valor do pixel na imagem img na posição (j, i) com o valor obtido da imagem blurImg
      ImageSetPixel(img, j, i, ImageGetPixel(blurImg, j, i));
    }
  }

  //Apagar a imagem blurImg para libertar a memoria alocada
  ImageDestroy(&blurImg);
}



*/



void ImageBlur(Image img, int dx, int dy) {
  //Verificar se a imagem existe
  assert(img != NULL);

  //Obter a largura e a altura da imagem para não ter que chamar as funções ImageWidth e ImageHeight nos for loops
  int imgWidth = ImageWidth(img);
  int imgHeight = ImageHeight(img);

  //Criar uma nova imagem temporaria
  Image blurImg = ImageCreate(imgWidth, imgHeight, ImageMaxval(img));
  //Verificar se a imagem temporaria foi criada
  if (blurImg == NULL) {
    return;
  }

  //Criar um array 2D para guardar os pixeis da imagem original
  uint8 **pixels = malloc(imgHeight * sizeof(uint8 *));
  for (int i = 0; i < imgHeight; i++) {
    pixels[i] = malloc(imgWidth * sizeof(uint8));
    for (int j = 0; j < imgWidth; j++) {
      pixels[i][j] = ImageGetPixel(img, j, i);
    }
  }

  //Iterar sobre todas as linhas da imagem
  for (int i = 0; i < imgHeight; i++) {
    //Iterar sobre cada pixel dessa linha
    for (int j = 0; j < imgWidth; j++) {
      //Variável para guardar a soma dos pixeis
      int sum = 0;
      //Variável para guardar o número de pixeis
      int count = 0;

      //Calcular os limites do retangulo [y-dy, y+dy] e [x-dx, x+dx]
      int startY = i - dy < 0 ? 0 : i - dy;
      int endY = i + dy >= imgHeight ? imgHeight - 1 : i + dy;
      int startX = j - dx < 0 ? 0 : j - dx;
      int endX = j + dx >= imgWidth ? imgWidth - 1 : j + dx;

      //Iterar sobre os pixeis dentro do retangulo [y-dy, y+dy] e [x-dx, x+dx]
      for (int k = startY; k <= endY; k++) {
        for (int l = startX; l <= endX; l++) {
          //Somar o valor do pixel na posição (l, k) à variável sum e incrementar a variável count
          sum += pixels[k][l];
          count++;
        }
      }
      //Calcular a média do valor dos pixeis dentro do retangulo
      uint8 pixelValue = (uint8)((sum + count * 0.5) / count);
      //Definir o valor do pixel na imagem blurImg na posição (j, i) com o valor obtido
      ImageSetPixel(blurImg, j, i, pixelValue);
    }
  }

  //Copiar a imagem blurImg para a imagem img:
  //Iterar sobre todas as linhas da imagem
  for (int i = 0; i < imgHeight; i++) {
    //Iterar sobre cada pixel dessa linha
    for (int j = 0; j < imgWidth; j++) {
      //Definir o valor do pixel na imagem img na posição (j, i) com o valor obtido da imagem blurImg
      uint8 pixelValue = ImageGetPixel(blurImg, j, i);
      ImageSetPixel(img, j, i, pixelValue);
    }
  }

  //Libertar a memoria alocada para o array de pixeis da imagem original
  for (int i = 0; i < imgHeight; i++) {
    free(pixels[i]);
  }
  free(pixels);

  //Apagar a imagem blurImg
  ImageDestroy(&blurImg);
}