# Port iOS / iPadOS — cahier des charges

Ce document décrit le portage de Recharged vers iPhone et iPad : ce qu'il doit
faire, comment il est construit, où il en est, et les pièges déjà rencontrés.
**Il fait foi** : le lire en entier avant de reprendre le travail, et le tenir
à jour à chaque changement : statut des exigences (§ 3), conception (§ 4),
pièges (§ 5), et une entrée dans le suivi des travaux (§ 11) pour chaque lot
de travail, avec ses commits et sa vérification. Les runs de CI sont
supprimés au fil du temps : citer des commits, pas des numéros de runs.

Le dépôt est public : ce document ne contient que des informations
techniques (pas de données personnelles, pas de citations d'échanges).

## 1. Objet et périmètre

- **Objet** : faire tourner Recharged (launcher et jeu) nativement sur iPhone
  et iPad, à partir de la branche `ios-port` du fork `zozo787/recharged`.
- **Base** : `main` du projet d'origine (Recharged **1.1.7**, `a8a4cf3`),
  fusionnée dans `ios-port` le 2026-11-14 ; avant, `stable` 1.1.6
  (`84e9c7c`). Le mainteneur veut les pull requests vers `main` : `main` y
  est fusionnée à chaque avancée.
- **Distribution** : `.ipa` non signé produit par la CI, signé et installé par
  chacun avec son identifiant Apple (Sideloadly, AltStore, SideStore, Xcode).
  Pas d'App Store.
- **Usage** : personnel ; le jeu (image disque) est fourni par la personne qui
  joue, jamais par le dépôt.
- **Hors périmètre** (voir § 3.3) : vraies Wiimotes, menu HOME de la Wii,
  pauses au pliage et en arrière-plan, Simulateur Xcode (reporté).

## 2. Règles

- **Aucune donnée de jeu dans Git.** Ni image disque (`.iso`, `.rvz`,
  `.wbfs`, …), ni DOL, ni fichiers extraits. Le `.gitignore` les exclut, le
  workflow iOS refuse de compiler s'il en trouve, et `tools/package_ios.py`
  refuse de les mettre dans l'app.
- **Commits** : même auteur que les commits existants de `ios-port` (voir
  `git log`), avec la ligne `Co-Authored-By` habituelle, sans lien de session.
  Message au format du projet d'origine (`CONTRIBUTING.md`) :
  `type(domaine): description`, avec `feat`, `fix`, `docs`, `ci`, `chore`
  ou `revert`, et le domaine `ios` (par exemple `fix(ios): ...`). **Pas de
  numéro d'issue** dans les commits du fork : GitHub les relierait à
  l'issue #163 du projet d'origine et les afficherait dans sa discussion.
  Le numéro ne va que dans le titre d'une pull request vers le projet
  d'origine (`feat(ios): … (iOS and iPad OS port #163)`, § 9), qui devient
  son commit.
  Un changement = **un seul commit** `feat(ios)` / `fix(ios)` avec le code
  et sa documentation (conception, pièges, suivi décrit sans son propre
  identifiant), au message qui décrit le changement. Le résultat d'un essai
  sur l'appareil, et l'identifiant du commit dans le suivi, vont ensuite dans
  un commit `docs(ios)`. Un commit qui ne touche que la documentation porte
  `[skip ci]` dans son texte : aucun workflow ne se lance, même après un
  push forcé.
- **Autres plateformes intactes** : le code propre à iOS est derrière
  `MSCHARGED_PLATFORM_IOS` (C++) ou `MSCHARGED_IOS` (CMake) ; ce qui est
  propre à macOS garde `__APPLE__ && !MSCHARGED_PLATFORM_IOS`. Les fichiers
  propres à iOS ne sont compilés que pour iOS. Après un changement de code
  partagé, vérifier les autres plateformes (§ 6.2).
- **Projet d'origine** (`previouslevelgames/recharged`) : `main` reçoit le
  développement et les pull requests, `stable` les versions. Son
  `CONTRIBUTING.md` demande une issue d'abord pour les gros changements, une
  modification par pull request, `ctest --preset debug` qui passe, les
  changements du jeu sous forme de patchs, et accepte l'aide d'une IA si
  l'auteur assume le résultat. Issue
  [#163](https://github.com/previouslevelgames/recharged/issues/163) : port
  accepté, pull requests vers `main` (§ 9).

## 3. Exigences

Statuts : **Validé** (essayé sur appareil), **Fait** (compilé, pas encore
essayé), **À faire**, **Abandonné**.

### 3.1 Fonctionnelles

| # | Exigence | Statut |
|---|---|---|
| F1 | App universelle iPhone + iPad, arm64, Metal, iOS 18.0 minimum | Validé |
| F2 | La CI produit un `.ipa` non signé à chaque push sur `ios-port` | Validé |
| F3 | Plein écran natif sur chaque format d'écran (pas de cadre de compatibilité), paysage | Validé (iPhone Duo, iPad Pro M5) |
| F4 | Réglages, données, sauvegardes et journaux dans `Documents`, visible dans Fichiers | Validé |
| F5 | Importer une image disque depuis n'importe où (sélecteur d'iOS) | Validé |
| F6 | Les boutons de dossier ouvrent le bon dossier dans Fichiers | Validé pour `game`, `logs`, `texture_packs` ; dossier de `recharged.ini` : ouvre `recharged.ini` (limite d'iOS acceptée, § 5) |
| F7 | Launcher utilisable au doigt (défilement avec inertie) | Validé |
| F8 | Launcher dans la zone sûre de l'écran | Validé (iPhone Duo, iPad Pro M5 ; les marges sont lues sur l'appareil) |
| F9 | Réglages sans effet sur iOS masqués dans le launcher | Validé |
| F10 | Contrôles tactiles couvrant tous les boutons utiles de la Wiimote et du Nunchuk | Validé (iPhone Duo, iPad Pro M5) |
| F11 | Pointeur de la Wiimote au doigt | Validé |
| F12 | Disposition des contrôles réglable avant la partie (position, taille, opacité, retour au défaut) | Validé |
| F13 | Manette Bluetooth (MFi, DualSense, Xbox…) ; elle remplace les contrôles tactiles quand elle se connecte | Validé (DualSense, iPhone Duo et iPad Pro M5) |
| F14 | Quitter le jeu depuis l'écran tactile, avec confirmation | Validé (ferme l'app, § 5) |
| F15 | Jeu fluide (60 FPS) sur l'appareil de test | Validé (120 FPS possible) |
| F16 | Fonctionne sur les autres iPhone et iPad sous iOS 18+ | iPad Pro M5 validé ; autres iPhone à tester |
| F17 | Disque GameCube de l'option Super Team importable | Validé (iPhone Duo ; sur iOS, le sélecteur ne s'ouvre qu'avec « Choose image… ») |
| F18 | Vibration légère à chaque appui sur un contrôle tactile, désactivable | Validé (iPhone Duo ; sans effet sur iPad, sans Taptic Engine) |
| F19 | Vibrations du jeu (rumble de la Wiimote) ressenties dans l'iPhone avec l'écran tactile | Validé (iPhone Duo : réalistes en jeu) |

