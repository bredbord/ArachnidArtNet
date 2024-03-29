# ArachnidArtNet
Art-Net Version of the Arachnid System. Provides Art-Net control of up to 16 asynchronous stepper motors, along with a maximum of 8 Serial LED Strips (typically anything of the WS28XX$ varient) at variable refresh rates (tested 120Hz). Features a power-saving motion control algorithm (Cardinal) to move steppes to predefined positions followed by deactivation to save power. Steppers seamlessly resume their activity upon new DMX instructions. DMX footprint is linearly customizable for small or large installations.

This project relies on the AccelStepper, FastLED, and OctoWS2811 libraries.
