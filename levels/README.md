# Level format

Levels are plain text, one command per line, loaded in filename order. `#`
starts a comment. Coordinates are in a 1280 x 720 world with y pointing down,
so they match pixels at the default window size. Keep things below y = 50;
the HUD sits there.

| Command | Arguments | Meaning |
|---|---|---|
| `name` | text | Title shown in the menu and HUD |
| `hint` | text | One line shown before the taps open |
| `ink` | length | How much wall the player may draw |
| `target` | count | Particles that must sit in the goal |
| `par` | seconds | Finish within this for the time star |
| `emitter` | `x y dx dy speed total [width]` | A pipe. Flow rate follows from speed and width |
| `wall` | `x1 y1 x2 y2 [radius]` | A static capsule, radius 6 by default |
| `chain` | `radius x1 y1 x2 y2 ...` | Connected walls through the points |
| `goal` | `x y w h` | Region the water is counted in |
| `drain` | `x y w h` | Region that deletes water |
| `zone` | `x y w h ax ay` | Adds acceleration to water and balls inside |
| `ball` | `x y radius [density]` | A rigid disc; density is relative to water, 0.5 by default |
| `nodraw` | `x y w h` | Region the player cannot draw in |
| `solution` | `x1 y1 x2 y2 ...` | A reference stroke; repeat for several strokes |

## Solutions

Every shipped level carries its own `solution` lines. The test suite plays
each level twice, once with nothing drawn (it must not be winnable) and once
with the solution drawn (it must be won), so a physics change that breaks a
level fails CI instead of shipping.

To see a solution in action:

```
spillway --capture 4 5 updraft.png solved
```

## Stars

1. Fill the goal and keep it full for one second.
2. Use no more than half the ink.
3. Fill it within par.
