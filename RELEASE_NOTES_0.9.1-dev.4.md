# iDotMatrix WLED UserMod 0.9.1-dev.4

## Clock 64x64 correction

This build corrects the spacing change introduced in 0.9.1-dev.3.

For native 64x64 Clock styles 0 and 3:

- the HH:MM row is restored exactly to its previous layout;
- the time separator is not moved;
- the minute digits are not moved;
- the date day field remains fixed;
- the date `/` separator moves two physical LEDs to the right;
- both month digits move two physical LEDs to the right.

The 16x16 and 32x32 layouts and all other Clock styles are unchanged.

No other runtime behavior is intentionally changed.
