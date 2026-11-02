# Content-driven settings layout (after rc.6)

This source change replaces rc.6's window-width bands with layout rules. It is
not part of the immutable rc.6 package and does not modify monitor configuration.

## Rules

- The compositor and Qt control the outer window. The application no longer
  imposes a fixed minimum window size or computes an initial size from one screen.
- Navigation compares the natural sidebar and page-content widths with the space
  actually available. When both cannot fit, a compact page selector replaces the
  sidebar. Both views share one model and preserve the selected page and shortcuts.
- `AdaptiveRow` measures current child size hints, including fonts, text and
  controls, to choose horizontal or vertical arrangement. Font/style and layout
  changes trigger recalculation even if the window size stays unchanged.
- Width preferences remain independent of the current arrangement, preventing
  the parent and child from repeatedly changing their layout decisions. Wrapped
  row heights are published back to the parent so wrapped text is not clipped.
- `FlowLayout` wraps complete key controls into additional lines. Individual
  controls keep their intrinsic minimum sizes; page scroll areas provide access
  if even an individual control cannot fit. Arbitrarily tiny windows cannot make
  every control simultaneously visible.
- Key labels derive size hints from the current font and padding. There are no
  device-resolution lists, per-monitor scale multipliers or sidebar width bands.

## Verification approach

Tests assert invariants rather than treating a list of devices as the layout
algorithm: controls do not overlap; changing fonts without resizing changes the
arrangement; wrapped labels have enough height; page selection survives navigation
changes; and overflow has a scroll range. The actual-window harness sweeps widths
in 73-pixel increments, two heights and four scale factors with isolated settings.
These samples exercise the rules; they are not a whitelist of supported devices.

Local verification passed all 13 CTest tests with Qt 6.11.2, including the actual
window sweep of 16 widths, two heights, four scale factors and all eight pages.
Final narrow and wide screenshots were inspected after correcting wrapped-row
height propagation. The tests use isolated XDG directories and synthetic data.

Offscreen testing and synthetic screenshots do not prove mixed-DPI monitor
movement, compositor-specific rendering or real-device interoperability. No
Hyprland monitor settings or installed application files are changed by these
source tests.
