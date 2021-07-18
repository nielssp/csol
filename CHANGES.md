# csol changelog

## Unreleased

New features:

- [#4](https://github.com/nielssp/csol/issues/4): Smart cursor movement (`smart_cursor 1` in `csolrc`)
- [#5](https://github.com/nielssp/csol/issues/5): Manpage added

## csol 1.4.1

Fixed issues:

- Fix initial cursor position.
- [#3](https://github.com/nielssp/csol/issues/3): Fix busy wait after victory animation


## csol 1.4.0

New features:

- Games added:
  - `spider1`/`spider2`/`spider4`: Spider with 1 suit, 2 suits, and 4 suits.
  - `golf`: Golf
- Themes added:
  - `corners`
- Asks before resetting or quitting game, but only if no moves have been made.
- It's now possible to load games with long file names in DOS.
- Adds stats and scores CSV files to keep track of wins/losses and playtime across games.
- Adds automatic conversion from UTF8 to CP437 so that the default Unicode-based themes can be used in DOS and Windows.


## csol 1.3.0

New features:

- Games added:
  - `klondikefc`: Klondike FreeCell
  - `yukonfc`: Yukon FreeCell
- Themes added:
  - `compact`
  - `compact-ascii`
  - `ultracompact`
- Adds <kbd>s</kbd> and <kbd>w</kbd> commands to make it easier to move cards from stock and waste.
- Adds support for the 16 named default colors in themes, e.g. `red`, `blue`, `bright_green`, etc.
- Merges changes from [csol-dos](https://github.com/nielssp/csol-dos) so that csol can now be compiled for DOS and WIN32.

## csol 1.2.0

New features:

- Allow using free cells to move several cards at once in Freecell and Eight Off.
- It is now possible to use the number keys to quickly move cards from cells in Freecell and Eight Off.
- Ask before resetting game.
- Add silly victory animation.

## csol 1.1.0

New features:

- Win condition added in the form of the property win_rank on game rules. It is set to king by default on all foundations which means that the game has been won when each foundation has a king as its top card.

Fixed issues:

- [#1](https://github.com/nielssp/csol/issues/1): Removes the redeal limit from klondike, since most common implementations don't include it.

## csol 1.0.0

This is the first stable release of csol.

The following games are included:

- klondike
- yukon
- russian
- freecell
- eightoff

The following themes are included:

- default
- default-xl
- ascii
