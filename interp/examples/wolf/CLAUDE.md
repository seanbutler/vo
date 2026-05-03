# Wolf — 2.5D text-mode raycaster in VO

## Context

Subfolder of `interp/`. Demonstrates floats, trig, OOP, loops, term.vo, logic.vo, cmath.vo.

**Run:** from `interp/` — `./build/vo game/main.vo`
**Quit:** press `q` in-game

Dependencies (all in interp/lib/):
- `cmath.vo`  - sin, cos, atan2, sqrt, floor, fabs
- `stdlib.vo` - null, clone, merge
- `logic.vo`  - boolean helpers
- `vtkkit.vo` - better cursor, color, input, drawing, etc
- `term.vo`   - also drawing cursor, colour, raw input


## Files to create

| File | Purpose |
|------|---------|
| `game/main.vo` | Entry point: init, game loop, shutdown |
| `game/map.vo` | Map data (array of strings) + wall-hit query |
| `game/player.vo` | Player hash: pos, angle, move, strafe, rotate |
| `game/render.vo` | Raycaster: DDA per column, draw frame |
| `game/config.vo` | Screen dims, FOV, move/rotate speed constants |

No other files. Do not create helpers outside these five.

## Screen layout

```
cols = 80   rows = 24
Wall columns fill rows 0..23.
Top half of each column = ceiling (dark).
Bottom half = floor (dark).
Middle strip = wall slice, height proportional to 1/dist.
```
screen cols and rows should be determined from vtkit.vo lib


ASCII wall shading by distance (nearest → farthest):
```
< 1.5  →  #
< 2.5  →  @  (bright colour)
< 4.0  →  +  (mid colour)
< 6.0  →  -  (dim colour)
>= 6.0 →  .  (darkest)
```
Ceiling char: space (black bg).  Floor char: `_` (dark).

## Raycasting algorithm (DDA)

For each screen column `x` (0 to COLS-1):

```
camera_x = 2.0 * x / COLS - 1.0          // -1.0 .. +1.0
ray_dx = dir_x + plane_x * camera_x
ray_dy = dir_y + plane_y * camera_y

// DDA step sizes
delta_x = fabs(1.0 / ray_dx)
delta_y = fabs(1.0 / ray_dy)

// initial step + side distances (from player to first grid line)
// march until wall hit
// distance = perpendicular (not Euclidean) to avoid fisheye
perp_dist = ...

wall_height = int(ROWS / perp_dist)
draw_start  = max(0, ROWS/2 - wall_height/2)
draw_end    = min(ROWS-1, ROWS/2 + wall_height/2)
```

Player direction vector `(dir_x, dir_y)` and camera plane `(plane_x, plane_y)`:
- Initial facing east: dir=(1,0), plane=(0, 0.66)
- Rotate left/right by delta_angle each frame using 2D rotation matrix

## Player controls (raw keycodes)

| Key | Code | Action |
|-----|------|--------|
| `w` | 119 | move forward |
| `s` | 115 | move backward |
| `a` | 97  | rotate left |
| `d` | 100 | rotate right |
| `q` | 113 | quit |

Movement speed: 0.05 units/frame. Rotation speed: 0.04 rad/frame.
Collision: do not step into a wall cell — check map before updating pos.

## Map format

Stored as a hash with an array of strings (one per row) and width/height.
`1` = wall, `0` = floor. Player starts at (2.0, 2.0) facing east.

```
MAP_DATA = {
    w : int = 16
    h : int = 16
    rows = {
        r0  : string = "1111111111111111"
        r1  : string = "1000000000000001"
        r2  : string = "1011100000001101"
        ...
        r15 : string = "1111111111111111"
    }
}
```

Wall query: `map_hit(map, x:int, y:int)` returns 1 if cell is wall, 0 if floor.

## Phases

Work through these in order. Complete and verify each before moving on.

### Phase 1 — Scaffold
- Create all five files with correct `@` imports and stub hashes/functions
- `main.vo` enters raw mode, clears screen, runs empty `~{}` loop, exits on `q`
- `term.restore_mode()` and `term.show_cursor()` called on exit
- **Verify:** runs, shows blank screen, `q` exits cleanly

### Phase 2 — Map + player state
- `map.vo`: MAP_DATA hash + `map_hit` function
- `player.vo`: player hash with pos_x, pos_y, dir_x, dir_y, plane_x, plane_y
- `player.vo`: `player_move`, `player_strafe`, `player_rotate` functions
- Input handling in game loop updates player each frame
- **Verify:** no crash, player state changes on keypress (add a debug printf if needed)

### Phase 3 — Renderer skeleton
- `render.vo`: `draw_frame(player, map)` stub that clears screen and draws a border
- Call `draw_frame` each iteration of the game loop
- **Verify:** border redraws each frame, no flicker issues

### Phase 4 — Raycaster
- Implement full DDA loop in `render.vo`
- For each column compute `perp_dist` and `wall_height`
- Draw ceiling, wall slice, floor characters for that column using `term.goto` + `printf_s`
- **Verify:** standing in the map shows walls. Moving forward/back changes wall height.

### Phase 5 — Shading + colour
- Apply distance-based shading chars and `term.color()` per column
- Ceiling = `term.color(BLACK)` space; floor = `term.color(BLACK)` `_`
- **Verify:** scene looks like a corridor. Distance shading visible.

### Phase 6 — Polish
- Add a one-line HUD at bottom: pos_x, pos_y, angle (use printf_s/printf_i)
- Ensure `term.goto(0,0)` at start of each frame (no scroll)
- Cap frame time if needed (`term.getch` already non-blocking, loop is fast enough)
- **Verify:** playable, no visual artefacts, quit leaves terminal clean

## Notes

- `cmath.vo` is an FFI lib — call e.g. `math.sin(angle)` after `@ "lib/cmath.vo"`
- All float literals need a decimal point: `1.0` not `1`
- `term.goto(col, row)` — col is x (0-based), row is y (0-based)
- Do not use `printf_i` / `printf_s` for anything that needs to appear at a specific
  screen position — always pair with `term.goto` first
- `int(float_val)` casts float to int in VO
- `vtkkit.vo` - prefer vtkit over term its better

code should be easy to read and structured using vo's features to create apropriate architecture and OO style, with minimal heirarchy but clear encapsulation.
