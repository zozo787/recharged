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
- **Base** : `main` du projet d'origine (Recharged **1.1.7**, `b815953`),
  fusionnée dans `ios-port` le 2026-11-14 (`a8a4cf3`, `3143dbf`, puis
  `b815953`, qui fusionne la publication de `stable` sans changer le code) ; avant, `stable` 1.1.6
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
- **Historique regroupé** (depuis le 2026-11-14) : `ios-port` = `main` du
  projet d'origine, puis un commit par pull request (§ 9), puis le commit
  propre au fork (`ios.yml`, ce document). Un nouveau changement reste
  d'abord un commit à part ; avant d'ouvrir la pull request qu'il concerne,
  il est fondu dans le commit de celle-ci (push forcé, avec accord). Une
  nouvelle avancée de `main` : rebaser les commits dessus plutôt que
  fusionner.
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
| F16 | Fonctionne sur les autres iPhone et iPad sous iOS 18+ | iPad Pro M5 et iPhone 14 Plus (encoche) validés ; autres iPhone à tester |
| F17 | Disque GameCube de l'option Super Team importable | Validé (iPhone Duo ; sur iOS, le sélecteur ne s'ouvre qu'avec « Choose image… ») |
| F18 | Vibration légère à chaque appui sur un contrôle tactile, désactivable | Validé (iPhone Duo ; sans effet sur iPad, sans Taptic Engine) |
| F19 | Vibrations du jeu (rumble de la Wiimote) ressenties dans l'iPhone avec l'écran tactile | Validé (iPhone Duo : réalistes en jeu) |
| F20 | Nouvelle version signalée comme sur PC (barre de la page Play, fenêtre des mises à jour), version iOS : la page de la version, d'où l'on prend le nouvel `.ipa`, pas d'installation | Validé (iPhone Duo : version de test `v1.1.8` du fork) |
| F21 | Jeu en ligne avec les autres plateformes | Validé (iPhone Duo contre un Mac, partie en ligne) |

### 3.2 Non fonctionnelles

| # | Exigence | Statut |
|---|---|---|
| N1 | Linux, Steam Deck, Windows et macOS compilent avec les changements du portage | Validé à `34f5891` (historique regroupé, pull request 1 préparée) : Linux, Steam Deck, Windows, macOS x86_64, arm64 et universel, et le job iOS de `build.yml` (Build binaries) |
| N2 | `ctest --preset debug` passe | Validé à `2704410` (1.1.7) : 61 tests sur 61, sous Linux (Clang 18) |
| N3 | Mémoire suffisante sur les modèles à 3 Go (iPhone XR, SE 2) | À tester |
| N4 | Notices de licence des crates Rust dans l'app | Validé (CI : 64 crates couvertes, `LICENSES/rust/<crate>-<version>`) |
| N5 | Build CI d'environ 10 min avec les caches | Validé (7 à 10 min ; ~30 min à froid ; module EU seul) |

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

