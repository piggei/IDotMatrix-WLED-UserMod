# iDotMatrix WLED Usermod 0.9.0-rc.3

RC3 is a focused behavioral correction discovered during comparison with the original iDotMatrix device.

## TEXT multi-row pagination

For TEXT effects that do **not** scroll, the renderer now uses the complete logical matrix height and lays glyphs out row-by-row:

- 64x64: one row for 64px glyphs, two rows for 32px glyphs, four rows for 16px glyphs;
- 32x32: one row for 32px glyphs, two rows for 16px glyphs;
- 16x16: one 16px row.

The block of rows actually used by a page is vertically centered, so a short page is not pinned to the top. Horizontal and vertical scrolling effects remain single-line/tape presentations and are unchanged.

No BLE framing, media transport, Preset/Carousel, clock, timer, audio or image behavior changes in this RC.
