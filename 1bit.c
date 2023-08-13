/* Uses Floyd–Steinberg dithering to create a 1-bit
 * image from a full color image. */

#include <stdio.h>
#include <MagickWand/MagickWand.h>

int main(int argc, char** argv){
	MagickWand* input_wand;
	MagickWand* output_wand;
	MagickBooleanType status;

	PixelIterator* input_iterator;
	PixelIterator* output_iterator;

	PixelWand** input_pixels;
	PixelWand** output_pixels;

	MagickWandGenesis();
	input_wand = NewMagickWand();
	status = MagickReadImage(input_wand, argv[1]);
	if(status == MagickFalse){
		printf("Failed to read image \"%s\"\n", argv[1]);
		exit(-1);
	}
	output_wand = CloneMagickWand(input_wand);

	input_iterator = NewPixelIterator(input_wand);
	output_iterator = NewPixelIterator(output_wand);
	if(input_iterator == (PixelIterator*) NULL ||
			output_iterator == (PixelIterator*) NULL){
		printf("Failed to create pixel input_iterator.\n");
		exit(-1);
	}

	long height = MagickGetImageHeight(input_wand);
	long width = MagickGetImageWidth(input_wand);

	long y, x;
	double h, s, input_lightness, output_lightness, error;

	// Instead of changing the pixel values of the next row, which
	// is slow, we store the amount by which we would have changed
	// them in a buffer.
	double* current_row_error_buffer = (double*) malloc(sizeof(double) * width);
	double* next_row_error_buffer = (double*) malloc(sizeof(double) * width);
	double* swap;

	for(y = 0; y < height; y++){
		input_pixels = PixelGetNextIteratorRow(input_iterator, &width);
		output_pixels = PixelGetNextIteratorRow(output_iterator, &width);
		if(input_pixels == (PixelWand**) NULL ||
				output_pixels == (PixelWand**) NULL){
			break;
		}

		swap = current_row_error_buffer;
		current_row_error_buffer = next_row_error_buffer;
		next_row_error_buffer = swap;
		for(x = 0; x < width; x++){
			next_row_error_buffer[x] = 0;
		}

		for(x = 0; x < width; x++){
			PixelGetHSL(input_pixels[x], &h, &s, &input_lightness);
			input_lightness += current_row_error_buffer[x];
			output_lightness = input_lightness > 0.5;
			PixelSetHSL(output_pixels[x], 0, 0, output_lightness);
			error = input_lightness - output_lightness;

			if(x < width - 1){
				current_row_error_buffer[x + 1] += error * 7 / 16;
			}
			if(x > 0){
				next_row_error_buffer[x - 1] += error * 3 / 16;
			}
			next_row_error_buffer[x] += error * 5 / 16;
			if(x < width - 1){
				next_row_error_buffer[x + 1] += error / 16;
			}
		}
		PixelSyncIterator(output_iterator);
	}

	if(y < height){
		printf("Failed to convert.\n");
		exit(-1);
	}

	input_iterator = DestroyPixelIterator(input_iterator);
	output_iterator = DestroyPixelIterator(output_iterator);
	input_wand = DestroyMagickWand(input_wand);

	status = MagickWriteImages(output_wand, argv[2], MagickTrue);
	if(status = MagickFalse){
		printf("Failed to save image.\n");
		exit(-1);
	}

	output_wand = DestroyMagickWand(output_wand);

	MagickWandTerminus();

	return 0;
}
