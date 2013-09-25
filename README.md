![alt tag](https://raw.github.com/inalogic/pico-pixel-client-sdk/master/Pictures/pico-pixel.png)
Pico Pixel Client SDK
=========

[Pico Pixel] is a texture viewer for software developers. It lets you view many image format types such as PNG,
JPEG, DXT, HDR, KTX, OpenEXR and more.

This SDK provides a new way of viewing program's raw image data. 

In C/C++, printing text to the console is quite easy:

```c
printf("Hello World!");
```

This SDK let you do the same but with raw image data. No need to save images to files first.
Just send them over to Pico Pixel and they will be displayed. Pico Pixel lets you explore the data
in your images in a fast, clean and simple environment. Additionally, Pico Pixel can save your images to files
if you want.

In its simplest form, you would use the SDK like this:

```cpp
PicoPixelClient pico_pixel_client("GlxGears");
pico_pixel_client.PixelPrintf(image_info, raw_image_data);
```

Pico Pixel has additional support to make it very easy to get raw image data out of your programs
with just a mouse click.

Integration
-----------
The SDK is written in C++ and is made of 3 files:
  - PicoPixelClient.h           - Pico Pixel Client programming interface
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

After the connection to Pico Pixel desktop application is made, you may send raw image data like this:
```cpp
PixelPrintf("color-framebuffer",            // image name to appear in Pico Pixel desktop application
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

Markers are objects used by PixelPrintf to trace the source of raw image data in your code and decide whether to send
the data to Pico Pixel or not.
A marker has an associated counter of integer values. While a counter value is greater
than 0, any call to send data with tha associated marker will be carried through. Each time a marker is used,
its counter value is decremented by 1.

When a marker's counter value reaches zero, the marker can no longer be use to send data to Pico Pixel.
Its counter value has to be reloaded before it can be used again. 

Reloading a marker's counter is done through Pico Pixel destop application interface.

Here is how you define a marker in your code:

```cpp
PicoPixelClient pico_pixel_client("GlxGears");

pico_pixel_client.StartConnection();

// the last parameter '0' means the marker use_count is 0.
int marker = pico_pixel_client.CreateMarker(std::string("Color Framebuffer"), 0);

// Send the defined markers to PicoPixel desktop application
pico_pixel_client.SendMarkersToPicoPixel();
```

Here is how you use a marker with PixelPrintf:

```cpp

// The image is sent if and only if the marker's use_count is not 0.
// A marker's use_count is decremented each time the marker is used, until it reaches 0.
pico_pixel_client.PixelPrintf(
    marker,                                 // data marker
    "color-framebuffer",                    // image name to appear in Pico Pixel desktop application
    PicoPixelClient::PIXEL_FORMAT_BGR8,     // image pixel data format
    400,                                    // image width
    300,                                    // image height
    1200,                                   // image row pitch in bytes
    FALSE,                                  // set to TRUE if the data is in srgb 
    FALSE,                                  // set to TRUE to rotate the image horizontally when displayed
    raw_data                                // char* pointer to the image raw data
    );
```

![alt tag](https://raw.github.com/inalogic/pico-pixel-client-sdk/master/Pictures/pico-pixel-client.png)

The tech behind PixelPrintf
---------------------------
Pico Pixel Client SDK implements a network client interface to communicate with Pico Pixel desktop application.
Pico Pixel desktop application waits for client connection on port 2001.

In the example program coming with this SDK, sending image data is done from the program's main thread. However
you are free to call PixelPrintf from any thread in your program.

Pico Pixel client SDK is open source
------------------------------------
You can see everything that happens in a call to PixelPrintf. Your images are going directly to Pico Pixel desktop
application and no further!

You may modify the SDK to fit your development environment. And feel free to propose patches and features you would
like to see in [Pico Pixel]


[Pico Pixel]: https://pixelandpolygon.com

