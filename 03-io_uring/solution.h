#pragma once

/**
   Implement this function to copy data from @in to @out with io_uring.
   File descriptors @in and @out are guaranteed to be regular files.

   If a copy was successful, return 0. If an error occurred during a read
   or a write, return -errno.
*/
int copy(int in, int out);
