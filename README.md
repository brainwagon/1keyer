# 1keyer
An iambic paddle controller for sending Morse code, implemented for hackaday.io's 1K code contest
This is a simple iambic keyboard controller that allows you to use an iambic key or side swiper style key to quickly send properly timed Morse. It also serves as a serial interface: you can transmit ASCII from microcontroller and controller will queue it up and send it at the appropriate time. 

The target is a cheap Arduino micro module, running at 8Mhz and costing less than three bucks. The code is simple and written in C. 