### 3.2 Non fonctionnelles

| # | Exigence | Statut |
|---|---|---|
| N1 | Linux, Steam Deck, Windows et macOS compilent avec les changements du portage | Validé à `2704410` (1.1.7) : Linux, Steam Deck, Windows, macOS x86_64, arm64 et universel (Build binaries) |
| N2 | `ctest --preset debug` passe | Validé à `2704410` (1.1.7) : 61 tests sur 61, sous Linux (Clang 18) |
| N3 | Mémoire suffisante sur les modèles à 3 Go (iPhone XR, SE 2) | À tester |
| N4 | Notices de licence des crates Rust dans l'app | Validé (CI : 64 crates couvertes, `LICENSES/rust/<crate>-<version>`) |
| N5 | Build CI d'environ 10 min avec les caches | Validé (7 à 10 min ; ~30 min à froid) |

### 3.3 Abandonné ou hors périmètre

- **Pause « Dépliez pour continuer »** au pliage, et **pause en arrière-plan** :
  abandonnées (pas une priorité). Si elles reviennent : ne jamais déclencher
  le comportement du pliage sur un appareil non pliable (rotation, Stage
  Manager), détecter d'abord l'appareil.
- **Menu HOME** : non compilé dans les builds normaux du projet d'origine
  (§ 5). Pas de bouton HOME tactile tant qu'il ne l'active pas.
- **Vraies Wiimotes** : Bluetooth classique inaccessible aux apps iOS.
- **Retour au launcher sans fermer l'app** : abandonné. Le jeu (code
  d'origine de la Wii) ne sait pas s'arrêter : Recharged termine le processus
  (`TerminateProcessNow`) puis en relance un (`ReopenLauncher`), ce qu'iOS
  refuse. Le faire dans le même processus demanderait d'arrêter ses fils,
  libérer sa mémoire, remettre à zéro ses variables globales et le moteur
  graphique. Alternative possible si besoin : un « Reset » vers l'écran titre
  du jeu (comme le bouton Reset de la Wii), sans repasser par le launcher.
- **Simulateur Xcode** (essayer d'autres tailles d'écran sur un Mac) : reporté.

## 4. Conception

### 4.1 Build et CI

- **CMake** : `CMAKE_SYSTEM_NAME=iOS` donne `MSCHARGED_IOS` (CMake) et la
  définition `MSCHARGED_PLATFORM_IOS=1`. Preset `ios-release`
  (`CMakePresets.json`) : hérite de `release`, `build/ios-release`, arm64,
  iOS 18.0, Rust `aarch64-apple-ios`, sans tests ; ne s'active que sur un Mac.
  iOS est admis comme macOS dans `OriginalGame.cmake`,
  `NativeAXModuleMemory.cmake`, `OriginalNativeCompilerProfile.cmake`,
  `OriginalSourceWarnings.cmake`, `AppIcon.cmake`, `AuroraHost.cmake`,
  `MacOSLinkLibraries.cmake` ; pas d'IOKit (`SystemUsage.cmake`).
- **Rust (nod)** : la cible de déploiement est passée à Cargo seul
  (`corrosion_set_env_vars`), jamais à tout l'environnement (§ 5).
- **Workflow `.github/workflows/ios.yml`** : sur push et pull request vers
  `ios-port` (sauf `docs/**` et `**.md`), et à la main.
  Runner par défaut **`xcode-27`** (Xcode 27.2, SDK iOS 27.2, macOS 27 arm64,
  préversion GitHub) ; entrées `runner`, `xcode` (`newest` par défaut),
  `regions` (`eu` par défaut ; `us`, `jp`, `kr`, `all`), et variables de dépôt
  `IOS_RUNNER`, `IOS_XCODE`, `IOS_REGIONS`. Une file par runner, sans annuler
  le build en cours. Caches : ccache (C, C++, Objective-C/C++), sources
  préparées et SQLite, Cargo. Refuse les données de jeu ; publie l'`.ipa` en
  artefact (14 jours).
- **Autres plateformes** : `build.yml` du projet d'origine (Linux, Steam
  Deck, Windows, macOS), lancé à la main sur `ios-port` (§ 6.2). Le contrôle
  macOS dédié (`macos-check.yml`) a été retiré le 2026-11-11.

### 4.2 Paquet

- `tools/package_ios.py` assemble `Recharged.app` et
  `recharged-<version>-ios-arm64.ipa` (zip de `Payload/`), sans signature :
  programme, `Info.plist` (depuis `cmake/Info-iOS.plist.in`), icônes
  `AppIcon*`, `assets/`, modules de jeu dans `Frameworks/`, clés DT* du SDK,
  notices : celles des dépendances préparées (`LICENSES/dependencies`), et
  celles de chaque crate Rust du graphe de `nod-ffi` pour la cible iOS
  (`LICENSES/rust/<crate>-<version>`, lues dans le cache de Cargo par les
  fonctions `cargo_packages` et `cached_crate` de `package_build.py`). Le
  contrôle de la CI affiche le nombre de crates couvertes. Vérifie l'architecture et la plateforme des binaires ; option
  `--regions`.
