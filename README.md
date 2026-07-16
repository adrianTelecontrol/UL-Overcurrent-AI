# UL Overcurrent v01

Embedded firmware project for a Texas Instruments TM4C1294NCPDT-based UL overcurrent tester platform.

The board uses external IS42S16320F SDRAM through the TM4C EPI peripheral and communicates with a gen4-FT812-50T display through QuadSPI. The UI stack combines a software-rendered RGB565 framebuffer with selective FT812/EVE hardware primitives for high-refresh elements such as graph traces.

## Development Environment

- Code Composer Studio 20.5.0
- Texas Instruments TM4C1294NCPDT
- TivaWare C Series
- FT812/EVE display controller

## Maintenance Note

This repository is expected to be reviewed, revised, and modified with assistance from OpenAI Codex. Codex-generated changes should be reviewed and validated on target hardware before production use.
