# csol

A small collection of solitaire games implemented in C using ncurses.

![csol](csol.png)

## Usage

Compile and run:
```
cmake .
make
./csol
```

## Games

Klondike (default): `csol klondike`

![Klondike](games/klondike.png)

Yukon: `csol yukon`

![Yukon](games/yukon.png)

Eight Off: `csol eightoff`

![Eight Off](games/eightoff.png)

Freecell: `csol freecell`

![Freecell](games/freecell.png)

Russian Solitaire: `csol russian`

![Russian Solitaire](games/russian.png)

## Themes

`csol -t default`

`csol -t default-xl`

![default-xl](themes/default-xl.png)

`csol -t ascii`

![ascii](themes/ascii.png)

## Options

* `--version`/`-v`: Show version
* `--help`/`-h`: Show help
* `--list`/`-l`: List games
* `--themes`/`-T`: List themes
* `--theme <name>`/`-t <name>`: Use a theme
* `--mono`/`-m`: Disable colors
* `--seed <seed>`/`-s <seed>`: Seed the pnrg
* `--config <file>`/`-c <file>`: Use a configuration file

## Configuration

The system-wide configuration file is `/etc/xdg/csol/csolrc` with games in `/etc/xdg/csol/games` and themes in `/etc/xdg/csol/themes`.

## Keys

Move the cursor using <kbd>H</kbd>, <kbd>J</kbd>, <kbd>K</kbd>, and <kbd>L</kbd> or the arrow keys.

Select the card under the cursor using <kbd>SPACE</kbd>.

Move the selected card to the tableau or foundaton under the cursor using <kbd>M</kbd>.

Press <kbd>A</kbd> to automatically move a card (from any tableau) to a foundation if possible.

Press <kbd>Q</kbd> to quit.
