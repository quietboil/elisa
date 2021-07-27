#ifndef __TRY_H
#define __TRY_H

#include <esp_err.h>

/**
 * Automates early return upon error from functions
 * that return esp_err_t.
 *
 * @param f function call that returns esp_err_t.
 */
#define TRY(f) do {         \
    esp_err_t err = (f);    \
    if ( err != ESP_OK ) {  \
        return err;         \
    }                       \
} while (0)

#endif