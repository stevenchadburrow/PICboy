# PICboy
A Gameboy (Color) Emulator designed for use in Microcontrollers<br>

This emulator is completely hand-made with copies of code, though it was inspired by <a href="https://github.com/deltabeard/Peanut-GB">Peanut-GB</a> and used it for reference occasionally.<br>

The principle is to keep this emulator in a single C file and have it as portable as possible to smaller devices.  It is also designed for speed over accuracy.  It uses OpenGL/GLFW and OpenAL as a basis for graphics and audio on desktop computers.  It was developed on Linux but should be portable to other systems with minimum or any effort at all.<br>

As of now, it is a full DMG emulator with audio included, for No-MBC, MBC1, MBC3, and MBC5 carts.  Future development will focus on GBC additions.<br>

The best source for information is <a href="https://gbdev.io/pandocs/About.html">PanDocs</a> though I looked at some others for help once in a while.<br>

<b>Images (before DAA was coded):</b><br>
<table>
  <tr><td><img src="Tetris1.png"></td><td><img src="Tetris2.png"></td></tr>
  <tr><td><img src="Alleyway1.png"></td><td><img src="Alleyway2.png"></td></tr>
  <tr><td><img src="DrMario1.png"></td><td><img src="DrMario2.png"></td></tr>
</table>
