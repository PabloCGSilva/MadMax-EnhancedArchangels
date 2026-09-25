# Mad Max – Enhanced Archangels (v0.9.1-beta2)

Four new Archangels, a bonus for every Archangel, and fixes for the screens
that never expected more than sixteen.

**This is a beta.** Back up your saves before trying it.

## What's new in beta2

* **Steam fix.** On Steam the previous version found none of the game functions it needs: the Steam executable keeps its code encrypted until the game starts (Steam's DRM), and the mod was looking too early. It now waits for the game to start before looking. Tested by simulating that startup on the GOG version; Steam players, please send `scripts\EnhancedArchangels.log` if anything goes wrong.

## The new Archangels

| Archangel | Body | Paint | Build | Bonus |
|---|---|---|---|---|
| **Crusader of Torments** | Ripper | Black Tar | max ramming and armor, spikes, grinders | Ramming |
| **Skull Collector** | Ripper | Primer (red) | Ultimate Big Chief V8, President exhaust, spikes | Engine |
| **Noble Vagrant** | Ripper | Gold Tar | Slick Rubs, Big Whammy lift, small engine | Traction |
| **Penitent Stalker** | Wild Hunt | Camouflage | Big Chief V8, Big Mama lift, the least used parts in the game | Traction |

They sit at the end of the Archangels list, with their own names,
descriptions and pictures, in the garage, in Collectibles and in the
stronghold's *Choose Vehicle* screen. Like the originals, each one is linked
to a Death Run (Tricky Pass, Mortal Bite, Heat Haze and Even Rip) and is
built from parts you own.

The Ripper body comes from the game's *The Ripper* content; Crusader of
Torments, Skull Collector and Noble Vagrant need it.

## A bonus for every Archangel

Each of the 20 Archangels gives the bonus of **one hood ornament**, picked
automatically from its build:

| Group | Effect | Archangels |
|---|---|---|
| **Engine** | +2.5% top speed, +1% acceleration | Speed Demon, Sanguine Guardian, Argent Cavalier, Cardinal Grinder, Speed Freak, Skull Collector |
| **Traction** | more tire grip | Aurelian the Ready, Rule of War, Lord Gravel, Noble Vagrant, Penitent Stalker |
| **Ramming** | harder hits when ramming | Kill Box, Radiant Shadow, Jugger of Virtue, Crusader of Torments |
| **Armor** | +20% fire resistance, boarders shaken off more easily | Soul Sweeper, Righteous Spike |
| **Weapons** | harpoon reloads 10% faster, side burners use 20% less fuel | The Jack, Pinky Finger, Celestial Bones |

* It is the game's own ornament effect, so it shows as a **+** on the stat.
* It only counts while the Archangel is installed as it is. Change a part and
  it stops being that Archangel, so the bonus goes away.
* Both ornament slots stay free. A real ornament of the same group stacks on
  top: **++**, and a third one shows as a single **red +**.
* While you browse the Archangels screen, the **+** moves to the stat of the
  Archangel you are looking at.

## Fixes

* **More than 16 Archangels.** The garage screen had room for exactly sixteen
  and crashed with more; Collectibles and *Choose Vehicle* only showed sixteen,
  and picking a 17th in *Choose Vehicle* left the screen black. All fixed (the
  garage now takes up to 64).
* **Handling bar running past its frame.** A bug of the original game: when
  your car's Handling is below zero and the Archangel you preview has it above
  zero, the green part of the bar was drawn too long.

## Installation

1. Copy everything in this package into your Mad Max folder (the one with
   `AVAMain.exe`), merging the `scripts` and `dropzone` folders.
2. `dinput8.dll` is the Ultimate ASI Loader. If you already have it from
   another mod, keep yours.
3. Start the game. `scripts\EnhancedArchangels.log` shows what the mod found.

To uninstall, delete `scripts\EnhancedArchangels.asi` and the files this
package added under `dropzone`. A car built from one of the new Archangels
keeps its parts.

## Requirements and compatibility

* Made and tested on the **GOG** version. Steam should work since beta2 but
  hasn't been confirmed yet: if the mod can't find what it needs in your
  executable it disables itself and tells you so, and the game stays safe to
  play.
* *The Ripper* content, for three of the new Archangels.
* It replaces, through the dropzone, the Archangel table
  (`vehicles/archetypes.xlsc`), the garage and Collectibles screens
  (`gui/npc_menu_upgrades2.guixc`, `gui/ingame_collectibles2.guixc`) and the
  interface image list (`gui/texturelist.guistreamertexturelistc`). Another
  mod that changes the same files won't work together with this one.
* Works alongside Enhanced Convoys and Wasteland Storms.

## Known limitations

* The pictures of the new Archangels are recolors of an existing render (the
  Wild Hunt body), not renders of the cars themselves.

## Shout outs

* **Rick Gibbed** for the Gibbed Mad Max tools, and **gigaHours** for the
  fork with the XVM script assembler and disassembler this mod was built with.
* **Tsuda Kageyu** for MinHook, **ThirteenAG** for the Ultimate ASI Loader.
* **Avalanche Studios** for Mad Max.
