# GBA RTCRead

This tool can read/write data in the RTC found inside carts of some GBA games (Pokemon Ruby/Sapphire/Emerald and Boktai games).

The original tool and details about the RTC chip was found here: [http://www.furlocks-forest.net/wiki/?page=Pokemon_Ruby/Sapphire_New_Battery_Glitch](https://web.archive.org/web/20180413154901/http://www.furlocks-forest.net/wiki/?page=Pokemon_Ruby/Sapphire_New_Battery_Glitch)

The DS port is still available here: [https://sourceforge.net/projects/rtcread-ds/](https://sourceforge.net/projects/rtcread-ds/)


## Features

- Enables read current date and time from RTC data
- Enables write new date and time to the RTC data
- The GBA version must be written to a GBA flashcart, and works with cartridge swap
- The GBA version can also edit the RTC found in the Everdrive GBA cartridge
- The DS version is a homebrew for R4 carts that access the Slot-2 carts

## Known Issues

- Both the GBA and DS original releas versions have a rollover bug in February of a leap year, where it rollsover at Feb 09 instead of Feb 29.
