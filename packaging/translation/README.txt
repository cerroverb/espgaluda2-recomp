English translation patch for espgaluda2-recomp

Where the English comes from
The menus, messages and tutorials use the official English text of the game's Nintendo Switch release
(Live Wire / CAVE, 2021). The Xbox 360 game was released only in Japanese, so this text is matched to the Xbox 360
game's Japanese lines and shown in their place.

The patch is optional. Turn off "translation_patch" in the settings window (F4, then "Save to config"), or start
the game with --translation_patch=false, and the next launch shows the original Japanese.

What was changed and why
- Corrected lines: a few official lines have obvious errors (for example the main menu's Score Attack help said
  "Compete for high scores against your friends for high scores."). Every correction is listed in CHANGES.txt
  beside this file, with the original line.
- Adapted lines: some Xbox 360 messages do not exist on the Switch (Xbox LIVE, storage devices, the Xbox Guide
  button, Achievements). These use the closest official Switch lines with the Xbox 360 terms put back. Every
  adapted line is listed in CHANGES.txt, with the Switch lines it was made from.
- Lines are wrapped differently where the English did not fit the Xbox 360 game's text boxes.

- The tutorial's controller diagram labels its buttons in Japanese art (the Switch release left them in Japanese
  too). With the patch on they are redrawn, as the game loads that picture, in the game's own English letters with
  the official Switch names of those buttons: Shot, Ascension, Guard Barrier, Rapid-fire Shot.

No game files are included. Other pictures that contain Japanese text (parts of the game's art) are not changed.