- **CMake** : un seul test de système, `cmake/Platform.cmake` (demandé par
  le mainteneur) : `MSCHARGED_SUPPORTED_SYSTEM` (Linux, macOS, iOS,
  Windows), `MSCHARGED_APPLE` (macOS ou iOS), `MSCHARGED_IOS` et la
  définition `MSCHARGED_PLATFORM_IOS=1`. Inclus par `CMakeLists.txt` et par
  chaque fichier qui les teste (`OriginalGame.cmake`,
  `NativeAXModuleMemory.cmake`, `OriginalNativeCompilerProfile.cmake`,
  `OriginalSourceWarnings.cmake`, `AppIcon.cmake`, `AuroraHost.cmake`,
  `MacOSLinkLibraries.cmake`, `SystemUsage.cmake`, `Launcher.cmake`) ; un
  nouveau système s'ajoute là. Pas d'IOKit (`SystemUsage.cmake`). Preset
  `ios-release` (`CMakePresets.json`) : hérite de `release`,
  `build/ios-release`, arm64, iOS 18.0, Rust `aarch64-apple-ios`, sans
  tests ; ne s'active que sur un Mac.
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
- **Job iOS de `build.yml`** (pour le projet d'origine, pull request 1) :
  `ios` (`ios-arm64`), runner `xcode-27` (variable `IOS_RUNNER`), Xcode le
  plus récent, les quatre modules de jeu, origine de la build comme les
  autres (`release`, `test`, `dev`), sans autopilote, ccache. **Ne bloque
  jamais une version** : aucun job ne l'attend (`release` et `test-build`
  ne dépendent que de `build`, `windows-game`, `macos-universal`) ; un
  échec iOS laisse tous les autres jobs aller au bout. Son artefact est
  l'`.ipa` lui-même (`recharged-<version>-ios-arm64.ipa`, sans zip).
  `release` et `test-build` n'envoient que leurs paquets nommés
  (`*.tar.gz`, `*.zip`, `*.dmg`, `SHA256SUMS`) : le job **`ios-attach`**
  (option B, retenue le 2026-11-14 : le mainteneur veut l'`.ipa` dans le
  workflow actuel) attend `ios` et `release` ou `test-build`, puis ajoute
  l'`.ipa` à la version en brouillon de ce run (`v<version>`) ou au build
  de test de la pull request (`test-pr<N>-<commit>`), et réécrit
  `SHA256SUMS` avec sa ligne. Seulement pour une version faite à ce commit
  (`targetCommitish`) ; rien ne l'attend, et si le build iOS échoue la
  version sort sans `.ipa`. Un premier essai avait mis l'artefact dans un
  zip nommé `ios-arm64`, par crainte à tort qu'il entre au hasard dans les
  versions.

### 4.2 Paquet

- `tools/package_ios.py` assemble `Recharged.app` et
  `recharged-<version>-ios-arm64.ipa` (zip de `Payload/`), sans signature :
  programme, `Info.plist` (depuis `cmake/Info-iOS.plist.in`), icônes
  `AppIcon*`, modules de jeu dans `Frameworks/`, les fichiers communs à
  tous les paquets (`common_files()` de `package_build.py`, demandé par le
  mainteneur : assets, données graphiques et de jeu, documents publics ; un
  fichier ajouté là entre dans chaque paquet), clés DT* du SDK,
  notices : celles des dépendances préparées (`LICENSES/dependencies`), et
  celles de chaque crate Rust du graphe de `nod-ffi` pour la cible iOS
  (`LICENSES/rust/<crate>-<version>`, lues dans le cache de Cargo par les
  fonctions `cargo_packages` et `cached_crate` de `package_build.py`). Le
  contrôle de la CI affiche le nombre de crates couvertes. Vérifie
  l'architecture et la plateforme des binaires ; option `--regions`. Le guide
  d'accueil de l'app est `assets/launcher/welcome-ios.md`, copié sous le nom
  `assets/launcher/welcome.md` (§ 4.5).
- **Info.plist** : iPhone et iPad, paysage, `UIRequiresFullScreen`, écran de
  lancement vide (résolution native), barre d'état masquée,
  `UIFileSharingEnabled` et `LSSupportsOpeningDocumentsInPlace` (Documents
  dans Fichiers), manettes, ProMotion (`CADisableMinimumFrameDurationOnPhone`),
  `NSAppTransportSecurity` / `NSAllowsArbitraryLoads` (`http` vers le
  serveur en ligne, § 4.9).
- **Identifiant** (`MSCHARGED_IOS_BUNDLE_IDENTIFIER`) :
  `io.github.previouslevelgames.recharged` par défaut ; le `ios.yml` du fork
  passe `io.github.zozo787.recharged`, celui des `.ipa` déjà installés (un
  autre identifiant ferait une autre app, sans ses réglages ni ses
  sauvegardes). Remplaçable par les outils de sideload.
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
- **Masqué sur iOS** : carte Screen, Discord, « Log window »,
  carte Wii Remote, « Reopen after the game », imports Dolphin ; « Mouse
  pointer » quand le joueur 1 est l'écran tactile (le doigt pointe toujours
  comme souris ; la ligne reste pour une manette, utile avec une souris ou un
  trackpad sur iPad). Textes
  « NATIVE PORT » et « natively on iPhone and iPad » ; paquet `ios`.
