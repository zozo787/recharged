# iPhone and iPad

Recharged runs natively on iPhone and iPad: no emulator and no jailbreak. It
is not on the App Store. You install the `.ipa` file from the release page
yourself, signed with your own Apple Account, then play with the touch screen
or a Bluetooth controller.

**Quick version** for people who have sideloaded apps before:

1. Download `recharged-<version>-ios-arm64.ipa` from the
   [latest release](https://github.com/previouslevelgames/recharged/releases/latest).
2. Install it with SideStore, AltStore, Sideloadly or Xcode.
3. Start Recharged. On the **Game** page, tap **Choose file...** and pick your
   disc image. Then press **Play**.

The steps in detail:

## 1. What you need

- An iPhone or iPad with **iOS or iPadOS 18** or newer.
- Your own **Mario Strikers Charged disc image** (`.iso`, `.rvz` or `.wbfs`),
  as on the other systems ([install guide](INSTALL.md)). Recharged copies it
  into its own folder, so the device needs that much free space (1 to 2 GB).
- An **Apple Account** to sign the app. A free one works.
- A sideloading tool: [SideStore](https://sidestore.io),
  [AltStore](https://altstore.io), [Sideloadly](https://sideloadly.io) or
  Xcode on a Mac.

## 2. Download

On the [latest release](https://github.com/previouslevelgames/recharged/releases/latest)
page, download the file ending in **`ios-arm64.ipa`**. The same file works
on iPhone and iPad. `SHA256SUMS` lists its checksum.

## 3. Install

The `.ipa` is not signed, and iOS only installs signed apps. Your sideloading
tool signs it with your Apple Account and installs it; follow that tool's
own guide. Then:

- **Developer Mode**: iOS asks for it the first time
  (*Settings → Privacy & Security → Developer Mode*, then restart).
- **Trust the developer**: with Sideloadly or Xcode, iOS may refuse to open
  the app until you trust your Apple Account under
  *Settings → General → VPN & Device Management*.
- **Free Apple Account**: the app stops opening after **7 days** until it is
  signed again. SideStore and AltStore refresh it for you; with the other
  tools, install it again. A free account signs up to 3 apps at a time.

## 4. First start: add your game

The launcher opens with a short welcome guide (the light bulb at the bottom
opens it again). On the **Game** page, tap **Choose file...** and pick your
disc image wherever it is: the Files app, iCloud Drive or a USB drive.
Recharged copies it into its own folder; the button says **Importing...**
until the copy is done.

You can also copy the image yourself in the Files app, into
*On My iPhone (or iPad) → Recharged → game*: the launcher lists it there.

Then press **Play**.

## 5. Playing

- **Touch screen**: player 1 plays with on-screen controls by default: a
  stick, the Wii Remote's and Nunchuk's buttons (**Hit** shakes the Remote,
  **Swap** shakes the Nunchuk), **−**, **+**, **1** (pause), **2** and the
  D-pad. A finger anywhere else on the screen points the Remote.
- **Your own layout**: on the **Controls** page, the sliders button beside
  *Touch screen* opens the controls over the whole screen. Drag a control to
  move it, tap it and use **−** and **+** to size it, and set the opacity and
  *Vibrate on press*. **Save** keeps the layout.
- **Bluetooth controllers** (Xbox, PlayStation, MFi and others) play as soon
  as they are connected, and the touch controls hide. With no controller
  connected when the game starts, the touch screen plays as player 1.
- **Rumble**: on an iPhone, the game's rumble and a light tap on each press
  are felt in the phone (iPads have no Taptic Engine). The **Controls**
  page sets the rumble's strength.
- **Leaving**: tap the small cross at the top left and confirm
  *Quit the game?*. The game shuts down like a Wii does, and the app closes:
  open it again to come back to the launcher.

Real Wii Remotes cannot connect: iOS apps have no access to them.

## 6. Your files

Recharged keeps everything in its folder in the Files app
(*On My iPhone (or iPad) → Recharged*): `recharged.ini` (settings), `data/`
(saves and the shader cache), `game/` (imported disc images), `logs/` and
`texture_packs/`. The folder buttons in the launcher open these folders in
the Files app.

**Deleting the app deletes this folder**, saves included. Copy `data/nand`
somewhere else first if you want to keep your saves.

**Texture packs**: put the pack's folder into `texture_packs` with the Files
app; **Open** on the **Gameplay** page shows that folder.

## 7. Updates

Recharged cannot update itself on iOS. When a new release is out, the
launcher says so, as on the other systems, and **Open release page** opens
the release page. Download the new `.ipa` and install it the way you
installed the first one: installing over the app keeps your settings and
saves. Online play needs the newest release, as on the other systems.

## 8. Troubleshooting

- **The app does not open any more**: with a free Apple Account, its
  signature lasted 7 days. Refresh it in SideStore or AltStore, or install it
  again.
- **"The signing certificate … was revoked"** when installing: your Apple
  Account's development certificate changed (each new one created by a tool
  revokes the old one). In SideStore, sign out and in again, revoke the
  certificate it offers, sign SideStore again, then install Recharged again.
- **"Untrusted Developer"**: trust your Apple Account under
  *Settings → General → VPN & Device Management*.
- **Something else**: the log of the last run is
  `logs/recharged.log` in the Recharged folder of the Files app (the run
  before: `recharged.previous.log`). Attach it to an
  [issue](https://github.com/previouslevelgames/recharged/issues), with your
  device's model and iOS version ([reporting a problem](TROUBLESHOOTING.md#reporting-a-problem)).