- **Info.plist** : iPhone et iPad, paysage, `UIRequiresFullScreen`, écran de
  lancement vide (résolution native), barre d'état masquée,
  `UIFileSharingEnabled` et `LSSupportsOpeningDocumentsInPlace` (Documents
  dans Fichiers), manettes, ProMotion (`CADisableMinimumFrameDurationOnPhone`).
- **Identifiant** `io.github.zozo787.recharged`
  (`MSCHARGED_IOS_BUNDLE_IDENTIFIER`), remplaçable par les outils de sideload.
- **Modules de jeu** : un par version du jeu (US, EU, JP, KR), mêmes symboles,
  donc chargés par `dlopen` depuis `Frameworks/` (autorisé : code signé de
  l'app). CMake `MODULE` (MH_BUNDLE), `-undefined dynamic_lookup` (simple
  avertissement sur iOS). Pas de JIT : tout est compilé à l'avance.
- CMake ne fait pas de `recharged.app` autour du programme
  (`MACOSX_BUNDLE OFF`) : c'est le packager qui fait l'app.

### 4.3 Fichiers de l'utilisateur

- `UserFolder()` (`src/bootstrap/config.cpp`) = `$HOME/Documents` sur iOS
  (dossier du programme ailleurs). Y vont `recharged.ini`, `data/`
  (sauvegardes `nand/`, cache de shaders), `logs/` (journal de session,
  performances), `game/` (dossier du jeu par défaut), `texture_packs/`. Le
  programme se place dans Documents au démarrage. Ressources et modules
  restent lus dans l'app (en lecture seule).
- **Sélecteur d'iOS** (`src/launcher/ios_document_picker.{h,mm}`,
  Objective-C++, ARC, UIKit + UniformTypeIdentifiers) : SDL n'a pas de boîte
  de dialogue sur iOS. « Choose file… » et le disque GameCube ouvrent
  `UIDocumentPickerViewController` en place (`asCopy:NO`, tout fichier
  `UTTypeData`) ; pendant qu'il est ouvert, la boucle d'iOS tourne
  (`NSRunLoop`, comme `SDL_uikitmessagebox.m`) ; le fichier choisi est copié
  en arrière-plan (`NSFileCoordinator`, téléchargement iCloud compris) dans le
  dossier du jeu. Même nom et même taille déjà présents : fichier gardé ;
  sinon nom numéroté. Le bouton affiche « Importing… » ; la bibliothèque est
  relue après l'import. Chaque étape va dans le journal (« File picker: … »).
- **Dossiers** : pas de choix de dossier sur iOS ; « Change… » devient « Open
  in Files », « Browse… » (sauvegardes) devient « Show » : ils ouvrent le
  dossier dans Fichiers. `FileUrl` donne sur iOS
  `shareddocuments:///var/mobile/…/` (chemin normalisé sans `/private`, « / » final pour un
  dossier) ; chaque lien ouvert va dans le journal (« Open in Files: … »).
  Pour Documents lui-même (dossier de `recharged.ini`, « Open folder » de la
  page Advanced), aucun lien de dossier ne marche : le lien envoyé est celui
  du fichier `recharged.ini`, qui ouvre ce fichier dans Fichiers (en le
  fermant, Fichiers revient près de son accueil, pas dans le dossier). Limite
  acceptée.
  Import Dolphin masqué.
- Une réinstallation de l'app peut vider Documents (réglages et image à
  refaire).

### 4.4 Fenêtres et affichage

- Launcher et jeu en fenêtre **sans bordure** (`SDL_WINDOW_BORDERLESS`) : sur
  iOS une fenêtre couvre l'écran et suit ses rotations ; la barre d'état est
  masquée. Jamais `SDL_WINDOW_FULLSCREEN` (§ 5). `SDL_HINT_ORIENTATIONS` en
  paysage. Le jeu force `fullscreen=false`, `remember_window=false`, forme non
  verrouillée.
- **Défauts iOS** : `display.antialiasing=off`, `display.resolution=window`
  (§ 5) ; cadence `original` (60 FPS), 120 FPS possible.
- Pas d'attente 60 Hz dans la boucle du launcher sur iOS (vsync seul).
- Le journal écrit tailles de fenêtre, d'écran, orientation et zone sûre à
  chaque changement (« Screen (…) … safe area »).

### 4.5 Launcher

- **Défilement au doigt** (`src/launcher/touch_scroll.{h,cpp}`, actif sur iOS
  seulement) : au-delà de 10 points surtout verticaux, le clic est annulé et la
  fenêtre défile, avec inertie ; respecte `NoScrollWithMouse` (la fenêtre
  racine ne défile pas).
- **Zone sûre** : `UpdateSafeInsets` lit `SDL_GetWindowSafeArea` à chaque
  image (marges à 0 hors iOS, mise en page inchangée). Barre latérale :
  couleur sous la marge, contenu décalé ; page et barre du bas avant les
  marges droite et basse ; l'illustration de la page Play va jusqu'au bord.
- **Masqué sur iOS** : carte Screen, mises à jour (barre de la page Play et
  carte Updates), Discord, « Log window »,
  carte Wii Remote, « Reopen after the game », imports Dolphin ; « Mouse
  pointer » quand le joueur 1 est l'écran tactile (le doigt pointe toujours
  comme souris ; la ligne reste pour une manette, utile avec une souris ou un
  trackpad sur iPad). Textes
  « NATIVE PORT » et « natively on iPhone and iPad » ; paquet `ios`.
- **Joueurs** : choix « Touch screen » (`controls.playerN = touch`, iOS
  seulement ; défaut joueur 1), avec le bouton de l'éditeur de disposition.

### 4.6 Contrôles tactiles