- **Textes adaptés à iOS** (le texte de bureau reste identique ailleurs) :
  « Tap » au lieu de « Click » (macro `LAUNCHER_CLICK`) et « this device »
  au lieu de « this computer » (`LAUNCHER_THIS_MACHINE`) : sidekicks,
  images du jeu, boutons de la manette, Match rules (« On this device »,
  « This device ») ; Miis sans Dolphin ; langue du jeu (« your device's
  language ») ; image manquante (« Tap to choose it again ») ; Display (en-
  tête, notes de format et de résolution sur l'écran entier, mesuré en
  pixels par la fenêtre du launcher : `PictureScreen`, `GameFillsScreen`) ;
  carte Display de la page Play (« Full screen ») ; niveaux sonores ;
  packs de textures : pas de glisser-déposer dans l'app, la zone « Drop a
  texture pack here » est masquée, le bouton Open montre le dossier dans
  Fichiers, où l'on met le dossier du pack.
  « How well the computer plays » (l'IA) reste ; les infobulles (survol à la
  souris) ne s'affichent pas sur iOS.
- **Guide d'accueil** (ampoule à côté de Revert, ouvert au premier
  démarrage) : `assets/launcher/welcome-ios.md`, écrit pour iPhone et iPad
  (import par « Choose file... », contrôles tactiles et leur éditeur, repli
  sur l'écran tactile, croix pour quitter, rumble, fichiers dans Fichiers,
  pas de mise à jour automatique, un Mii pour jouer en ligne). Le paquet le
  met à la place du guide de bureau : le launcher n'est pas modifié.
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
- **Zone sûre** : les contrôles sont placés entre les marges latérales de la
  zone sûre (encoche ou Dynamic Island en paysage), décalés de la marge
  gauche : dans le jeu (`Service` reçoit les marges lues par
  `SDL_GetWindowSafeArea`, converties en pixels de la surface) comme dans
  l'éditeur (`inset_left_`, `inset_right_`). Sans encoche (iPhone Duo, iPad),
  marges nulles : rien ne bouge. Les marges du haut et du bas (barre
  d'accueil) ne sont pas appliquées, pour ne pas déplacer les dispositions
  réglées. Journal : « controls inside the safe area: … ».
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
  l'intensité suit celle demandée, arrêté à 0, quand les contrôles
  tactiles se masquent (une manette prend leur place) et quand ils
  disparaissent (fin de partie). Le moteur est refait après une interruption
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

### 4.9 Réseau et mises à jour

- **Réseau** : `src/platform/http_client.cpp` passe par NSURLSession sur
  iOS (libcurl n'y existe pas) ; le fichier y est compilé en Objective-C++
  (`cmake/Http.cmake`, `-fobjc-arc`, Foundation). Mêmes règles que libcurl
  et WinHTTP : https seulement (redirections comprises), délai sans données
  (`stall_timeout`), annulation vérifiée toutes les 100 ms, taille maximale,
  un second essai d'un GET échoué avant toute donnée. Servent : recherche
  de mises à jour, joueurs en ligne, rapports de match.
- **`http` vers le serveur en ligne** : joueurs en ligne et rapports de
  match vont en `http` simple au serveur `online.server` (une adresse IP par
  défaut, modifiable), comme les requêtes WFC du jeu. iOS refuse le `http`
  par défaut (App Transport Security) ; aucune exception par domaine ne
  convient à une adresse IP réglable : `NSAllowsArbitraryLoads` dans
  `Info.plist`. Le reste reste en `https` : le client HTTP refuse le `http`
  sauf aux requêtes qui l'autorisent (`allow_http`).
- **Paquet iOS d'une version** : `recharged-<version>-ios-arm64.ipa`
  (`update_check.cpp` reconnaît `.ipa` et la plateforme `ios` ; test dans
  `tests/launcher_updates.cpp`). Les builds du fork cherchent dans les
  versions du fork (`MSCHARGED_BUILD_REPOSITORY`).
- **Affichage** : le même que sur PC. Barre de la page Play (Release/Test,
  état, joueurs en ligne), carte Updates de la page Advanced (texte iOS),
  fenêtre « UPDATE AVAILABLE » (version, notes de version). Sur iOS,
  `InstallBlocker` répond « An iOS app cannot replace itself. » : la
  fenêtre affiche « Get the new .ipa from the release page and install it
  the way you installed this one (SideStore, AltStore, Sideloadly...). »,
  et son bouton **Open release page** ouvre la page GitHub de la version
  (celle du dépôt dont vient la build : le projet d'origine pour ses
  builds, le fork pour celles du fork), qui remplace « View on GitHub ».
  Pas de lien direct vers l'`.ipa` : la page montre aussi les notes et les
  autres fichiers.
- **Mises à jour forcées** : jamais sur iOS (`EnforcedUpdates()` faux) ; une
  nouvelle version ouvre la fenêtre au démarrage. Le blocage du jeu en ligne
  d'une version officielle dépassée reste (`GatesOnline`).

- **Jeu en ligne** : le réseau du jeu lui-même (connexion au serveur,
  parties en ligne) passe par ses propres connexions (`src/net/so_host.c`),
  ni par libcurl ni par NSURLSession, et App Transport Security ne s'y
  applique pas : il marche sur iOS sans adaptation (partie en ligne entre
  l'iPhone Duo et un Mac, le 2026-11-14).

### 4.10 Code partagé modifié (neutre hors iOS)

`desktop_wpad.cpp` / `.h` (emplacement tactile −1 hors iOS ; l'emplacement
est cherché avant d'ouvrir une manette), `hardware_owner.cpp` / `.h` (joueur
tactile −1), `host_overlay.*` (dessin tactile sous garde iOS), launcher
(marges 0, `touch_scroll` désactivé, `FileUrl`), `config.*` /
`launch_options.cpp` (clés tactiles et `UserFolder` sous garde iOS),
`session_log.cpp`, `system_usage.cpp`, `hbm_debug.cpp` (`vm_region_64`),
`tools/prepare_sqlite.py` (ne retire que les variables des cibles iOS et
apparentées), `http_client.cpp` et `cmake/Http.cmake` (NSURLSession sous
garde iOS), `update_check.cpp` (paquets `.ipa` : aucun paquet de bureau ne
porte ce nom), `update_install.cpp` (`InstallBlocker` sous garde iOS).

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
- **Joueurs en ligne, mises à jour, rapports de match indisponibles** →
  libcurl absent sur iOS : NSURLSession (§ 4.9).
- **« The signing certificate used to install … was revoked »** (SideStore,
  à l'installation) → le certificat de développement du compte Apple a été
  révoqué (compte gratuit : un nouveau certificat créé par un outil révoque
  l'ancien) ; l'`.ipa` n'y est pour rien. Se déconnecter puis se
  reconnecter dans SideStore, révoquer le certificat qu'il propose, re-signer
  SideStore, puis réinstaller. Une app désinstallée perd ses Documents
  (réglages, sauvegardes, image importée).
- **Version de test du fork ignorée** → publiée en préversion : le launcher
  n'offre les préversions que comme builds de test de pull request
  (étiquettes `test-pr…`, `update_check.cpp`). Décocher « Set as a
  pre-release ».
- **Joueurs en ligne absents malgré NSURLSession** (« App Transport Security
  policy requires the use of a secure connection » dans le journal) → le
  serveur en ligne est en `http` : `NSAllowsArbitraryLoads` (§ 4.9).
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
  (`launcher/current_build.h`). Sur iOS, même une version officielle ne
  s'installe jamais seule : la fenêtre propose l'`.ipa` (§ 4.9).
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
Windows, macOS et, avec la version de `build.yml` de `ios-port`, iOS (job
`ios-arm64`, quatre modules, artefact `ios-arm64`) ; ses jobs macOS demandent des runners Apple Silicon,
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
- **iPhone 14 Plus** (encoche) : le jeu tourne (`.ipa` de `14af582`) ; les
  contrôles tactiles passaient sous l'encoche, d'où la zone sûre (§ 4.6).
- Manette **DualSense** : reconnue, le jeu répond.

## 8. Reste à faire

- Essais sur d'autres iPhone (F16), dont un modèle à 3 Go (N3) ; iPhone 14
  Plus fait.
- Essai de la notification de nouvelle version (F20, § 4.9) : publier dans
  le fork une version de test (étiquette `v1.1.8`, publiée : ni brouillon
  ni préversion, que la recherche ignore) avec un fichier `recharged-1.1.8-ios-arm64.ipa` ; au
  démarrage, l'app 1.1.7 doit ouvrir « UPDATE AVAILABLE / Version 1.1.8 »,
  et Open release page ouvrir la page de la version. Supprimer ensuite la version et son
  étiquette. Fait le 2026-11-14 : fenêtre ouverte après avoir décoché
  « pre-release » (§ 5). Côté projet d'origine : joindre l'`.ipa` aux
  versions publiées.
- Pull requests vers `main` du projet d'origine (§ 9).

## 9. Contribution au projet d'origine

Issue [#163](https://github.com/previouslevelgames/recharged/issues/163) :
le mainteneur accepte le port dans le dépôt d'origine (2026-11-13).

**Pull requests** vers `main` (pas `stable`), petites, dans cet ordre :

1. Cible de build iOS, paquet et CI : un job iOS **dans `build.yml`**, que
   les jobs de version et de build de test n'attendent pas (un build iOS en
   échec ne bloque jamais une version). Il compile les **quatre modules
   de jeu** (US, EU, JP, KR), comme les jobs Linux, Windows et macOS de
   `build.yml`. Le `ios.yml` du fork garde le seul module EU : il sert
   aux essais du fork, sur une image européenne, avec des builds plus
   courts. Un job iOS qui joint son `.ipa` aux versions publiées doit, lui,
   servir toutes les versions du jeu.
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

**Découpage retenu** (2026-11-14) : ses cinq pull requests, avec le réseau,
l'éditeur, les mises à jour et la documentation à part, et le launcher en
deux. Une pull request = un commit du fork après le regroupement, qui
compile seul ; le commit propre au fork (`IOS_PORT.md`, `ios.yml`) n'en
fait partie d'aucune.

| # | Pull request | Contenu | Après |
|---|---|---|---|
| 1 | Cible iOS, paquet, CI | CMake avec un seul test de plateforme partagé (`cmake/Platform.cmake`, fait) ; `Info.plist` ; `package_ios.py` sur la liste de `package_build.py` (`common_files`, fait) ; notices de licence ; job iOS dans `build.yml` (non bloquant, quatre modules) et `ios-attach`, qui joint l'`.ipa` à la version en brouillon ou au build de test (option B ; fait) ; correctifs de compilation (`system_usage`, `hbm_debug`, `session_log`, `prepare_sqlite.py`) ; identifiant d'app neutre (`io.github.previouslevelgames.recharged` par défaut, au mainteneur de le confirmer ; le fork garde le sien par `ios.yml` ; fait) | — |
| 2 | Dossiers, fenêtres, sortie | Documents ; fenêtres sans bordure ; paysage ; zone sûre du launcher ; défauts d'affichage ; quitter ferme l'app (« Reopen after the game » masqué) | 1 |
| 3 | Réseau | NSURLSession ; `http` vers le serveur en ligne (`NSAllowsArbitraryLoads`) | 1 |
| 4 | Contrôles tactiles | Manette virtuelle ; cœur de `touch_layout` et ses tests ; joueur « Touch screen » ; repli tactile ; croix et « Quit the game? » ; zone sûre (encoche) ; « Mouse pointer » masqué | 2 |
| 5 | Éditeur de disposition | Éditeur ; opacité | 4 |
| 6a | Import et fichiers | Sélecteur d'iOS ; liens vers Fichiers ; disque GameCube | 2 |
| 6b | Launcher tactile | Défilement au doigt ; réglages masqués ; textes adaptés ; guide d'accueil iOS | 2 |
| 7 | Vibrations | Tap ; rumble ; « Vibrate on press » ; arrêt du rumble quand les contrôles se masquent (corrigé, à essayer) | 5 |
| 8 | Mises à jour sur iOS | Paquets `.ipa` ; fenêtre avec « Open release page » ; carte Updates (l'`.ipa` joint aux versions : pull request 1, `ios-attach`) | 3 |
| 9 | Documentation | `docs/IOS.md` (nouveau : télécharger, signer et installer, premier démarrage, jeu et contrôles, mises à jour, journaux) ; `INSTALL.md` (renvoi comme pour le Steam Deck, ligne `.ipa`) ; `README.md` ; `FEATURES.md` (tactile) ; `SETTINGS.md` (clés iOS) ; `TROUBLESHOOTING.md` (app expirée, profil, journaux) | 8 |

La page d'installation demandée « en chemin » vient avec la pull request 9,
quand l'`.ipa` est publié avec les versions : la description de la pull
request 1 le dira.

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
  passent à `2704410`. 1.1.7 publiée sur `stable` (`053746f`) ; `main`
  refusionnée (`3143dbf` : clics gauche et droit réaffectables, sans conflit).
  Le mainteneur accepte le port : plan des pull
  requests au § 9. Libellés des contrôles tactiles traduits en anglais
  (« Hit », « Swap », « Quit the game? »). Guide d'accueil propre à iOS.
  Réseau par NSURLSession et notification de nouvelle version comme sur PC
  (version iOS : la page de la version, d'où l'on prend l'`.ipa`) ;
  joueurs en ligne (`http` autorisé). Contrôles hors de l'encoche (iPhone
  14 Plus). Textes du launcher adaptés à iOS. Le job iOS de la pull
  request compilera les quatre modules de jeu ; `ios.yml` garde le module EU.
- **2026-11-14 (fin)** — Jeu en ligne essayé : partie en ligne entre
  l'iPhone Duo et un Mac, sans problème. Revue de tout le portage : ce
  document n'est plus mis dans l'app (`package_ios.py`) et le code n'y
  renvoie plus (il n'ira pas dans le projet d'origine) ; un commentaire
  périmé et l'indentation d'un bloc de `touch_controls.cpp` corrigés.
  Historique de `ios-port` regroupé : `main`
  (`b815953`), puis un commit par pull request prévue (§ 9) et le commit
  propre au fork. Code final identique, octet pour octet, à l'ancien
  `d136136`, sauf une phrase du guide d'accueil (« online play needs a
  Mii », remise après l'essai en ligne) et les corrections de la revue ; chaque commit compile seul (launcher en mode iOS et Linux,
  build Linux et `ctest` complets). L'ancien historique est gardé sur
  la branche `archive/ios-port-2026-11-14`.
- **2026-11-14 (suite)** — Pull request 1 préparée dans le commit 1 : un
  seul test de système (`cmake/Platform.cmake`), `package_ios.py` sur
  `common_files()` de `package_build.py`, job iOS dans `build.yml` qui ne
  bloque aucune version (artefact hors de `recharged-*`), identifiant d'app
  neutre par défaut (le fork garde le sien par `ios.yml`).

## 11. Suivi des travaux

**Commits après le regroupement** (2026-11-14) :

| # | Pull request (§ 9) | Commit |
|---|---|---|
| 1 | Cible iOS, paquet, CI | `e728210` |
| 2 | Dossiers, fenêtres, sortie | `7861656` |
| 3 | Réseau | `15c9836` |
| 4 | Contrôles tactiles | `0395dac` |
| 5 | Éditeur de disposition | `779a703` |
| 6a | Import et fichiers | `8d7fa59` |
| 6b | Launcher tactile et guide d'accueil | `e335993` |
| 7 | Vibrations | `e9fbb75` |
| 8 | Mises à jour sur iOS | `f27e5af` |
| — | Propre au fork : `ios.yml`, ce document | le dernier |

La pull request 9 (documentation) n'a pas encore de commit. Les
identifiants cités dans les lots ci-dessous sont ceux de l'ancien
historique, sur la branche `archive/ios-port-2026-11-14`.

Un lot = un objectif, ses commits sur `ios-port`, la vérification faite
(CI : compilation ; appareil : essai sur l'iPhone Duo ou l'iPad) et son état.
Les commits des lots sont ceux de l'ancien historique (réécrit le
2026-11-11, messages renommés le même jour, regroupé le 2026-11-14 :
branche `archive/ios-port-2026-11-14`).

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
| L25 | 11-14 | Libellés des contrôles tactiles en anglais, demandé pour les pull requests : « Tacle » → « Hit », « Changer » → « Swap » (noms déjà utilisés dans `controls.touch_layout`), « Quitter le jeu ? / L'application va se fermer. » → « Quit the game? / The app will close. », Annuler / Quitter → Cancel / Quit ; éditeur du launcher et journal (« Quit: … ») | `f09c27b` | Appareil : libellés dans leurs boutons, Cancel et Quit, éditeur | Terminé |
| L26 | 11-14 | Guide d'accueil propre à iOS (`assets/launcher/welcome-ios.md`), mis par le paquet à la place du guide de bureau ; textes vérifiés contre le launcher actuel | `14af582` | Appareil : guide ouvert par l'ampoule, textes justes | Terminé |
| L27 | 11-14 | Réseau sur iOS par NSURLSession (mises à jour, joueurs en ligne, rapports de match) ; paquets `.ipa` reconnus dans les versions publiées (test) ; barre et fenêtre des mises à jour comme sur PC, avec la page de la version au lieu de l'installation ; carte Updates de la page Advanced | `383f2f8` `59c5ee7` `3042d68` | Linux : `ctest --preset debug` 61/61 ; appareil : barre des mises à jour revenue, fenêtre « UPDATE AVAILABLE / Version 1.1.8 » avec la version de test du fork, Open release page ouvre sa page (version et étiquette supprimées ensuite) | Terminé |
| L28 | 11-14 | Contrôles tactiles hors de l'encoche : placés entre les marges latérales de la zone sûre, dans le jeu et dans l'éditeur | `d8f552d` | Appareil : iPhone 14 Plus, plus rien sous l'encoche ; iPhone Duo inchangé | Terminé |
| L29 | 11-14 | Joueurs en ligne et rapports de match : `http` vers le serveur en ligne autorisé (App Transport Security, `NSAllowsArbitraryLoads`) | `4aea56c` | Appareil : compteur de joueurs affiché (iPhone Duo, iPhone 14 Plus) | Terminé |
| L30 | 11-14 | Textes du launcher adaptés à iOS : Tap, this device, Miis sans Dolphin, langue, image manquante, page Display (écran entier), niveaux sonores, packs de textures (zone de dépôt masquée, ajout par Open et Fichiers) ; textes de bureau inchangés | `7bae7f5` `e534379` | Local : launcher compilé en mode iOS et Linux, `ctest` 61/61 ; appareil : Display (écran entier), Play, Sidekicks, Match rules, Miis, Game, Gameplay (pas de zone de dépôt, Open ouvre le dossier) | Terminé |
| L31 | 11-14 | Rumble arrêté quand les contrôles tactiles se masquent : une manette connectée pendant une vibration la laissait tourner en boucle dans l'iPhone | `d136136` | À essayer : connecter la DualSense pendant une vibration | En cours |
| L32 | 11-14 | Historique regroupé : un commit par pull request prévue, plus le commit propre au fork ; ancien historique gardé sur une branche | voir le tableau ci-dessus | Arbre final identique à `d136136` (sauf la phrase des Miis du guide d'accueil et les corrections de la revue) ; chaque commit compile (iOS et Linux), `ctest` 61/61 | Terminé |
| L33 | 11-14 | Jeu en ligne sur iOS (réseau du jeu, sans changement de code) | — | Appareil : partie en ligne iPhone Duo contre Mac | Terminé |
| L34 | 11-14 | Revue du portage : `IOS_PORT.md` retiré de l'app et des commentaires du code ; commentaire des libellés et indentation dans `touch_controls.cpp` ; identifiant d'app neutre noté pour la pull request 1 | commits 1, 4 et 6a du regroupement | Chaque commit compile (iOS et Linux), `ctest` ; CI iOS sur le dernier | En cours |
| L35 | 11-14 | Pull request 1 : `cmake/Platform.cmake` (un seul test de système) ; `common_files()` partagé par `package_build.py` et `package_ios.py` ; job iOS dans `build.yml`, non bloquant, `.ipa` en artefact (hors des versions publiées jusqu'à la pull request 8) ; identifiant d'app neutre par défaut, celui du fork passé par `ios.yml` | commit 1 (`e728210`) et commit du fork | Local : `test_package_build.py` (29 tests), build Linux et `ctest` à chaque commit ; CI à `34f5891` : `ios.yml` (identifiant du fork gardé) et Build binaries, tous les jobs verts, `ios-arm64` en 11 min 30 avec le cache de `ios.yml` | Terminé |
| L36 | 11-14 | `.ipa` de `build.yml` en artefact sans zip, et job `ios-attach` (option B) : l'`.ipa` et sa ligne de `SHA256SUMS` rejoignent la version en brouillon ou le build de test, sans que rien ne l'attende | commit 1 | Local : YAML lu, logique de l'étiquette et de `SHA256SUMS` essayée ; CI : à essayer sur une version ou un build de test | En cours |
