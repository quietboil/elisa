# Sunrise/Sunset Computation

This module is an adaptation of `sunriset.c` that was written by Paul Schlyter in 1989 and released to public domain in 1992. The demo `main` implementation and day length computation were removed from this adaptation. The rest of the original implementation was kept almost intact, barring a few cosmetic changes that were made to make it a bit more readable.

## API

Besides the "workhorse" `__sunriset__` function that computes times when the Sun crosses indicated altitude there are 4 helper functions that compute start and end times for daytime, civil, nautical and astronomical twilights. See `sunriset.h` for details.
