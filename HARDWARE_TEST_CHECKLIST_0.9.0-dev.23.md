# Hardware test checklist — 0.9.0-dev.23

- Upload a Preset asset large enough for the indicator delay: red arrow / blue tray / oscillating bar appears.
- Upload two or more Preset assets: indicator stays continuous between assets without showing partial Preset media.
- Send `06/02`: indicator disappears and first Preset item appears immediately.
- Verify GIF/image and TEXT Preset playback remains unchanged.
- Interrupt an upload and wait >5 s: indicator clears; an already active Preset resumes if one existed.
- Re-test Carousel upload indicator and ensure its behavior is unchanged.
