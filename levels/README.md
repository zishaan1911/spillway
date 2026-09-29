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
| `hold` | seconds | How long the goal must stay full, 1 by default |
| `emitter` | `x y dx dy speed total [width]` | A pipe. Flow rate follows from speed and width |
| `wall` | `x1 y1 x2 y2 [radius]` | A static capsule, radius 6 by default |
| `chain` | `radius x1 y1 x2 y2 ...` | Connected walls through the points |
| `goal` | `x y w h` | Region the water is counted in |
| `drain` | `x y w h` | Region that deletes water |
| `zone` | `x y w h ax ay` | Adds acceleration to water and balls inside |
| `ball` | `x y radius [density]` | A rigid disc; density is relative to water, 0.5 by default |
| `nodraw` | `x y w h` | Region the player cannot draw in |
| `fake` | `x1 y1 x2 y2 [radius]` | Looks exactly like a wall. Isn't one |
| `solution` | `x1 y1 x2 y2 ...` | A reference stroke; repeat for several strokes |

## Moving walls

Walls (and goals) can move. Open a group with a motion, list its walls and
goals, and close it with `end`:

```
slide 400 0  5  0.75        # dx dy period [phase]: out and back, eased
  chain 6   370 520   370 665   510 665   510 520
  goal      377 540   126 118
end

spin 660 420  75            # px py degrees-per-second
  wall 560 420  760 420  7
end

shift 220 0  0.8   690 60 120 660   # dx dy seconds, then a trigger box:
  chain 6  740 470  740 660  900 660  900 470   # moves once, when water
end                                             # first enters the box
```

Only `wall`, `chain` and `goal` go inside a group, and goals can ride on
slides and shifts but not spins. Movers are kinematic: they shove water and
balls around and nothing shoves back. They run on the world clock from the
moment the level loads, so when you open the taps matters.

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

1. Fill the goal and keep it full for the level's hold time.
2. Use no more than half the ink.
3. Fill it within par.

## Balancing

The shipped levels are tuned so the reference solution clears the target by
about 8 to 10 percent, after holding it for three seconds. Anything sloppier
than the reference tends to come up a few drops short, which is the point.
