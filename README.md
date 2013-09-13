Pico Pixel Client SDK
=========

[Pico Pixel] is a texture viewer for graphics engineers. It lets you view many image format type such as png,
jpeg, DXT, HDR, KTX, OpenEXR and more.

This SDK provides a new way of viewing a program's raw images data. 

In C/C++, printing text to the console is quite easy:

```c
printf("Hello World!");
```

This SDK let you do the same but with raw image data. You no longer need to save your
images to the disk before you can view them. With this SDK, Pico Pixel will display your
images for you in a clean, fast and simple user interface designed with graphics engineers in mind.

In its simplest form, you would use the SDK like this:

```cpp
PixelPrintf(image_info, raw_image_data);
```

Pico Pixel has additional support to make it very easy to get raw image data out of your programs
with just a click.

Integration
-----------
The SDK is written in C++ and is made of 3 files:
  - PicoPixelClient.h           - Pico Pixel Client programing interface
  - PicoPixelClient.cpp         - Pico Pixel Client code you need to build with your program
  - PicoPixelClientProtocol.h   - Communication interface between Pico Pixel and your program

Using PixelPrintf
-----------------
Before you can use PixelPrintf calls to send image raw data to Pico Pixel you must first initialize the
client interface and open a network connection to Pico Pixel desktop application.

```cpp
PicoPixelClient pico_pixel_client("GlxGears");
pico_pixel_client.StartConnection();
```

After the connection to Pico Pixel desktop application is made, you cmay send raw image data like this:
```cpp
PixelPrintf("Test Image",                   // image name to appear in Pico Pixel desktop application
    PicoPixelClient::PIXEL_FORMAT_BGR8,     // image pixel data format
    400,                                    // image width
    300,                                    // image height
    1200,                                   // image row pitch in bytes
    FALSE,                                  // set to TRUE if the data is in srgb 
    FALSE,                                  // set to TRUE to rotate the image horizontally when displayed
    raw_data                                // char* pointer to the image raw data
    );
```

You can do more with PixelPrintf
--------------------------------
In the previous section, the call to PixelPrintf sends an image raw data to Pico Pixel desktop application
every time. For instance if you are calling PixelPrintf inside a loop, the image will be sent continously as
long as the code is iterating through the loop. This maybe undesirable and unnecessary.

Fortunately, Pico Pixel has a solution. You can declare makers in your code!

Markers are objects used by PixelPrintf to decide whether to send a pixel data to Pico Pixel or not.
A marker has an associated __*use_count*__ (positive value). As long as a marker's use_count is greater
than 0, any call to send data with that marker will be carried through. Each time the marker is used,
its __*use_count*__ is decremented by 1.

When a marker __*use_count*__ reaches zero, the marker can no longer be use to send data to Pico Pixel.
Its __*use_count*__ has to be reloaded before it can be used again. 

Reloading a marker is done through Pico Pixel destop application interface.

![alt tag](https://raw.github.com/username/projectname/branch/path/to/img.png)

Here is how you define a marker in your code:

```cpp
PicoPixelClient pico_pixel_client("GlxGears");

pico_pixel_client.StartConnection();

// the last parameter '0' means the marker use_count is 0.
int marker = pico_pixel_client.AddMarker(std::string("Color Buffer"), 0);

// Send the defined markers to PicoPixel desktop application
pico_pixel_client.SendMarkersToPicoPixel();
```

Here is how you use a marker with PixelPrintf:

```cpp

// The image is sent if and only if the marker's use_count is not 0.
// A marker's use_count is decremented each time the marker is used, until it reaches 0.
PixelPrintf(marker, "Test Image",           // image name to appear in Pico Pixel desktop application
    PicoPixelClient::PIXEL_FORMAT_BGR8,     // image pixel data format
    400,                                    // image width
    300,                                    // image height
    1200,                                   // image row pitch in bytes
    FALSE,                                  // set to TRUE if the data is in srgb 
    FALSE,                                  // set to TRUE to rotate the image horizontally when displayed
    raw_data                                // char* pointer to the image raw data
    );
```

The tech behind PixelPrintf
---------------------------
Pico Pixel Client SDK implements a network client interface to communicate with Pico Pixel desktop application.
Pico Pixel desktop application waits for client connection on port 2001.

In the example program comming with this SDK, sending image data is done from the program's main thread. However
you are free to call PixelPrintf from any thread in your program.

Pico Pixel client SDK is open source
------------------------------------
You can see everything that happens in a call to PixelPrintf. Your images are going directky to Pico Pixel desktop
application and no further!

You may modify the SDK to fit your development environment. And feel free to proposes patches and features you would
like to see in [Pico Pixel]


[Pico Pixel]: https://pixelandpolygon.com