- `src/runtime/touch_controls.{h,cpp}` (iOS seulement) : une **manette
  virtuelle SDL** (`SDL_AttachVirtualJoystick`, masques complets de boutons et
  d'axes) que le jeu lit comme toute manette (`desktop_wpad.cpp`, disposition
  par défaut de `bootstrap/gamepad_actions.h`). Créée seulement si un joueur
  a choisi « Touch screen », **avant** `InitializeNativeHardwareInput`.
  Dessinée par `HostOverlay` (ImGui d'Aurora), redessinée quand elle change.
- **Commandes** (libellés en anglais, police ASCII + ×) : stick (Nunchuk),
  A, B, C (utiliser l'objet), Z, « Hit » (secouer la Wiimote, gâchette
  droite), « Swap » (secouer le Nunchuk : ordre des objets, RB), − et +
  (Back, Start), 1 (pause dans ce jeu) et 2, croix directionnelle, et une
  petite croix « × » qui ouvre « Quit the game? / The app will close. »
  (Cancel, Quit).
- **Pointeur** : un doigt posé hors des boutons et hors de la zone du stick
  pointe ; sa position est envoyée comme souris (identifiant propre
  `0x54434850`) à chaque image, en gardant la dernière position. La souris
  pointe pour le joueur 1 quand l'écran tactile joue seul
  (`hardware_owner.cpp`).
- **Touchers brefs** : un toucher apparu et disparu entre deux images appuie
  son bouton 3 images (`taps_`).
- **Joueurs et emplacements** : `NativePlayers::touch`/`touch_id` réservent
  un emplacement de manette (le plus haut libre, `touch_slot`) que seule la
  manette virtuelle prend ; elle n'est jamais comptée parmi les manettes ni
  retirée comme absente. Une manette physique sans emplacement prend celui
  du tactile : les contrôles se masquent et relâchent tout, puis reviennent
  quand elle part.
- **Repli sur l'écran tactile** : les contrôles tactiles sont créés à chaque
  partie, même si personne ne les a choisis. Si aucune manette configurée
  n'est connectée au lancement, l'écran tactile devient le joueur 1, à la
  place du clavier et de la souris d'origine (un iPhone n'en a pas) ;
  sinon ils sont retirés (`NativeTouchControlsPlay`, « Touch controls: not
  playing, removed »). En attente, leur manette virtuelle ne prend aucun
  emplacement (`desktop_wpad.cpp`). Le launcher l'annonce (« With nobody
  left, the touch screen plays as player 1 », `PredictLineup`).
