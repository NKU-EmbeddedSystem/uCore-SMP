#include <ucore/ucore.h>
#include "ff.h"
#include "init.h"

static FATFS fs;

void ffinit() {
//    FATFS fs;           /* Filesystem object */
    FRESULT res;  /* API result code */

    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        printf("f_mount() failed: %d\n", res);
        panic("");
        return;
    }

    res = f_chdrive("0:");
    if (res != FR_OK) {
        printf("f_chdrive() failed: %d\n", res);
        panic("");
        return;
    }
}
