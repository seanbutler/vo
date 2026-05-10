---
status: PENDING
priority: (MUST, SOONER, LOW, HIGH)
---

# SDL3 binding via C shim

Open a window, run a game loop, draw, handle input — all from VO. Avoid returning C structs where possible. Some handles (windows, textures) stay opaque behind getters/setters.

## Step 1 — FFI extensions (`interpreter.cpp`)

- Add `"pointer"` param/return type — stored as `int64_t`, cast to/from `void*`
- Add `"void"` return type — returns `Value::nil()`
- Extend `bind_foreign_function()` dispatch to handle `pointer` in any parameter position
- Empty hash `{}` passed as a `"pointer"` argument maps to `nullptr`

## Step 2 — C shim (`interp/sdl/vosdl.c`, compiled to `libvosdl.so`)

Structs fall into two categories:

**Opaque handles** (`SDL_Window*`, `SDL_Renderer*`, `SDL_Texture*`) — heap-managed by SDL. Shim provides thin create/destroy pairs; handles cross the boundary as `int64_t` pointers:

```c
void* vo_create_window(const char* t, int w, int h) {
    SDL_Init(SDL_INIT_VIDEO);
    return SDL_CreateWindow(t, w, h, 0);
}
void vo_destroy_window(void* win) { SDL_DestroyWindow(win); }
```

**Value structs** (`SDL_FRect`, `SDL_Color`, `SDL_Point`) — stack-allocated, no teardown. Shim takes individual scalars, builds the struct internally, calls SDL, discards it:

```c
void vo_fill_rect(void* ren, int x, int y, int w, int h) {
    SDL_FRect r = { x, y, w, h };
    SDL_RenderFillRect(ren, &r);
}
```

VO side uses a hash as the struct; wrapper callables unpack fields at the call site:

```vo
rect = { x:int=0  y:int=0  w:int=0  h:int=0
         ()=@(px:int py:int pw:int ph:int){ self.x:=px self.y:=py self.w:=pw self.h:=ph } }

fill_rect = @(ren r) { SDL.fill_rect(ren, r.x, r.y, r.w, r.h) }
```

Full shim API:
- `vo_create_window(title, w, h)` -> pointer — SDL_Init + SDL_CreateWindow
- `vo_create_renderer(win)` -> pointer — SDL_CreateRenderer(win, NULL)
- `vo_destroy_window(win)`, `vo_destroy_renderer(ren)` — cleanup
- `vo_pump()` — drains SDL_Event queue, updates internal state
- `vo_quit()` — 1 if quit event received
- `vo_key(scancode)` — 1 if key currently held
- `vo_mouse_x()`, `vo_mouse_y()` — current cursor position
- `vo_set_draw_color(ren, r, g, b, a)` — set render colour
- `vo_clear(ren)`, `vo_present(ren)` — frame rendering
- `vo_fill_rect(ren, x, y, w, h)` — draw filled rectangle
- `vo_delay(ms)` — frame timing

## Step 3 — VO descriptor (`interp/lib/vosdl.vo`)

- Descriptor hash + `bind_lib` call
- Exposes `SDL` hash with all shim functions bound

## Result — minimal VO game loop

```vo
# "lib/vosdl.vo"

win = SDL.create_window("hello", 800, 600, null)
ren = SDL.create_renderer(win, null)
~{
    SDL.pump()
    ? SDL.quit()    { \ }
    ? SDL.key(41)   { \ }    // escape
    SDL.draw_color(ren, 0, 0, 0, 255)
    SDL.clear(ren)
    SDL.present(ren)
    SDL.delay(16)
}
SDL.destroy_renderer(ren)
SDL.destroy_window(win)
```

## Deferred — needs struct marshalling to add later

- `SDL_Texture` / sprite rendering
- `SDL_Rect` for clipping
- Full keyboard event stream (not just current state)
- Audio (`SDL_audio`)
