#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <ftdi.h>


// FT232H development module
#define VENDOR 0x0403
#define PRODUCT 0x6014


enum{
   SK = 0x01, // ADBUS0, SPI data clock
   DO = 0x02, // ADBUS1, SPI data out
   DI = 0x04, // ADBUS2, SPI data in
   CS = 0x08, // ADBUS3, SPI chip select
   L0 = 0x10, // ADBUS4, general-ourpose i/o, GPIOL0
   L1 = 0x20, // ADBUS5, general-ourpose i/o, GPIOL1
   L2 = 0x40, // ADBUS6, general-ourpose i/o, GPIOL2
   L3 = 0x80  // ADBUS7, general-ourpose i/o, GPIOL3
};

// Set these pins high
const uint8_t pinInitialState = CS;
// Use these pins as outputs
const uint8_t pinDirection    = SK|DO|CS;


uint8_t spi_init_ftdi( struct ftdi_context **ftdi){
    int ret=0;

    if ((*ftdi = ftdi_new()) == 0){
        fprintf( stdout, "ftdi_new failed\n");
        return EXIT_FAILURE;
    }

    ftdi_set_interface( *ftdi, INTERFACE_ANY);

    if ((ret = ftdi_usb_open(*ftdi, VENDOR, PRODUCT)) < 0){
        fprintf( stdout, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(*ftdi));
        ftdi_free(*ftdi);
        return EXIT_FAILURE;
    }

    unsigned int chipid;
    ftdi_read_chipid(*ftdi, &chipid);
    fprintf( stdout, "FTDI chipid: %X\n", chipid);

    ftdi_usb_reset( *ftdi);

    return EXIT_SUCCESS;
}


uint8_t get_pins( struct ftdi_context *ftdi){
    uint8_t pins=0;
    ftdi_read_pins( ftdi, &pins);
    return pins;
}

uint8_t spi_init( struct ftdi_context *ftdi){
    ftdi_set_bitmode( ftdi, 0x0, BITMODE_MPSSE);

    uint8_t spidata[10];

    // enable clock divide by 5
    spidata[0] = EN_DIV_5;

    // set clock to 2 MHz
    spidata[1] = TCK_DIVISOR;
    spidata[2] = 0x00;
    spidata[3] = 0x02;

    // init pins
    spidata[4] = SET_BITS_LOW;
    spidata[5] = pinInitialState;
    spidata[6] = pinDirection;

    // read 2 bytes
    spidata[7] = MPSSE_DO_READ;
    spidata[8] = 0x01;
    spidata[9] = 0x00;

    ftdi_write_data( ftdi, spidata, 10);

    usleep(5000);

    unsigned char data[10] = {0};
    ftdi_read_data( ftdi, data, 2);

    fprintf(stderr, "pins: 0x%X\n", get_pins( ftdi));

    return EXIT_SUCCESS;
}

uint8_t spi_write( struct ftdi_context *ftdi, uint8_t* data, int size){
    int ret = 0;
    uint8_t *spidata=(uint8_t *)malloc( size+9);

    uint8_t pins = get_pins( ftdi);

    spidata[0] = SET_BITS_LOW;
    spidata[1] = pins & (~CS);
    spidata[2] = pinDirection;


    spidata[3] = MPSSE_DO_WRITE | MPSSE_WRITE_NEG;
    spidata[4] = (size-1) & 0xFF;
    spidata[5] = ((size-1)>>8) & 0xFF;

    memcpy( &spidata[6], data, size);

    spidata[6+size] = SET_BITS_LOW;
    spidata[7+size] = pins | CS;
    spidata[8+size] = pinDirection;

    ret = ftdi_write_data( ftdi, spidata, size+9);

    free( spidata);
    return ret;
}



int main(int argc, char *argv[]){
    int ret;
    struct ftdi_context *ftdi = NULL;

    spi_init_ftdi( &ftdi);

    spi_init( ftdi);

    int c = 10;
    while( c-- > 0){
        spi_write( ftdi, (uint8_t*)"Hallo\n", 6);
        sleep(1);
    }



    ftdi_usb_reset(ftdi);

    if ((ret = ftdi_usb_close(ftdi)) < 0){
        fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }

    ftdi_free(ftdi);

    return EXIT_SUCCESS;
}
