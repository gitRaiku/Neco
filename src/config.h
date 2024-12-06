#ifndef CONFIG_H
#define CONFIG_H

#define EMBED_GIF 0 // 0 = always read the gif from the specified path
                    // 1 = #embed the gif into the executable (C23)
#if EMBED_GIF
#define GIF_PATH "../neco.gif" // Change this path to set where neco searches
                            // for gifs to embed during compilation
#else
#define GIF_PATH "/usr/share/wl-neco/neco.gif" // Change this path to 
                                               // set where neco searches
                                               // for gifs on startup
#endif


uint32_t log_level = 2;

uint32_t screenw = 1920; // Screen width and height in px
uint32_t screenh = 1080;

float startx = -999.0f; // Starting x position (leave -999 for random)
float starty = -999.0f; // Starting y position (leave -999 for random)
float startxspeed = -999.0f; // Starting x speed (leave -999 for random)
float startyspeed = -999.0f; // Starting y speed (leave -999 for random)

float speedMultiplier = 7.0;

#endif
