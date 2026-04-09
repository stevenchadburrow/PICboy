# PICboy
A fast Gameboy (Color) Emulator designed for use in Microcontrollers<br>

This emulator is completely hand-made without any previous code used, though it was inspired by <a href="https://github.com/deltabeard/Peanut-GB">Peanut-GB</a> and used it for reference occasionally.<br>

The principle is to keep this emulator in a single C file and have it as portable as possible to smaller devices.  It is also designed for speed over accuracy.  It uses OpenGL/GLFW and OpenAL as a basis for graphics and audio on desktop computers.  It was developed on Linux but should be portable to other systems with minimum or no effort at all.<br>

This is a full DMG/GBC emulator with audio included, for No-MBC, MBC1, MBC2, MBC3, and MBC5 carts.  Nearly all GBC additions are marked somehow.  Most GBC games work, some do not, specifically Shantae (which I'm ok with!)  The audio sounds great on microcontroller hardware, but this implementation using OpenAL makes it sound pretty bad.  Overall, it runs Crystal Clear just fine, so good enough!<br>

DMG games run at 60 FPS on a PIC32MZ running at 260 MHz, as do some GBC/DX games.  Other GBC games are slow, but reducing the framerate to 20 FPS makes them run at full speed.<br>

<b>Features:</b><br>
- Cart RAM Save/Load<br>
- Turbo A/B Buttons<br>
- Fast-Forward and Freeze<br>
- Adjustable Color Palettes<br>
- Force GBC or DMG modes<br>

<b>Resources:</b><br>
- <a href="https://gbdev.io/pandocs/About.html">https://gbdev.io/pandocs/About.html</a>
- <a href="https://gbdev.io/gb-opcodes/optables/">https://gbdev.io/gb-opcodes/optables/</a>
- <a href="https://gbdev.gg8.se/wiki/articles/Main_Page">https://gbdev.gg8.se/wiki/articles/Main_Page</a>
- <a href="https://gekkio.fi/files/gb-docs/gbctr.pdf">https://gekkio.fi/files/gb-docs/gbctr.pdf</a>
- <a href="https://mgba-emu.github.io/gbdoc/">https://mgba-emu.github.io/gbdoc/</a>
- <a href="https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM">https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM</a>
- <a href="https://github.com/catskull/gb-rom-database">https://github.com/catskull/gb-rom-database</a>

<b>Images:</b><br>
<table>
  <tr><td><table><tr><td><img src="Images/Tetris.png"></td></tr><tr><td align="center"><b>Tetris</b></td></tr></table></td>
    <td><table><tr><td><img src="Images/F1Race.png"></td></tr><tr><td align="center"><b>F-1 Race</b></td></tr></table></td></tr>
  <tr><td><table><tr><td><img src="Images/ExpPak.png"></td></tr><tr><td align="center"><b>Kanto Expansion Pak</b></td></tr></table></td>
    <td><table><tr><td><img src="Images/BalloonKid.png"></td></tr><tr><td align="center"><b>Balloon Kid</b></td></tr></table></td></tr>
  <tr><td><table><tr><td><img src="Images/ZeldaDX.png"></td></tr><tr><td align="center"><b>Link's Awakening DX</b></td></tr></table></td>
    <td><table><tr><td><img src="Images/TobuDX.png"></td></tr><tr><td align="center"><b>Tobu Tobu Girl DX</b></td></tr></table></td></tr>
   <tr><td><table><tr><td><img src="Images/OracleOfSeasons.png"></td></tr><tr><td align="center"><b>Oracle of Seasons</b></td></tr></table></td>
    <td><table><tr><td><img src="Images/MarioDeluxe.png"></td></tr><tr><td align="center"><b>Super Mario Deluxe</b></td></tr></table></td></tr>
  <tr><td><table><tr><td><img src="Images/CrystalClear1.png"></td></tr><tr><td align="center"><b>Crystal Clear World</b></td></tr></table></td>
    <td><table><tr><td><img src="Images/CrystalClear2.png"></td></tr><tr><td align="center"><b>Crystal Clear Battle</b></td></tr></table></td></tr>
</table>
