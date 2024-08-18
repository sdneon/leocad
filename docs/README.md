# About LeoCAD _Mod_

LeoCAD is a CAD program for creating virtual LEGO(R) models. It's available
for free under the GNU Public License v2 and works on the Windows, Linux
and macOS Operating Systems.

This repo is my **mod** adding these _**ease of use enhancements**_:
* Add keyboard shortcuts (**mnemonics**) for menu items. E.g.:
  * In Timeline: insert steps before, after; move selection here...
  * Various commands in main menu.
* **Easier parts search** via Search Parts textfield.
  * Specify brick sizes without spaces. E.g. key in just 2x2 instead of "2 x 2".
  * If exact phrase (search text) does not match, also try matching all keywords in search text. So you don't have to know exact phrasing of the part.
    * E.g. to find "Brick 5 x 5 Corner Round with Slope 45 Chamfer", you don't need to remember and key in "Corner Round with Slope" exactly. Just key in "slope corner" or any of the keywords (even partial keywords) to search for.
* Alternative to `Edit> Replace` command: New command `Piece>Replace` which replaces selected bricks with the one in Parts panel.
  * Hotkey: CTRL+R; mnemonic: R.
* Open .LXF - as an import. So you can associate .LXF files with LeoCAD, to open them easily.

Enjoy ",

# Installation

You can download the latest version of LeoCAD and its Parts Library from
the main website at https://www.leocad.org

It's recommended that you install the latest drivers for your video card
since LeoCAD uses OpenGL to take advantage of hardware acceleration for
rendering.

## LeoCAD for Windows

  Download the latest LeoCAD-Windows.exe to your computer, double click on
  the icon to launch the installer and follow the instructions.

## LeoCAD for Linux

There are multiple ways to install LeoCAD on Linux.

* Snap Store
  
  You can find LeoCAD in your distribution's Snap Store. Alternatively, you
  can enter `sudo snap install leocad` to install it using the command line.

* AppImage

  Download the latest LeoCAD-Linux.AppImage, make the file executable
  (`chmod +x`) and run it.

* Flatpak

  You can also install LeoCAD releases as a Flatpak from Flathub:
    https://flathub.org/apps/details/org.leocad.LeoCAD

  Note: there might be a delay for new releases to appear there. If
  you have it already installed, it will be updated.

* From source

  If you prefer to compile LeoCAD yourself, go to the GitHub releases page
  at https://github.com/leozide/leocad/releases/latest and download the
  source archive from there. If you do not already have a Parts Library
  installed, you will need to download one and follow the installation
  instructions. More information on how to compile your own executable is
  available in the Documentation section of https://www.leocad.org

## LeoCAD for macOS

  Download the latest LeoCAD-macOS.dmg to your computer, double click on
  the icon to open the archive, copy LeoCAD.app to your Applications folder
  and then launch it from there.

New users should read the online tutorial located at
https://www.leocad.org/docs/tutorial1.html to learn how to use LeoCAD.


# Commandline Options
Usage: `leocad [options] [file]`
`[options]` can be:
* -l, --libpath <path>: Set the Parts Library location to path.
* -i, --image <outfile.ext>: Save a picture in the format specified by ext and exit.
* -w, --width <width>: Set the picture width.
* -h, --height <height>: Set the picture height.
* -f, --from <step>: Set the first step to save pictures.
* -t, --to <step>: Set the last step to save pictures.
* -s, --submodel <submodel>: Set the active submodel.
* -c, --camera <camera>: Set the active camera.
* -ss, --stud-style <id>: Set the stud style 0=No style, 1=LDraw single wire, 2=LDraw double wire, 3=LDraw raised floating, 4=LDraw raised rounded, 5=LDraw subtle rounded, 6=LEGO no logo, 7=LEGO single wire.
* --viewpoint <front|back|left|right|top|bottom|home>: Set the viewpoint.
* --camera-angles <latitude> <longitude>: Set the camera angles in degrees around the model.
* --camera-position <x> <y> <z> <tx> <ty> <tz> <ux> <uy> <uz>: Set the camera position, target and up vector.
* --camera-position-ldraw <x> <y> <z> <tx> <ty> <tz> <ux> <uy> <uz>: Set the camera position, target and up vector using LDraw coordinates.
* --orthographic: Render images using an orthographic projection.
* --fov <degrees>: Set the vertical field of view used to render images (< 180).
* --zplanes <near> <far>: Set the near and far clipping planes used to render images (1 <= <near> < <far>).
* --fade-steps: Render parts from prior steps faded.
* --no-fade-steps: Do not render parts from prior steps faded.
* --fade-steps-color <rgba>: Renderinng color for prior step parts (#AARRGGBB).
* --highlight: Highlight parts in the steps they appear.
* --no-highlight: Do not highlight parts in the steps they appear.
* --highlight-color: Renderinng color for highlighted parts (#AARRGGBB).
* --shading <wireframe|flat|default|full>: Select shading mode for rendering.
* --line-width <width>: Set the width of the edge lines.
* --aa-samples <count>: AntiAliasing sample size (1, 2, 4, or 8).
* -nscc, --disable-stud-cylinder-color: Disable high contrast stud cylinder color.
* -scc, --stud-cylinder-color <#AARRGGBB>: High contrast stud cylinder color.
* -nec, --disable-edge-color: Disable high contrast edge color.
* -ec, --edge-color <#AARRGGBB>: High contrast edge color.
* -nbec, --disable-black-edge-color: Disable high contrast edge color for black parts.
* -bec, --black-edge-color <#AARRGGBB>: High contrast edge color for black parts.
* -ndec, --disable-dark-edge-color: Disable high contrast edge color for dark color parts.
* -dec, --dark-edge-color <#AARRGGBB>: High contrast edge color for dark color parts.
* -aec, --automate-edge-color: Enable automatically adjusted edge colors.
* -cc, --color-contrast <float>: Color contrast value between 0.0 and 1.0.
* -ldv, --light-dark-value <float>: Light/Dark color value between 0.0 and 1.0.
* -obj, --export-wavefront <outfile.obj>: Export the model to Wavefront OBJ format.
* -3ds, --export-3ds <outfile.3ds>: Export the model to 3D Studio 3DS format.
* -dae, --export-collada <outfile.dae>: Export the model to COLLADA DAE format.
* -csv, --export-csv <outfile.csv>: Export the list of parts used in csv format.
* -html, --export-html <folder>: Create an HTML page for the model.
* -v, --version: Output version information and exit.
* -?, --help: Display this help message and exit.

`[file]` can be:
* .LDR
* .LXF - will be auto-imported

# Online Resources

- Website:
  https://www.leocad.org

- GitHub page:
  https://github.com/leozide/leocad

- Unstable builds:
  https://github.com/leozide/leocad/releases/tag/continuous


# Legal Disclaimer

LEGO(R) is a trademark of the LEGO Group of companies which does not sponsor,
authorize or endorse this software.