- **Disposition** (`src/bootstrap/touch_layout.{h,cpp}`, C++17, partagé
  launcher/jeu, iOS seulement) : chaque contrôle (A…croix, D-pad, stick) est
  ancré au bord le plus proche ou à la ligne du centre ; décalage en
  millièmes du petit côté de l'écran, taille 50-200 %. La zone où un doigt
  prend le stick suit le stick (retournée s'il passe à droite ou en haut).
  Format : `controls.touch_layout = a:rb,-200,-180,100 | …` ; vide = défaut ;
  une valeur illisible est ignorée.
- **Éditeur** (launcher, plein écran) : glisser pour déplacer, toucher pour
  sélectionner puis − / + pour la taille, curseur Opacity, interrupteur
  Vibration, Reset, Cancel, Save.
- **Opacité** : `controls.touch_opacity` 20-100 %, appliquée à tous les
  contrôles (pas à la fenêtre de confirmation).
- **Vibration à l'appui** : `controls.touch_haptics` (activée par défaut,
  interrupteur « Vibrate on press » de l'éditeur ; Reset la réactive). Un
  léger « tap » du Taptic Engine (`UIImpactFeedbackGenerator`, style léger,
  `src/runtime/touch_haptics.mm`, Objective-C++, joué sur le fil principal)
  quand un bouton passe de relâché à appuyé (un seul par image, pas pendant
  l'appui) et à l'ouverture de « Quit the game? ».
- **Vibrations du jeu (rumble)** : la manette virtuelle déclare une fonction
  `Rumble` ; le jeu y envoie ses vibrations comme à toute manette
  (`desktop_wpad.cpp`, réglages Rumble et Strength de la page Controls) ; elles
  sont jouées par Core Haptics : un événement continu en boucle dont
  l'intensité suit celle demandée, arrêté à 0 ou quand les contrôles
  tactiles disparaissent. Le moteur est refait après une interruption
  (arrière-plan, appel).
- Rien de tout cela sur les appareils sans Taptic Engine (iPad).
- `SDL_HINT_TOUCH_MOUSE_EVENTS=0` pendant le jeu, sinon la Wiimote
  clavier-souris lirait aussi les touchers.
- Journal : « Touch controls: on / default|custom layout, N % opacity /
  virtual gamepad N attached / drawing on … / shown|hidden / Quit ».

### 4.7 Sortie

La croix envoie `SDL_EVENT_WINDOW_CLOSE_REQUESTED` à la fenêtre du jeu, que
`hardware_owner.cpp` traite comme le bouton d'alimentation de la Wii :
extinction propre, puis l'app se ferme. `launcher.reopen_after_game` est
masqué sur iOS et jamais armé.

### 4.8 Configuration propre à iOS

| Clé | Valeurs | Défaut iOS |
|---|---|---|
| `controls.player1-4` | ajoute `touch` | joueur 1 = `touch` |
| `controls.touch_layout` | voir § 4.6 | vide (défaut) |
| `controls.touch_opacity` | 20-100 | 100 |
| `controls.touch_haptics` | `true` / `false` (« tap » à l'appui) | `true` |
| `display.antialiasing` | inchangé | `off` |
| `display.resolution` | inchangé | `window` |

Une installation existante garde ses anciennes valeurs (le launcher écrit
tous les réglages dans `recharged.ini`).

### 4.9 Code partagé modifié (neutre hors iOS)

`desktop_wpad.cpp` / `.h` (emplacement tactile −1 hors iOS ; l'emplacement
est cherché avant d'ouvrir une manette), `hardware_owner.cpp` / `.h` (joueur
tactile −1), `host_overlay.*` (dessin tactile sous garde iOS), launcher
(marges 0, `touch_scroll` désactivé, `FileUrl`), `config.*` /
`launch_options.cpp` (clés tactiles et `UserFolder` sous garde iOS),
`session_log.cpp`, `system_usage.cpp`, `hbm_debug.cpp` (`vm_region_64`),
`tools/prepare_sqlite.py` (ne retire que les variables des cibles iOS et
apparentées).

## 5. Pièges connus (cause → solution)

- **Cadre de compatibilité 16:9 sur l'iPhone Duo** → l'app liée à un SDK
  antérieur à l'appareil (26.x) : compiler avec le **SDK iOS 27** (runner
  `xcode-27`). `UIRequiresFullScreen` n'y était pour rien.
- **Image en portrait sur un écran en paysage** → `SDL_WINDOW_FULLSCREEN`
  fige la fenêtre au mode lu au démarrage : fenêtre sans bordure.
- **Jeu à 30 FPS** → processeur graphique saturé par l'antialiasing 4x sur
  ~5,7 Mpx : antialiasing désactivé par défaut.
- **Outils du Mac compilés pour iOS** (échec de SQLite) →
  `IPHONEOS_DEPLOYMENT_TARGET` dans tout l'environnement : ne jamais
  l'exporter ; `prepare_sqlite.py` le retire.
- **`recharged.app` créé par CMake** → `MACOSX_BUNDLE OFF`.
- **Contrôles tactiles sans effet**, trois causes successives : manette
  virtuelle créée après le comptage des manettes ; `TouchControls` créé
  après `InitializeNativeHardwareInput` ; joueur 1 réglé sur `keyboard`
  (aucune manette ouverte) → « Touch screen » comme choix de joueur.
- **Seuls A, B et le stick marchaient** → SDL numérote les entrées d'une
  manette virtuelle dans l'ordre des bits des masques : masques complets.
- **Gâchettes à mi-course** → un axe de gâchette repose à
  `SDL_JOYSTICK_AXIS_MIN`, pas à 0.
- **Touchers près des bords livrés en retard** (gestes système différés) →
  boutons éloignés du bord, et `taps_`.
- **Pointeur qui revient au centre** → le jeu n'écoute la souris que 3 s
  après son dernier mouvement : renvoyer la dernière position à chaque image.
- **HOME sans effet** → le menu HOME n'est compilé que si
  `MSCHARGED_DIAGNOSTIC_FRONTEND_HOME` est activée (patch
  `0593-original-home-task-admission.patch`), ce qu'aucun preset ne fait,
  même à l'origine. La pause de ce jeu est le bouton **1**.
- **Relance de l'app refusée** (`execv` de `ReopenLauncher`) → quitter ferme
  l'app.
- **« This operation is not supported »** sur les choix de fichier → SDL n'a
  pas de boîte de dialogue sur iOS : sélecteur d'iOS (§ 4.3). Un accès à un
  fichier hors de l'app ne survit pas à un redémarrage sans signets
  (security-scoped bookmarks), jugés trop fragiles : copie dans l'app.
- **Sélecteur blanc** → son contenu vient d'un autre processus, qui a besoin
  de la boucle d'iOS : la faire tourner tant qu'il est ouvert.
- **Choix ignoré** → pendant sa présentation, `presentingViewController` est
  encore nul : ne conclure à une fermeture qu'après la fin de la
  présentation (ou 10 s sans apparition).
- **Fichiers reste sur l'ancien dossier pour Documents** (dossier de
  `recharged.ini`) → Fichiers ouvre les sous-dossiers de l'app par un lien
  `shareddocuments:///var/mobile/…/dossier/`, mais pas le dossier principal
  de l'app, quelle que soit la forme du lien (`/private/var` ou `/var`) ; un
  lien vers un fichier qui s'y trouve ouvre ce fichier ; le navigateur de
  documents d'iOS (`UIDocumentPickerViewController`) ignore son dossier de
  départ (`directoryURL`), même donné par l'URL exacte de `NSFileManager`,
  et se rouvre là où il a servi la dernière fois ; une liste du dossier dans
  le launcher n'y emmène pas ; le lien sans « / » final ne fait pas mieux
  (`dd04a78`). Recherche : `shareddocuments://` n'est pas documenté par
  Apple ; un ingénieur Apple (forums développeurs, fil 751391, 2024) indique
  qu'aucun schéma d'URL documenté n'ouvre ou ne révèle un dossier dans
  Fichiers sur iOS (l'équivalent de `NSWorkspace.activateFileViewerSelecting`
  manque) ; un bug ouvert depuis iOS 14.6 (FB9359784) signale que le schéma
  marche mal pour « Sur mon iPhone » ; `UIDocumentBrowserViewController`
  ne révèle un document que s'il est la vue racine de l'app. Retenu : le lien
  vers `recharged.ini`, qui ouvre au moins le fichier de réglages (le fermer
  ramène près de l'accueil de Fichiers, pas dans le dossier).
- **« Nintendo Wii Remote » dans le journal sans rien de connecté** → normal :
  Wiimote virtuelle créée par `desktop_wpad.cpp` pour chaque manette qui
  joue ; virtuelle, elle ne masque pas les contrôles tactiles.
- **Joueurs en ligne indisponibles** → libcurl absent sur iOS.
- **Joueur 1 = manette éteinte au lancement** → la règle d'origine donnait le
  joueur 1 au clavier et à la souris, absents d'un iPhone, et les contrôles
  tactiles n'existaient pas : partie sans commandes ni croix pour quitter.
  Repli sur l'écran tactile (§ 4.6).
- **Mises à jour forcées de la 1.1.7** (installation automatique, jeu en
  ligne fermé à une version officielle dépassée ou qui n'a pas pu vérifier)
  → ne concernent que les builds `release` et `test`
  (`EnforcesUpdates`, `GatesOnline`, `launcher/online_gate.cpp`) ; l'`.ipa`
  de la CI est une build `dev` (`MSCHARGED_BUILD_CHANNEL=dev`) : ni
  installation, ni blocage. Le paquet est `ios-arm64`
  (`launcher/current_build.h`).
- **« Run workflow » absent** pour un workflow propre à `ios-port` → GitHub ne
  l'affiche que pour les workflows de la branche par défaut (`stable`).
- **Envoi de l'`.ipa` en échec** (« Unexpected token '<', "<!DOCTYPE "… is
  not valid JSON ») → panne du service d'artefacts de GitHub, qui renvoie une
  page d'erreur : relancer le run plus tard.
- **Run bloqué « en cours »** alors que toutes ses étapes sont finies (envoi
  de l'artefact en échec) → GitHub refuse de l'annuler ou de le supprimer ;
  il se clôt seul. Pendant ce temps, un nouveau build iOS attend derrière lui
  (même file, sans annulation).

## 6. Procédures

### 6.1 Build iOS

- Automatique à chaque push sur `ios-port` qui touche autre chose que la
  documentation ; l'`.ipa` est dans les artefacts du run.
- Sur un Mac :

```sh
brew install cmake ninja python rustup tcl-tk
export PATH="$(brew --prefix tcl-tk)/bin:$PATH"
rustup target add aarch64-apple-ios
python3 tools/setup_dependencies.py
cmake --preset ios-release --fresh
cmake --build --preset ios-release --target mscharged mscharged_original_frontend_module_eu
python3 tools/package_ios.py --regions eu
```

L'`.ipa` arrive dans `build/ios-artifacts/`.

### 6.2 Vérifier les autres plateformes

Onglet Actions → **Build binaries** (`build.yml`, présent sur `stable`, donc
avec « Run workflow ») → branche `ios-port`. Il compile Linux, Steam Deck,
Windows et macOS ; ses jobs macOS demandent des runners Apple Silicon,
parfois indisponibles.

`ctest --preset debug` (exigé pour une pull request), sous Linux : le preset
`debug` compile le launcher et les outils, sans le jeu ni image du jeu.

```sh
sudo apt-get install -y clang cmake ninja-build make tcl pkg-config \
  libasound2-dev libpulse-dev libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxfixes-dev libxss-dev libxtst-dev \
  libx11-xcb-dev libegl-dev libwayland-dev libxkbcommon-dev \
  wayland-protocols libdecor-0-dev libudev-dev libdbus-1-dev libvulkan-dev
export CC=clang CXX=clang++
python3 tools/setup_dependencies.py
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Paquets repris du job Linux de `build.yml` ; Clang car le GCC d'Ubuntu
24.04 (13) est trop ancien. `setup_dependencies.py --launcher` ne suffit
pas pour `debug` (zlib-ng manque) : préparation complète. Dawn (hébergé
chez Chromium) n'est pas nécessaire à ces tests si son téléchargement est
bloqué.

### 6.3 Installer

Signer et installer l'`.ipa` avec un identifiant Apple (Sideloadly, AltStore,
SideStore ou Xcode ; compte gratuit : 7 jours, 3 apps). Mode développeur
activé sur l'appareil, profil approuvé dans Réglages → Général → VPN et
gestion de l'appareil.

### 6.4 Diagnostiquer

- Journal de session : Fichiers → Recharged → `logs/recharged.log` (la
  session précédente : `recharged.previous.log`).
- Console de l'appareil : app Console d'un Mac, appareil branché, filtre sur
  le nom de l'app ou le service concerné.

### 6.5 Notes pour les sessions de travail

- Dans l'environnement des sessions, l'API GitHub en écriture (lancer un
  workflow, supprimer des caches) est bloquée ; la lecture (`gh run list`,
  `gh run view`) et `git push` marchent. Lancer un build = pousser.
- Vérification locale rapide (Linux) :
  `clang++ -std=c++20 -fsyntax-only -DMSCHARGED_PLATFORM_IOS=1 -Isrc
  -Iextern/sdl/include -Iextern/imgui …` ; pour `src/launcher/main.cpp`,
  ajouter `-Iextern/imgui/backends -Iextern/imgui/misc/cpp`,
  `-DMSCHARGED_BUILD_CONFIG="Release"` et un `mscharged/build_version.h` tiré
  de `cmake/build_version.h.in`. L'Objective-C++ ne se vérifie qu'en CI.

## 7. Appareils de test et mesures

- **iPhone Duo** (pliable, iPhone19,4, iOS 27.2.1), joué déplié. SDL voit deux
  écrans : extérieur 466×678 points, intérieur 951×669, 120 Hz, densité 3.
  En paysage : plié, fenêtre 678×466 sans marge ; déplié, fenêtre 951×669
  (2853×2007 pixels), zone sûre 951×635 (**34 points en bas**, barre
  d'accueil).
- **iPad Pro M5** : validé : affichage aux bonnes dimensions, choix du jeu,
  contrôles tactiles, DualSense (les contrôles tactiles disparaissent quand
  elle joue).
- Manette **DualSense** : reconnue, le jeu répond.

## 8. Reste à faire

- Essais sur d'autres iPhone (F16), dont un modèle à 3 Go (N3).
- Piste, demandée par le mainteneur : notification de nouvelle version (sans installation, impossible
  sur iOS : app en lecture seule, signée, qui ne peut ni se remplacer ni se
  relancer). Demanderait des versions iOS publiées (un paquet `ios-arm64`
  joint aux releases), un accès réseau sur iOS (libcurl absent : passer par
  `NSURLSession`), et une interface qui informe et donne le lien, sans
  bouton « Installer ». Pour une pull request : mises à jour masquées,
  signalées comme limite connue.
- Pull requests vers `main` du projet d'origine (§ 9).
- À la fin du portage : regrouper l'historique de `ios-port` en un commit
  par fonctionnalité (compilation iOS, fichiers dans Documents, plein écran,
  launcher tactile, contrôles tactiles, éditeur de disposition, vibrations,
  fichiers et dossiers, cahier des charges), au-dessus de la version de base,
  par un push forcé ; code final identique, identifiants mis à jour ici.

## 9. Contribution au projet d'origine

Issue [#163](https://github.com/previouslevelgames/recharged/issues/163) :
le mainteneur accepte le port dans le dépôt d'origine (2026-11-13).

**Pull requests** vers `main` (pas `stable`), petites, dans cet ordre :

1. Cible de build iOS, paquet et CI : un job iOS **dans `build.yml`**, que
   les jobs de version et de build de test n'attendent pas (un build iOS en
   échec ne bloque jamais une version).
2. Dossiers de l'app et gestion des fenêtres.
3. Contrôles tactiles comme choix de joueur.
4. Changements du launcher.
5. Vibrations.

**Exigences du mainteneur** :

- **Titres** : ils fusionnent en un seul commit (squash), le titre devient
  le commit : format du projet avec le numéro de l'issue, par exemple
  `feat(ios): touch controls (iOS and iPad OS port #163)`.
- **HTTP** : sur les systèmes Apple, le launcher charge la libcurl du
  système à l'exécution ; absente sur iOS, la recherche de mises à jour, les
  joueurs en ligne et les rapports de match échouent : un système propre à
  iOS (`NSURLSession`).
- **Mises à jour** : `main` (1.1.7) installe les mises à jour toute seule et
  ferme le jeu en ligne aux versions dépassées ; une app iOS ne peut pas se
  remplacer : sur iOS, le launcher invite à installer le nouvel `.ipa`.
- **Paquet** : `package_ios.py` prend **toute** sa liste de fichiers dans
  `package_build.py`, pour que les nouveaux fichiers suivent seuls.
- **CMake** : un seul test de plateforme partagé au lieu d'ajouter iOS à
  chaque test de système (les 7 fichiers `cmake/*.cmake` du § 4.1).
- **Docs** : une courte page d'installation pour les joueurs dans le dépôt ;
  ce cahier des charges reste dans le fork (ou dans les descriptions des
  pull requests).
- **Après l'intégration** : surveiller le job iOS et tester sur appareil
  autour des sorties de version.

Préparation : branches partant de `main`, une par pull request ; `ctest
--preset debug` et Build binaries verts (§ 6.2) ; des tests pour `touch_layout`.

## 10. Historique

- **2026-11-09** — Cible iOS dans CMake, `Info.plist`, packager, workflow ;
  premier `.ipa` (`f27bf6e`). Premier démarrage : launcher, détection
  du disque, jeu lancé. Dossiers de l'utilisateur dans Documents. Caches CI.
- **2026-11-10** — SDK iOS 27 (`xcode-27`) : plus de cadre de compatibilité ;
  fenêtres sans bordure ; antialiasing désactivé par défaut. Contrôles
  tactiles, « Touch screen » comme choix de joueur, pointeur au doigt, sortie
  par la croix. Fusion de Recharged 1.1.6. Autres plateformes vérifiées.
  Finitions du launcher (réglages inutiles masqués). Issue #163 ouverte.
  Bouton HOME retiré ; « Changer ». Éditeur de disposition (`2b2bf06`),
  tactile compilé pour iOS seulement (`7838986`), opacité et zones sûres
  (`9d61642`).
- **2026-11-11** — Import d'image par le sélecteur d'iOS (`625bac3` à
  `da79c7f`), liens vers Fichiers (`cd181e6`). Historique de `ios-port`
  réécrit (poussé de force) pour retirer les détails personnels des
  anciennes versions de ce document ; le code de chaque commit est inchangé.
  Document refondu en cahier des charges.
- **2026-11-11 (suite)** — Messages de commit au format du projet
  d'origine, sans numéro d'issue. Dossier des réglages : aucun lien ne mène
  au dossier principal de l'app ; le bouton ouvre `recharged.ini`
  (`ce35205`). Notices de licence de 64 crates Rust dans l'app (`21175a3`).
  Revue (`83b27b6`). Contrôle macOS dédié retiré (`76aabcc`). Simulateur
  Xcode essayé en local, sans changement du dépôt : l'app compile et
  démarre. iPad Pro M5 validé.
- **2026-11-12** — Vibration à chaque appui (`6be3d5f`) et vibrations du jeu
  dans l'iPhone par Core Haptics (`478eba0`). Disque GameCube : sélecteur
  ouvert seulement par « Choose image… » (`42c58f1`). « Mouse pointer »
  masqué pour l'écran tactile (`d0ca827`). Repli sur l'écran tactile quand
  aucune manette configurée n'est là (`40b53c5`). Retour au launcher sans
  fermer l'app abandonné.
- **2026-11-14** — Fusion de `main` du projet d'origine (Recharged 1.1.7 :
  mises à jour forcées, jeu en ligne fermé aux versions dépassées, rollback) :
  `CurrentBuild()` déplacé dans `launcher/current_build.h` (paquet `ios`
  reporté), carte Updates réécrite (toujours masquée sur iOS). Build iOS,
  Build binaries (toutes les plateformes) et `ctest --preset debug` (61/61)
  passent à `2704410`. Le mainteneur accepte le port : plan des pull
  requests au § 9. Libellés des contrôles tactiles traduits en anglais
  (« Hit », « Swap », « Quit the game? »).

## 11. Suivi des travaux

Un lot = un objectif, ses commits sur `ios-port`, la vérification faite
(CI : compilation ; appareil : essai sur l'iPhone Duo ou l'iPad) et son état.
Les commits sont ceux de l'historique réécrit le 2026-11-11 (messages
renommés au format du projet le même jour).

| Lot | Date | Objet | Commits | Vérification | État |
|---|---|---|---|---|---|
| L1 | 11-09 | Cible iOS dans CMake, `Info.plist`, packager, workflow ; cible de déploiement gardée hors des outils du Mac ; programme simple pour le packager ; module EU seul par défaut | `25ce29d` `0e4e72f` `ba799fa` `0086a6c` `f27bf6e` | CI : `.ipa` produit | Terminé |
| L2 | 11-09 | Fichiers de l'utilisateur dans Documents ; launcher et jeu en plein écran ; barre de mise à jour masquée | `b496c32` `fda1f34` | Appareil : app, launcher et jeu démarrent | Terminé |
| L3 | 11-09/10 | CI plus rapide : caches (sources préparées, SQLite, Cargo, ccache Objective-C/C++), un job par cœur, Xcode le plus récent, choix du runner, une file par runner | `564d56e` `0c45ed3` `a6940f4` `d94d399` | CI : ~23 → 7-10 min avec cache | Terminé |
| L4 | 11-10 | Plein écran natif : SDK iOS 27 (runner `xcode-27`), fenêtres sans bordure ; essais antérieurs retirés | `45998c4` `09f3614` `5e2344c` | Appareil : plus de cadre, écran rempli | Terminé |
| L5 | 11-09/10 | Défilement du launcher au doigt (fenêtre racine exclue) ; rythme vsync seul | `bd2b07b` `a460039` `a6940f4` | Appareil : défilement fluide | Terminé |
| L6 | 11-10 | Performances : défauts sans antialiasing, rendu à la taille de la fenêtre | `4c8b8d6` `66bdfff` | Appareil : 60 FPS, 120 FPS stable | Terminé |
| L7 | 11-10 | Contrôles tactiles : manette virtuelle, « Touch screen » comme choix de joueur, remplacement par une manette physique, pointeur au doigt, masques complets, touchers brefs, −/1/2, traces de diagnostic puis retrait | `a460039` `4c0c5a7` `d11bc4d` `c993fa8` `801f7b9` `a7db6ec` `c0cd67d` `7b8bff5` `0168fdd` `001d0ba` `9b21288` `86f05e3` | Appareil : tous les boutons et le pointeur | Terminé |
| L8 | 11-10 | Fusion de Recharged 1.1.6 | `6900d32` | CI | Terminé |
| L9 | 11-10 | Sortie du jeu : croix discrète et confirmation ; l'app se ferme (relance refusée par iOS) | `daad627` `3fe0f1f` `16314bd` `d70de62` | Appareil : Annuler et Quitter | Terminé |
| L10 | 11-10 | Autres plateformes : contrôle macOS manuel ; code partagé neutre | `a9505c1` `6456b35` `86f05e3` | CI : Linux, Steam Deck, Windows, macOS | Terminé |
| L11 | 11-10 | Launcher : réglages sans effet sur iOS masqués, textes iPhone/iPad | `b8da438` | Appareil | Terminé |
| L12 | 11-10 | Bouton HOME retiré (menu HOME non compilé à l'origine) ; « Objet » renommé « Changer » | `f3daeb1` `ba2f727` | Appareil | Terminé |
| L13 | 11-10 | Éditeur de disposition des contrôles ; tactile compilé pour iOS seulement | `2b2bf06` `7838986` `2289565` `1de337b` | Appareil : éditeur, reprise en jeu, Reset ; CI : macOS | Terminé |
| L14 | 11-10 | Opacité des contrôles ; launcher dans la zone sûre ; zone sûre dans le journal | `9d61642` | Appareil ; marges de l'iPhone Duo relevées | Terminé |
| L15 | 11-10 | Revue : formes partagées entre tactile et disposition | `7978672` | CI | Terminé |
| L16 | 11-11 | Import d'image par le sélecteur d'iOS (copie dans `Documents/game`) : en place, copie en arrière-plan, boucle d'iOS pendant le sélecteur, attente de sa présentation | `625bac3` `4715a66` `d4704f4` `da79c7f` | Appareil : import complet | Terminé |
| L17 | 11-11 | Liens vers Fichiers au format d'iOS (`shareddocuments:///var/mobile/…/`, « / » final) pour les sous-dossiers. Dossier principal de l'app : lien de dossier avec ou sans « / » final (`dd04a78`) sans effet, navigateur de documents d'iOS (`ffcd6b5`, `86413bf`) rouvert sur son dernier dossier, liste dans le launcher (`5a61338`) qui n'y emmène pas ; retenu : lien vers `recharged.ini` (`ce35205`), qui ouvre le fichier de réglages | `cd181e6` `3bc3a9a` `93d6cbb` `ffcd6b5` `86413bf` `5a61338` `dd04a78` `ce35205` | Appareil : `game`, `logs`, `texture_packs` s'ouvrent ; `recharged.ini` s'ouvre, le dossier principal non (limite d'iOS acceptée) | Terminé |
| L18 | 11-11 | Documentation publique : historique réécrit sans détails personnels, « iPhone Duo » dans les commentaires, document refondu en cahier des charges | `23770ca` `44b1cd4` `baec666` | — | Terminé |
| L19 | 11-11 | Notices de licence de chaque crate Rust dans l'app (exigence N4), comme les paquets de bureau ; nombre de crates couvertes affiché au contrôle de la CI | `21175a3` | CI : 64 crates couvertes, `.ipa` de 18,4 à 18,8 Mo | Terminé |
| L20 | 11-12 | Vibration légère à chaque appui sur un contrôle tactile (F18), puis vibrations du jeu jouées par l'iPhone avec l'écran tactile (F19) ; carte Rumble adaptée à iOS, interrupteur de l'éditeur renommé « Vibrate on press » | `6be3d5f` `478eba0` | Appareil : tap et vibrations du jeu validés, réalistes | Terminé |
| L21 | 11-12 | Disque GameCube (Super Team) : sur iOS, cocher l'option n'ouvre plus le sélecteur, seul « Choose image… » le fait | `42c58f1` | Appareil : correctif et chargement de l'image validés | Terminé |
| L22 | 11-12 | « Mouse pointer » masqué quand le joueur 1 est l'écran tactile, où il ne change rien (masqué aussi pour « Keyboard & mouse », comme à l'origine : la souris y pointe déjà) | `d0ca827` | Appareil : masqué avec Touch screen, visible avec la DualSense | Terminé |
| L23 | 11-12 | Repli sur l'écran tactile quand aucune manette configurée n'est là au lancement (au lieu du clavier et de la souris), texte du launcher adapté | `40b53c5` | Appareil : DualSense éteinte, l'écran tactile joue ; allumée, elle joue sans contrôles tactiles ; Touch screen inchangé | Terminé |
| L24 | 11-14 | Fusion de `main` du projet d'origine (1.1.7) dans `ios-port` : 3 conflits dans `src/launcher/main.cpp` (inclusions, `CurrentBuild()` déplacé dans `launcher/current_build.h` où le paquet `ios` est reporté, carte Updates réécrite et gardée masquée sur iOS) | `2704410` | CI : `.ipa` 1.1.7 ; Build binaries vert (Linux, Steam Deck, Windows, macOS) ; `ctest --preset debug` 61/61 ; appareil : la 1.1.7 tourne sur l'iPhone Duo | Terminé |
| L25 | 11-14 | Libellés des contrôles tactiles en anglais, demandé pour les pull requests : « Tacle » → « Hit », « Changer » → « Swap » (noms déjà utilisés dans `controls.touch_layout`), « Quitter le jeu ? / L'application va se fermer. » → « Quit the game? / The app will close. », Annuler / Quitter → Cancel / Quit ; éditeur du launcher et journal (« Quit: … ») | ce commit | À essayer sur l'appareil (libellés dans leurs boutons) | En cours |
