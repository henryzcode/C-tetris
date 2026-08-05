# C-Tetris

A classic Tetris clone written in C++ using the SDL2 library, created by Henry.

## Features

* **Classic Gameplay**: Features the standard 7 Tetrominoes (I, J, L, O, S, T, Z) with randomized colors.


* **Modern Mechanics**: Includes a ghost piece for accurate dropping, a next piece preview, and wall-kick (SRS) rotation handling.


* **Audio System**: Background music playlist with automatic shuffling and cross-fading, plus sound effects for UI interactions.


* **Scoring System**: Points are awarded for line clears, scaling with multiple lines cleared at once and the current level.


* **Interactive UI**: A fully functional pause menu with resume and quit options.



---

## Controls

* **Left / Right Arrows**: Move the active piece.


* **Up Arrow**: Rotate the piece.


* **Down Arrow**: Soft drop (speeds up falling).


* **Spacebar**: Hard drop (instantly drops the piece to the bottom).


* **Escape**: Pause the game.


* **F1 / F2**: Decrease or increase music volume.


* **Delete**: Exit the game.



---

## Dependencies

To compile and run this project, you will need a compiler that supports C++20 (for `std::ranges::shuffle`) and the following libraries installed on your system:

* SDL2


* SDL2_image


* SDL2_mixer


* SDL2_ttf



## Assets Structure

Ensure your root directory contains the following asset folders for the game to load correctly:

* `/assets/tiles/` (contains individual block colors like `blue.png`, `red.png`, etc.)


* `/assets/music/` (contains background tracks)


* `/assets/sound/` (contains UI sound effects like `clicked.wav`)


* `/assets/pixel.ttf` (font file for rendering text)


* UI images (e.g., `button.png`, `menu.png`, `quit.png` and their hovered states)

