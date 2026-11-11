# Port iOS / iPadOS — journal de bord

Document de suivi du portage de Recharged vers iPhone et iPad, tenu à jour à
chaque session de travail : **ce fichier fait foi**. Le lire en entier avant de
reprendre.

Usage strictement personnel. Le port vit sur la branche `ios-port` du fork
`zozo787/recharged`.

## Règles du projet

- **Aucune donnée de jeu dans Git, jamais.** Ni image disque (`.iso`, `.rvz`,
  `.wbfs`, …), ni DOL, ni fichiers extraits. Le `.gitignore` les exclut, le
  workflow iOS refuse de compiler s'il en trouve dans le dépôt, et
  `tools/package_ios.py` refuse de les mettre dans l'app. Le jeu est lu sur
  l'appareil, à l'exécution.
- **Commits** : même auteur que les commits existants de `ios-port` (voir
  `git log`), avec la ligne `Co-Authored-By` habituelle, sans lien de session.
- **Proposition au projet d'origine** : issue
  [previouslevelgames/recharged#163](https://github.com/previouslevelgames/recharged/issues/163)
  (« iOS and iPadOS port », 2026-11-10). Suivre la discussion avant toute
  pull request ; ne rien promettre qui n'a pas été testé.
- **Branches du projet d'origine** (`previouslevelgames/recharged`, vérifié le
  2026-11-10 dans son `CONTRIBUTING.md`) : `main` reçoit les pull requests et
  le développement ; `stable` ne reçoit que les pull requests de version (les
  releases). `ios-port` part de `stable` et la fusionne à chaque version.
  Proposer le port au projet d'origine demanderait de le rebaser sur `main`.
- Les changements restent compatibles avec Linux, macOS et Windows : le code
  propre à iOS est derrière `MSCHARGED_PLATFORM_IOS` (C++) ou `MSCHARGED_IOS`
  (CMake). Ce qui est propre à macOS garde `__APPLE__ && !MSCHARGED_PLATFORM_IOS`.

## Matériel

- Un Mac avec Xcode (compilation locale possible).
- iPad Pro M5.
- iPhone Duo (iPhone pliable, iPhone19,4, iOS 27.2.1, sorti le 23 octobre
  2026) : appareil de test principal, **joué déplié**. La pause « Dépliez pour
  continuer » au pliage a été abandonnée (pas une priorité).
- Installation : par sideload (Sideloadly).

## Version du jeu

La version testée est **européenne** (`R4QP01`) : elle utilise le module
de jeu `recharged_game_eu` (révisions 1 et 2). Pour gagner du temps, la CI ne
compile que ce module (`IOS_REGIONS=eu`, voir le workflow) ; les autres
(`us`, `jp`, `kr`, ou `all`) se choisissent au lancement manuel ou avec la
variable de dépôt `IOS_REGIONS`. Une app sans le module d'une région refuse
simplement de démarrer un disque de cette région.

## Appareils pris en charge

Le port vise **tous les iPhone et iPad capables de le faire tourner**, pas
seulement les appareils de test ci-dessus. Une seule app universelle
(`UIDeviceFamily` 1 et 2), arm64, Metal.

- **Minimum : iOS 18.0** (`CMAKE_OSX_DEPLOYMENT_TARGET` du preset
  `ios-release`, d'où l'Info.plist tire `MinimumOSVersion`). **Ne jamais**
  exporter `IPHONEOS_DEPLOYMENT_TARGET` dans l'environnement du build : le clang
  d'Apple compilerait alors pour iOS les outils exécutés sur le Mac (générateur
  de SQLite, build scripts Rust). Cela couvre tous les iPhone
  depuis les XS/XR (2018), iPhone SE 2e et 3e génération compris, jusqu'aux
  modèles actuels et à l'iPhone Duo, et les iPad qui reçoivent iOS 18. Descendre à
  iOS 16 ajouterait seulement les iPhone 8 et X (2017) au prix de restrictions
  de la bibliothèque C++ : pas prévu sauf besoin.
- **Écrans** : chaque appareil a ses proportions (16:9 des SE, ~19,5:9 avec
  encoche ou Dynamic Island, 4:3 des iPad, écran déplié de l'iPhone Duo). L'écran de
  lancement vide donne la résolution native partout ; l'image du jeu garde ses
  proportions (`display.aspect`) avec des bandes si besoin. Le launcher
  respecte les zones sûres (`SDL_GetWindowSafeArea`) ; les contrôles tactiles
  sont placés en fractions du petit côté de l'écran et se règlent dans
  l'éditeur de disposition. Petits écrans (SE) : pas encore essayés.
- **Mémoire** : les plus anciens modèles visés ont 3 Go de RAM (XR, SE 2e
  génération). iOS ferme une app qui en demande trop : mesurer l'empreinte à
  l'étape 4 sur un appareil modeste, pas seulement sur l'iPad Pro M5.
- **Le comportement propre à l'iPhone Duo ne doit jamais se déclencher sur un autre
  appareil** (voir l'étape 3).
- **Tester plusieurs modèles sans les posséder** : un preset pour le Simulateur
  iOS d'Xcode (arm64) permettrait d'essayer les tailles d'écran de tous les
  iPhone et iPad sur le Mac. **Reporté** (bien plus tard).

## Reprendre ici (état au 2026-11-10, nuit)

Base : **Recharged 1.1.6** (`84e9c7c`, `stable` du projet d'origine,
fusionnée dans `ios-port`). Builds iOS sur le runner **`xcode-27`** (SDK
iOS 27.2), module EU. Dernier `.ipa` testé : `a829838` (run 42913930522).
Proposition au projet d'origine : issue #163 (voir les règles).

**Validé sur l'iPhone Duo (joué déplié)** :
- app en plein écran natif ; launcher au doigt (défilement fluide), dans la
  zone sûre (34 points de barre d'accueil en bas, déplié) ; dossier Documents
  (réglages, image disque, journaux, visible dans Fichiers) ;
- jeu fluide (antialiasing désactivé par défaut ; 60 FPS, 120 FPS testés) ;
- contrôles tactiles (joueur 1 = « Touch screen » par défaut) : stick, A, B,
  C (objet), Z, Tacle (secouer la Wiimote), Changer (secouer le Nunchuk :
  ordre des objets), − et +, 1 (pause dans ce jeu) et 2, croix directionnelle,
  pointeur Wiimote au doigt ; masqués quand une manette physique est
  connectée ;
- **éditeur de disposition** (launcher → Controls → bouton réglages à côté de
  « Touch screen ») : déplacer, taille 50-200 %, opacité 20-100 %, Reset,
  Cancel, Save ; repris en jeu (`controls.touch_layout`,
  `controls.touch_opacity`) ;
- sortie par la croix discrète en haut à gauche → « Quitter le jeu ? » →
  Quitter ferme l'app (iOS refuse la relance ; option de relance masquée).
- iPad : presque sans problème au premier essai (avant les derniers ajouts).

**Pas de bouton HOME** : le menu HOME n'est pas compilé dans les builds
normaux du projet d'origine non plus (`MSCHARGED_DIAGNOSTIC_FRONTEND_HOME`
désactivée). À remettre s'il l'active.

**Abandonné / reporté par l'utilisateur** : pause « Dépliez pour continuer »
et pause en arrière-plan (abandonnées) ; Simulateur Xcode (« bien plus
tard »).

**Fichiers** : SDL n'a pas de boîte de dialogue sur iOS ; « Choose file... »
(et le disque GameCube) passent par le sélecteur d'iOS
(`src/launcher/ios_document_picker.mm`) qui copie l'image dans
`Documents/game` (validé) ; les boutons de dossier ouvrent l'app Fichiers.

**Pistes suivantes** : vibration à l'appui ; essais sur iPad (zone sûre) et
d'autres iPhone ; mémoire sur un appareil modeste ; retour au launcher sans
fermer l'app (difficile) ; notices de licence Rust dans l'app ; réponses aux
mainteneurs sur #163.

**Autres plateformes** : tout le tactile est compilé pour iOS seulement
(`touch_layout.cpp`, `touch_controls.cpp` ajoutés par `if(MSCHARGED_IOS)`) ;
le code partagé modifié est neutre hors iOS (`desktop_wpad.cpp`,
`hardware_owner.cpp` : joueur et emplacement tactiles à −1 ;
`host_overlay.cpp` ; launcher : marges de zone sûre à 0, `touch_scroll`
désactivé ; `config.*` : clés tactiles iOS seulement ;
`tools/prepare_sqlite.py`). Vérifié par compilation : macOS (run 42909894623,
après le tactile iOS seulement) ; Linux, Steam Deck, Windows via `build.yml`
(run 42854445316, plus ancien : à relancer avant une pull request). Non
lancé : `ctest --preset debug`, exigé par le projet d'origine.

**Avant une pull request** : repartir de `main`, découper, ajouter des tests
(`touch_layout` s'y prête), `ctest --preset debug`, traduire les libellés
français des contrôles, retirer les éléments personnels (identifiant de
bundle, runners).

## Plan et état

| Étape | Contenu | État |
|---|---|---|
| 0 | Compiler et lancer la version macOS sur le Mac (référence) | à faire par l'utilisateur |
| 1 | Cible iOS dans CMake + workflow GitHub Actions qui produit un `.ipa` non signé | **terminée** (run `42638583471`, 2026-11-09) |
| 2 | Premier démarrage (iPhone Duo, iPad) | **terminée** : plein écran natif, jeu fluide, manette DualSense reconnue |
| 3 | Contrôles tactiles, launcher tactile, arrière-plan, pliage de l'iPhone | **terminée pour l'essentiel** : tactile complet, éditeur de disposition, zones sûres ; arrière-plan et pliage abandonnés |
| 4 | Performance, finitions, installation par sideload | en partie : sideload OK (Sideloadly), 60/120 FPS ; reste mémoire sur appareil modeste, notices, autres appareils |

## Étape 1 — cible iOS et `.ipa` (session du 2026-11-09)

### État exact

**Terminée.** Le workflow `iOS build` produit
`recharged-<version>-ios-arm64.ipa` (artefact du run, gardé 14 jours) :

- exécutable arm64, `LC_BUILD_VERSION` platform IOS, minos 18.0, SDK 26.2 ;
- module `Frameworks/recharged_game_eu.so` (Mach-O bundle arm64), 514 symboles
  non définis (fonctions inachevées du décomp, liées paresseusement) ;
- `Info.plist` valide (`plutil -lint`) ; app 37 Mo, `.ipa` 18 Mo.

Durées sur `macos-15-intel` : build complet à froid (4 modules) 1 h 12 ;
avec le cache et le seul module EU, ~23 min de bout en bout.

**Rien n'a encore été lancé sur un appareil** : c'est l'étape 2.

### Ce qui a été ajouté ou modifié

- `CMakePresets.json` : preset `ios-release` (configure, build, workflow).
  Hérite de `release`, compile dans `build/ios-release`, cible
  `CMAKE_SYSTEM_NAME=iOS`, `iphoneos`, `arm64`, iOS 18.0 minimum, Rust
  `aarch64-apple-ios`, sans tests. Ne s'active que sur un Mac.
- `CMakeLists.txt` : `MSCHARGED_IOS` et la définition
  `MSCHARGED_PLATFORM_IOS=1` ; iOS accepté par la vérification de plateforme
  du runtime GX (ancienne ligne 111, qui n'acceptait que Linux/Darwin/Windows).
- `cmake/OriginalGame.cmake`, `NativeAXModuleMemory.cmake`,
  `OriginalNativeCompilerProfile.cmake` : iOS admis comme macOS.
- `cmake/OriginalSourceWarnings.cmake`, `AppIcon.cmake`, `AuroraHost.cmake`,
  `MacOSLinkLibraries.cmake` : les réglages AppleClang de macOS valent aussi
  pour iOS.
- `cmake/SystemUsage.cmake`, `src/platform/system_usage.cpp` : pas d'IOKit sur
  iOS (en-têtes absents du SDK) ; seule la charge processeur est mesurée.
- `src/platform/hbm_debug.cpp` : `vm_region_64` au lieu de `mach_vm_region`
  sur iOS (`mach/mach_vm.h` n'est pas dans le SDK iOS).
- `cmake/Launcher.cmake` : sur iOS, pas de `Recharged.app` façon macOS ni de
  `macos_app.cpp` (quarantaine, `/usr/bin/open`) ; génère
  `generated/mscharged/Info-iOS.plist` depuis `cmake/Info-iOS.plist.in`.
- `src/launcher/main.cpp` : `macos_app` exclu sur iOS ; le paquet s'identifie
  `ios-arm64` (aucune mise à jour publiée ne correspond, donc aucune
  auto-update ne s'installe).
- `src/runtime/original_game.cpp` : sur iOS les modules de jeu sont cherchés
  dans `Recharged.app/Frameworks/`.
- `cmake/Info-iOS.plist.in` : iPhone + iPad, paysage uniquement, plein écran
  (`UIRequiresFullScreen`, retiré puis remis, voir le journal), écran de lancement vide (résolution native), barre
  d'état masquée, partage de fichiers (dossier Documents visible dans
  Fichiers), manettes (`GCSupportedGameControllers`), ProMotion.
- `tools/package_ios.py` : assemble `Recharged.app` et
  `recharged-<version>-ios-arm64.ipa` (zip de `Payload/`) sans signature,
  vérifie que les binaires sont arm64 pour iOS (`lipo`, `otool`), redimensionne
  l'icône (`sips`), ajoute les clés DT* du SDK, copie les notices de licence.
- `.github/workflows/ios.yml` : runner `macos-15-intel` par défaut (voir le journal), Xcode 26.3, sur push et PR
  vers `ios-port` et à la demande. Refuse les données de jeu commitées, compile
  avec ccache, empaquette, contrôle l'app, publie l'`.ipa` en artefact
  (14 jours).
- `.gitignore` : `*.ipa`.

### Compiler sur le Mac

```sh
brew install cmake ninja python rust tcl-tk
export PATH="$(brew --prefix tcl-tk)/bin:$PATH"
rustup target add aarch64-apple-ios   # si Rust vient de rustup
python3 tools/setup_dependencies.py
cmake --preset ios-release --fresh
cmake --build --preset ios-release --target mscharged mscharged_original_frontend_module_eu
python3 tools/package_ios.py --regions eu
```

Sans `--target` ni `--regions`, les quatre modules sont compilés et empaquetés.
L'`.ipa` arrive dans `build/ios-artifacts/`. Avec Rust installé par Homebrew,
la cible iOS manque : utiliser rustup (`brew install rustup`).

### Installer l'`.ipa` (rappel, détaillé à l'étape 4)

L'`.ipa` n'est pas signé. Il faut le signer avec son identifiant Apple au
moment de l'installer : Sideloadly ou AltStore (compte gratuit : 7 jours,
3 apps), ou Xcode. Le Mode développeur doit être activé sur l'appareil
(Réglages → Confidentialité et sécurité).

### Décisions

- **Modules de jeu chargés dynamiquement, gardés tels quels.** Il y a un module
  par version du jeu (US, EU, JP, KR) avec les mêmes symboles : impossible de
  les lier tous les quatre dans l'exécutable. iOS autorise `dlopen` d'un
  binaire signé qui fait partie de l'app (c'est interdit seulement pour du code
  téléchargé ou généré). Ils restent des `MODULE` CMake (MH_BUNDLE, comme les
  bundles XCTest chargés sur iOS) et sont rangés dans `Frameworks/`, où les
  outils de sideload signent les binaires. Pas de vrai `dlclose` à attendre :
  un seul module est chargé par session.
- **iOS 18.0 minimum** : les appareils visés sont sous iOS 26 ; 18.0 couvre
  la bibliothèque C++ dont Aurora a besoin (`std::jthread`) sans contrainte.
- **Packager séparé** (`tools/package_ios.py`) plutôt que d'étendre
  `package_build.py`, qui est lié au format des archives desktop et à
  l'auto-update.
- **Identifiant** `io.github.zozo787.recharged` (variable CMake
  `MSCHARGED_IOS_BUNDLE_IDENTIFIER`) ; les outils de sideload peuvent le
  remplacer.
- **Pas de JIT** : tout est du code natif compilé à l'avance.

### Risques à vérifier au premier run de la CI

1. `-undefined dynamic_lookup` (modules de jeu, `cmake/OriginalGame.cmake`) :
   **simple avertissement** (« deprecated on iOS ») au build du 2026-11-09, les
   modules se lient. Reste à vérifier au lancement sur l'appareil (point 5).
2. Dawn et Aurora pour iOS : **compilent** (build du 2026-11-09) ; le rendu
   reste à vérifier sur l'appareil.
3. Corrosion + nod pour `aarch64-apple-ios` : **compile**. Les dépendances C
   des crates (cc-rs) visaient la version du SDK (iOS 26.2) ; Cargo reçoit
   maintenant `IPHONEOS_DEPLOYMENT_TARGET` via `corrosion_set_env_vars`
   (`CMakeLists.txt`, `mscharged_add_nod`).
4. API absentes du SDK iOS qui n'auraient pas été repérées à la lecture : les
   corriger au cas par cas derrière `MSCHARGED_PLATFORM_IOS`.
5. Au lancement (étape 2) : `dlopen` du module signé par l'outil de sideload, et
   liaison paresseuse des fonctions inachevées du décomp sur iOS.

## Étape 2 — premier démarrage (terminée)

### Constaté sur l'appareil (2026-11-09, iPhone Duo)

- L'`.ipa` non signé s'installe après signature avec un identifiant Apple
  (Mode développeur activé, profil approuvé dans Réglages →
  Général → VPN et gestion de l'appareil).
- **L'app démarre, ne plante pas, les menus du launcher se parcourent.**
- **Le launcher ne remplissait pas l'écran** (capture, iPhone Duo déplié) : il
  ouvrait une « fenêtre » de bureau (1120×720 dp ajustée avec des marges,
  `ApplyWindowMetrics`) centrée dans un grand cadre noir. Aussi : « Couldn't
  check for updates » et le choix Release/Test affichés, et « Cannot write
  configuration » (app en lecture seule, corrigé par `UserFolder`). Non testé
  sur d'autres iPhone ni sur l'iPad.

### Fait

- **Dossiers inscriptibles → Documents.** `UserFolder()`
  (`src/bootstrap/config.cpp`) donne `$HOME/Documents` sur iOS, le dossier du
  programme ailleurs. `recharged.ini` (`DefaultConfigPath`), `data/`
  (sauvegardes `nand/`, cache de shaders, données du launcher), `logs/` (journal
  de session, enregistrements de performances) y vont ; le programme se place
  dans Documents au démarrage (captures d'écran, chemins relatifs). Les
  ressources (`assets/`, modules) restent lues dans l'app. Documents est visible
  dans l'app Fichiers : « Sur mon iPhone/iPad » → « Recharged ».
- **Import du jeu** : copier son image disque (`.rvz`, `.iso`,
  `.wbfs`) dans ce dossier, ou dans son sous-dossier `game/` ; la recherche de
  disque du premier démarrage (`src/launcher/disc_search.cpp`) parcourt déjà le
  dossier des réglages et `game/`.

- **Fenêtres** : sur iOS, launcher et jeu en fenêtre **sans bordure** (une
  fenêtre iOS couvre toujours l'écran et suit ses rotations ; sans bordure
  masque la barre d'état). Ni `SDL_WINDOW_FULLSCREEN` (il fige la fenêtre au
  mode d'affichage lu au démarrage), ni réglage de taille propre à iOS : les
  essais faits avant le SDK 27 contre le cadre de compatibilité ont été
  retirés (voir le journal du 2026-11-10).
- **Mises à jour masquées** : la barre de mise à jour (état, Release/Test)
  n'est plus dessinée sur iOS.

### Constaté sur l'appareil (2026-11-09, build `42690545970`, iPhone Duo)

- **Détection de l'image disque OK, lancement du jeu OK** (module EU chargé
  par `dlopen` depuis `Frameworks/`, rendu Metal) : les risques 1 et 5 de
  l'étape 1 sont levés.
- **Pas de commandes** : aucune entrée n'atteint le jeu sans manette (le
  tactile en jeu est l'étape 3). À essayer avec une manette Bluetooth.
- **L'écran n'est toujours pas rempli** sur l'iPhone Duo (launcher et/ou jeu,
  capture demandée). Hypothèse principale : l'app est liée au SDK iOS 26.2
  (Xcode 26.3 du runner), antérieur à l'iPhone Duo (sorti le 23/10/2026) ; iOS
  peut afficher une app liée à un SDK plus ancien que l'appareil dans une zone
  de compatibilité, comme lors des précédents nouveaux formats d'écran. À
  confirmer en compilant avec le Xcode le plus récent (sur un Mac, ou un
  runner qui l'a). Autre cause possible pour le jeu
  seul : les bandes normales d'une image 16:9 ou 4:3 sur un écran déplié plus
  carré (`display.aspect`).

### Reste à faire (au 2026-11-09 ; tout est fait ou abandonné depuis, voir « Reprendre ici »)

- **Zones sûres** (encoche, Dynamic Island, coins arrondis, barre d'accueil) :
  la mise en page du launcher part de (0, 0) ; la décaler dans
  `SDL_GetWindowSafeArea` touche `Draw`, `DrawSidebar`, `DrawMain` et le
  dessin SDL de l'illustration. À faire après avoir vu le plein écran sur
  l'appareil.
- Textes propres au PC (« NATIVE PC PORT », « run natively on your PC ») :
  cosmétique.
- Réglages sans effet sur iOS, à masquer dans le launcher : plein écran /
  fenêtré, taille de fenêtre, résolution plein écran, mémoriser la fenêtre,
  écran (display), carte graphique, fenêtre de journal, mises à jour (le jeu
  force une fenêtre sans bordure couvrant l'écran, `original_game.cpp`).
- `src/platform/launcher_reopen.cpp:96` relance le processus avec `execv`
  (interdit) : sur iOS, revenir au launcher dans le même processus, ou quitter
  proprement.
- `src/platform/log_window.cpp` et l'option « Log window » : `posix_spawn` de
  `/usr/bin/open`, inexistant ; masquer l'option sur iOS.
- Auto-update (`src/launcher/update_*.cpp`) : masquer l'interface sur iOS
  (rien ne s'installe déjà, faute de paquet `ios-arm64`) ; libcurl n'existe
  pas sur iOS, donc aucune requête HTTP ne part.
- Manette Bluetooth via SDL3 (GameController). Les vraies Wiimotes sont
  impossibles : Bluetooth classique inaccessible aux apps.
- Statut Discord : sans objet sur iOS.

## Étape 3 — tactile, arrière-plan, pliage (notes de conception ; état à jour dans « Reprendre ici » et le journal)

- **Contrôles tactiles en jeu — première version** (`TouchControls`,
  `src/runtime/touch_controls.{h,cpp}`, iOS seulement). Une **manette virtuelle
  SDL** (`SDL_AttachVirtualJoystick`) que le jeu prend comme n'importe quelle
  manette (`desktop_wpad.cpp`, disposition par défaut de
  `bootstrap/gamepad_actions.h`) : rien à changer dans les entrées du jeu.
  Dessin dans l'overlay existant (`HostOverlay`, ImGui d'Aurora), redessiné
  seulement quand l'état change. Disposition (tailles relatives au petit côté
  de l'écran) : stick flottant à gauche (il se centre là où le doigt se pose),
  mini croix directionnelle en haut à gauche, à droite A (gros), B, Z, C,
  « Tacle » (secouer la Wiimote = gâchette droite), « Objet » (secouer le
  Nunchuk = RB), « + » en haut au centre. Plusieurs doigts à la fois, glisser
  d'un bouton à l'autre. Pendant le jeu, les touchers ne produisent plus de
  clics souris (`SDL_HINT_TOUCH_MOUSE_EVENTS=0`, rétabli en revenant au
  launcher), sinon la « Wiimote clavier-souris » les lirait aussi. Masqués,
  manette virtuelle retirée, dès qu'une vraie manette est connectée. Limites
  connues : une manette branchée en cours de partie peut devenir joueur 2
  (la virtuelle avait pris le joueur 1) ; pas de pointeur Wii au doigt ; pas
  encore de prise en compte des zones sûres ; libellés en ASCII (police de
  l'overlay).
- **Fait (2026-11-10), à tester : « Écran tactile » comme choix de
  joueur.** Ajouter `touch` aux valeurs de `controls.player1-4`
  (`src/bootstrap/config.cpp`), à la page Controls du launcher, et à
  l'attribution des joueurs (`NativePlayers`, `hardware_owner.cpp`,
  `desktop_wpad.cpp` : canal fixe pour l'identifiant SDL de la manette
  virtuelle de `TouchControls`). Défaut iOS : joueur 1 = écran tactile.
  Supprimer alors le masquage automatique quand une manette est connectée.
  À faire **après** avoir vérifié que la manette virtuelle est bien lue
  (build `42822490589`).
- Contrôles tactiles superposés au jeu, launcher ImGui utilisable au doigt
  (tailles de cibles, défilement, clavier virtuel).
  - **Fait : défilement au doigt** dans le launcher
    (`src/launcher/touch_scroll.{h,cpp}`, iOS seulement). ImGui ne connaît
    que la souris, et SDL transforme chaque toucher en clic-glisser : au-delà
    de 10 points de déplacement surtout vertical au-dessus d'une fenêtre qui
    défile, le clic est annulé et le doigt fait défiler cette fenêtre, avec de
    l'inertie au relâcher ; un toucher pendant l'inertie l'arrête sans cliquer.
    Les glissements horizontaux (curseurs) et les touchers simples ne changent
    pas.
- **Arrière-plan** : à `SDL_EVENT_WILL_ENTER_BACKGROUND`, mettre le jeu en
  pause, couper l'audio, arrêter de présenter des images Metal ; reprendre à
  `SDL_EVENT_DID_ENTER_FOREGROUND`. Sauvegarder les réglages en arrière-plan
  (iOS peut tuer l'app sans prévenir).
- **iPhone Duo** : je ne connais pas l'API d'Apple pour cet appareil (sorti
  après mes connaissances). Plan : détecter le pliage par le changement de
  taille et de proportions de la fenêtre/scène (événements SDL
  `SDL_EVENT_WINDOW_RESIZED` / `PIXEL_SIZE_CHANGED`, ou la géométrie de la
  `UIWindowScene`) ; utiliser une API dédiée si le SDK d'Xcode en expose une.
  Plié : même pause que l'arrière-plan (simulation figée, audio coupé, état
  conservé en mémoire) avec un écran « Dépliez pour continuer » ; déplié :
  reprise à l'identique. Le seuil de détection devra être réglé sur l'appareil.
  **Uniquement sur un appareil pliable** : sur un iPhone ou un iPad ordinaire,
  un changement de taille de fenêtre (rotation paysage gauche/droite, Stage
  Manager sur iPad) ne doit jamais afficher ce message. La détection doit donc
  d'abord établir que l'appareil est pliable (API du SDK si elle existe, sinon
  identifiant du modèle), avec un réglage pour la désactiver.

## Étape 4 — performance et finitions (à faire)

- **Vérifier les autres plateformes** : fait (voir « Reprendre ici ») ;
  relancer `build.yml` et `macos-check.yml` avant une pull request.

- Mesures sur iPad Pro M5 et iPhone ; réglages de résolution par défaut.
- Signature et installation : procédure Sideloadly/AltStore/Xcode, durée de
  validité du certificat gratuit.
- Notices de licence des crates Rust dans l'app (le packager desktop les
  collecte ; `package_ios.py` ne copie pour l'instant que `Cargo.lock`).

## Journal

- **2026-11-09** — Étape 1 écrite : preset `ios-release`, gardes iOS dans
  CMake et dans le code, `Info-iOS.plist`, `tools/package_ios.py`, workflow
  `.github/workflows/ios.yml`. Non compilé : pas de SDK iOS dans la session et
  push vers GitHub refusé (accès de la session au dépôt pas encore réglé). Prochaine action : pousser `ios-port`, lancer le
  workflow, corriger les erreurs de compilation.
- **2026-11-09** — Push réussi (l'accès GitHub a été réglé), premier run du
  workflow lancé. Exigence ajoutée : compatibilité avec tous les iPhone et iPad
  sous iOS 18+, pas seulement les appareils de test (section « Appareils pris
  en charge ») ; le comportement propre au pliage est limité aux appareils pliables.
- **2026-11-09** — Deux runs sur `macos-15` (Apple Silicon) abandonnés par
  GitHub sans démarrer : « not acquired by Runner … capacity constraints ».
  Le workflow tourne désormais par défaut sur `macos-15-intel` (même app arm64
  pour iOS, compilation plus lente, délai porté à 330 min) ; choix du runner au
  lancement manuel, ou variable de dépôt `IOS_RUNNER=macos-15` pour revenir à
  Apple Silicon sur les push.
- **2026-11-09** — Premier run réellement exécuté (`macos-15-intel`) : SDL3
  (UIKit, Metal, MFi), Dawn et les compilateurs iOS se configurent ; échec dans
  la génération de SQLite. Cause : `IPHONEOS_DEPLOYMENT_TARGET` défini pour tout
  le job faisait compiler pour iOS les outils exécutés sur le Mac. Corrigé :
  variable retirée du workflow, et `tools/prepare_sqlite.py` retire
  `*_DEPLOYMENT_TARGET` et `SDKROOT` de l'environnement de ses outils.
- **2026-11-09** — Le premier build complet est long (Dawn + le jeu compilé
  quatre fois, à froid, sur 3 cœurs Intel). La version testée est
  européenne : la CI ne compile plus que le module EU par défaut
  (`IOS_REGIONS`, option `--regions` de `tools/package_ios.py`). Un nouveau
  push n'annule plus le build en cours (`cancel-in-progress: false`) : il attend
  et profite du cache qu'il laisse.

- **2026-11-09** — Run `42626867778` : **tout compile et se lie pour iOS**
  (exécutable, Dawn, Aurora, nod, les quatre modules). Échec à l'empaquetage :
  CMake faisait par défaut de l'exécutable un bundle `recharged.app` sur iOS,
  le packager ne trouvait pas `build/ios-release/recharged`. Corrigé
  (`MACOSX_BUNDLE OFF`). Corrigé aussi : cible iOS 26.2 au lieu de 18.0 pour le
  C de nod.
- **2026-11-09** — **Étape 1 terminée** : run `42638583471` vert, premier
  `.ipa` (module EU) publié en artefact. Prochaine action (étape 2) : signer et
  installer l'`.ipa` sur l'iPad Pro M5, le lancer avec une manette Bluetooth,
  récupérer le journal ; puis rediriger les dossiers inscriptibles vers
  Documents (voir « Étape 2 »). Le workflow ignore désormais les push qui ne
  touchent que la documentation.
- **2026-11-09** — Étape 2 commencée. Sur l'iPhone Duo, l'app s'installe,
  démarre sans planter et le launcher se parcourt ; l'affichage n'est pas adapté
  à l'écran de l'iPhone Duo (détails demandés). Réglages, données, sauvegardes et
  journaux redirigés vers Documents (`UserFolder`), visibles dans Fichiers.
- **2026-11-09** — Capture du launcher sur l'iPhone Duo déplié : fenêtre de
  bureau centrée dans un cadre noir. Launcher et jeu passés en plein écran sur
  iOS ; barre de mise à jour masquée. Zones sûres à traiter ensuite.
- **2026-11-09** — Sur l'iPhone Duo : disque détecté, **jeu lancé**. Pas de
  commandes (pas de manette, pas encore de tactile en jeu). L'écran n'est pas
  rempli : hypothèse du SDK 26.2 antérieur à l'iPhone Duo, à vérifier.
- **2026-11-09** — Captures du launcher et du jeu sur l'iPhone Duo : **les deux
  sont dans le même cadre 16:9 aux coins arrondis, avec un bouton ⤢ en bas à
  droite** : c'est iOS qui exécute l'app en mode de compatibilité, pas la mise
  en page. Causes possibles : `UIRequiresFullScreen` (déprécié depuis
  iOS/iPadOS 26) ou le SDK 26.2 antérieur à l'iPhone Duo. Premier essai :
  `UIRequiresFullScreen` retiré de l'Info.plist. Second essai si besoin :
  compiler avec le Xcode le plus récent. Une manette DualSense sert aux
  essais des commandes.
- **2026-11-09** — Accélération de la CI. Sur un build avec cache ccache
  (~23 min) : configuration 7 min (surtout la préparation des sources patchées
  et la génération de SQLite, refaites à chaque fois), compilation 9 min (dont
  le Rust de nod, jamais en cache), 3 jobs pour 4 cœurs Intel. Ajouté : cache
  des sources préparées et de SQLite (clé exacte : submodules épinglés,
  patchs, outils de préparation), cache Cargo (`~/.cargo/registry`,
  `build/ios-release/cargo`), un job par cœur (`hw.ncpu`). Le premier run
  remplit ces caches ; les suivants devraient être nettement plus courts.
- **2026-11-09** — Retour de l'utilisateur : les menus du launcher ne défilent
  pas au doigt. Ajouté : défilement tactile avec inertie (`TouchScroll`).
- **2026-11-09** — Premier build avec tous les caches : 20 min 30 (contre
  ~23-25). Cargo ne recompile plus rien, la configuration passe de ~7 à ~3,5
  min (+1,5 min de restauration). Les sources Objective-C++ Metal de Dawn
  étaient recompilées à chaque fois (~3 min) : ccache ajouté pour OBJC/OBJCXX.
  Reste incompressible : la vérification des sources préparées de Dawn
  (~1,5 min) et l'installation des outils (~3 min).
- **2026-11-09** — Retours sur l'iPhone Duo : défilement « presque naturel »,
  léger saccadement ; **cadre de l'iPhone Duo toujours là** malgré le retrait de
  `UIRequiresFullScreen` ; **DualSense reconnue, le jeu répond** ; baisse de
  FPS ressentie malgré des réglages, sans échauffement. Xcode 27.2 existe
  (le runner : 26.3, SDK 26.2) : la piste du SDK se renforce. Changé :
  le workflow prend par défaut le **Xcode le plus récent** du runner (liste
  affichée ; input `xcode`, variable `IOS_XCODE`) et propose `macos-26` ; le
  launcher n'ajoute plus d'attente pour 60 Hz sur iOS (vsync seul), cause
  probable du saccadement sur un écran 120 Hz.
- **2026-11-09** — Le runner `macos-15-intel` n'a que des Xcode 16.x et 26.0
  à 26.3 (SDK iOS 26.2 au mieux) : le build `42721491215` (saccadement corrigé)
  ne peut pas régler le cadre de l'iPhone Duo. Lancé à la main sur `macos-26`
  (Apple Silicon, Xcode le plus récent de l'image) : run `42723591587`, à
  froid pour cette architecture (caches séparés par `runner.arch`).
- **2026-11-10** — Run `42723591587` sur `macos-26` réussi (33 min à froid,
  Apple Silicon) : Xcode **26.6**, SDK iOS **26.5**, le plus récent de
  GitHub ; pas de Xcode 27 sur les runners. Si le cadre de l'iPhone Duo reste avec
  cet `.ipa`, il faudra le SDK 27 : compiler sur un Mac avec Xcode 27.2, à la
  main ou via un runner GitHub auto-hébergé.
- **2026-11-10** — En pause (installation plus tard). Section « Reprendre
  ici » ajoutée en tête du document.
- **2026-11-10** — SDK 26.5 : cadre de l'iPhone Duo toujours là (il faut le SDK 27).
  Défilement : la fenêtre racine du launcher défilait au doigt (menu de gauche
  compris) ; `TouchScroll` respecte maintenant `NoScrollWithMouse` comme la
  molette. Ajouté : contrôles tactiles en jeu (manette virtuelle SDL).
- **2026-11-10** — Contrôles tactiles : **compilent** (runs `42800411445`
  Intel et `42801493501` macos-26, à faire essayer). L'utilisateur indique
  que GitHub propose Xcode 27 : le workflow accepte désormais n'importe quelle
  étiquette de runner au lancement manuel (file par runner). Constat : sur
  `macos-26`, toujours Xcode 26.6 (SDK 26.5) au plus récent ; `macos-27`
  (run `42801499524`) reste en file d'attente, étiquette peut-être inexistante.
  Demander à l'utilisateur le nom exact de l'image GitHub avec Xcode 27.
- **2026-11-10** — Changelog GitHub du 2026-09-10 : l'image **Xcode 27** a
  pour étiquettes `runs-on` **`xcode-27`** (et `xcode-27-xlarge`, runners
  payants), tourne sur macOS 27, arm64 seulement, en préversion publique. Les
  essais `macos-27` (étiquette inexistante, resté en file) et `macos-latest`
  annulés ; build lancé sur `xcode-27` : run `42804838824`. Si l'`.ipa`
  supprime le cadre de l'iPhone Duo, faire de `xcode-27` le runner par défaut.
  (`github.blog` ajouté aux domaines autorisés de l'environnement.)
- **2026-11-10** — Run `42804838824` sur `xcode-27` **réussi** (29 min à
  froid) : Xcode **27.2**, SDK iOS **27.2**. Le code
  compile sans changement avec ce SDK. `.ipa` :
  `recharged-1.1.5+g16ca2ab3c10b-ios-arm64.ipa` (avec contrôles tactiles). À
  faire vérifier : le cadre de l'iPhone Duo disparaît-il ? Si oui, `xcode-27`
  devient le runner par défaut.
- **2026-11-10** — Avec le SDK 27 (`xcode-27`) **le cadre de compatibilité a
  disparu**. Nouveau défaut sur l'iPhone Duo déplié : l'image n'occupe que
  1407 px de large sur 2000 et semble coupée en bas, comme une fenêtre en
  portrait (1407×2000) sur un écran en paysage. Cause probable :
  `SDL_WINDOW_FULLSCREEN`, que le cœur de SDL cale sur le mode d'affichage lu
  au démarrage. Corrigé (iOS) : launcher et jeu en fenêtre **sans bordure**
  (pleine taille sur iOS, suit les rotations, barre d'état masquée), forme de
  fenêtre du jeu non verrouillée, `SDL_HINT_ORIENTATIONS` paysage ; tailles de
  fenêtre/écran/orientation écrites dans le journal à chaque changement
  (« Screen (...) »). Runner par défaut : `xcode-27`. Le réglage d'image du
  jeu est revenu par défaut sur l'appareil (« No image chosen ») : la
  réinstallation a probablement vidé Documents.
- **2026-11-10** — À la demande de l'utilisateur, retrait des adaptations
  d'écran essayées avant le SDK 27 (le vrai remède au cadre était le SDK) :
  `UIRequiresFullScreen` remis dans l'Info.plist, le launcher refait sur iOS
  le même dimensionnement que sur ordinateur (`ApplyWindowMetrics`, sans
  effet sur iOS où SDL ignore les tailles demandées). Gardé : fenêtres sans
  bordure (correctif de la troncature vue avec le SDK 27), barre de mise à
  jour masquée, défilement tactile, rythme vsync.
- **2026-11-10** — Build `42812202827` (SDK 27.2) : **le launcher remplit
  l'écran de l'iPhone Duo, plus de cadre.** En jeu : **contrôles tactiles sans
  effet** et **forte lenteur**. Vérifié : Aurora crée sa vue Metal par
  `SDL_Metal_CreateView` (les touchers passent par SDL) ; les filtres
  d'événements du jeu (`hotkey_filter`, `friend_code_keys`) ne retiennent que
  des touches clavier. À noter : Aurora crée la fenêtre du jeu en
  `SDL_WINDOW_FULLSCREEN` sur iOS (`extern/aurora/lib/window.cpp`) ; le jeu la
  repasse sans bordure. Ajouté au journal (`Touch controls: ...`) : création
  et type de la manette virtuelle, surface, affichage ou masquage (et par
  quelle manette physique), premier toucher, premier appui de chaque bouton,
  redimensionnements de la fenêtre du jeu. Demandé à l'utilisateur : les
  boutons sont-ils visibles et s'éclairent-ils, la DualSense était-elle
  allumée, le journal de session, et l'overlay de performances.
- **2026-11-10** — Journal et `frame-times` de l'utilisateur (iPhone Duo,
  build `42816723872`) :
  - **Contrôles tactiles** : touchers reçus, manette virtuelle créée, chaque
    bouton appuyé — mais le jeu ne la lisait pas. Cause : au démarrage,
    `platform/hardware_owner.cpp` compte les manettes connectées ; s'il n'y en
    a aucune, il désactive les manettes pour toute la partie (« keyboard &
    mouse is player 1 »). La manette virtuelle n'était créée qu'à la première
    image, après ce comptage. Corrigé : elle est créée dès la construction de
    `TouchControls`, avant le comptage (s'il n'y a pas de manette physique).
  - **Lenteur** : 60 FPS les 30 premières secondes, puis 30 FPS stables
    (33,4 ms par image, 2 rafraîchissements par image) avec seulement 9,4 ms
    de CPU du jeu par image : **processeur graphique saturé**. Réglages par
    défaut : rendu 3D `window` (2853×2007 sur l'écran intérieur, ~4,5× la Wii)
    et antialiasing `4x` ; replié (écran extérieur 2034×1398) le jeu redevient
    fluide. Défauts iOS changés : `display.resolution=3x`,
    `display.antialiasing=fxaa`. Le launcher écrit tous les réglages dans
    `recharged.ini` : une installation existante garde les anciens, à changer
    à la main (page Display).
  - Deux écrans vus par SDL sur l'iPhone Duo : « Display 1 » 466×678 (extérieur)
    et « Display 2 » 951×669 (intérieur), 120 Hz, densité 3 ; la fenêtre suit
    le pliage (pour la pause « Dépliez pour continuer » de l'étape 3).
- **2026-11-10** — Test de l'utilisateur : **fluide en désactivant
  l'antialiasing**, résolution 3D laissée sur « Full » (`window`). Le coût
  venait du multiéchantillonnage 4x sur ~5,7 Mpx. Défauts iOS corrigés :
  `display.resolution=window`, `display.antialiasing=off` (au lieu de 3x/fxaa,
  non testés).
- **2026-11-10** — L'utilisateur a essayé plein écran puis fenêtré : aucune
  différence, normal (forcé sur iOS) ; ces réglages sont à masquer sur iOS.
  FPS de la cinématique inchangé : le compteur suit la cadence des vidéos du
  jeu ; reste à savoir si l'image et le son saccadent encore sans
  antialiasing.
- **2026-11-10** — Sans antialiasing, **la cinématique est fluide** (image et
  son), et le réglage de fréquence d'images **120 FPS est stable** sur le
  iPhone Duo déplié (écran 120 Hz). Défaut laissé à `original` (60 FPS, la
  cadence du jeu) ; 120 FPS reste un choix de l'utilisateur.
- **2026-11-10** — Build `42820375168` : contrôles tactiles toujours sans
  effet. Le journal montre la Wiimote clavier-souris en instance 9 et la
  manette virtuelle en 10 : le comptage des manettes
  (`InitializeNativeHardwareInput`, `original_game.cpp`) avait lieu **avant** la
  construction de `TouchControls` (créée avec l'overlay, bien plus loin) ;
  l'ordre des lignes d'Aurora dans le journal trompait (il les écrit en
  traitant les événements). Corrigé : `TouchControls` est créé juste avant
  `InitializeNativeHardwareInput` et branché sur l'overlay ensuite ; il tient
  sa propre référence au sous-système manettes de SDL (le comptage l'ouvre et
  le referme). L'utilisateur renonce au nettoyage des caches.
- **2026-11-10** — Proposition de l'utilisateur, acceptée : choisir
  « Écran tactile » dans les réglages des joueurs (voir « Étape 3 »), après
  le test du build `42822490589`.
- **2026-11-10** — Build `42822490589` : la manette virtuelle est bien créée
  avant le comptage (instance 9, Wiimote clavier-souris en 10), mais toujours
  aucun effet. **Vraie cause** : `controls.player1-4` vaut par défaut
  `keyboard, off, off, off` (`Settings::players`, `src/bootstrap/config.h`) ;
  sans manette déclarée comme joueur, le jeu n'ouvre aucune manette
  (`desktop.gamepads` faux dans `hardware_owner.cpp`). La DualSense marchait
  quand le joueur 1 était réglé sur `gamepad1`. Test demandé : Controls →
  Players → joueur 1 = Gamepad 1. Cela valide le plan « Écran tactile » comme
  choix de joueur (défaut iOS : joueur 1 = écran tactile).
- **2026-11-10** — **« Touch screen » est un choix de joueur** (iOS) :
  `controls.playerN = touch` (`config.cpp`, défaut iOS : joueur 1),
  option dans la carte Players du launcher. Au démarrage, `TouchControls`
  n'est créé que si un joueur l'a choisi, avant `InitializeNativeHardwareInput` ;
  `NativePlayers::touch`/`touch_id` donnent à ce joueur un emplacement de
  manette réservé (`DesktopWpadSettings::touch_slot`, le plus haut libre) que
  seule la manette virtuelle (par son identifiant SDL) peut prendre ; elle
  n'est jamais comptée parmi les manettes ni retirée comme absente. Plus de
  masquage automatique quand une vraie manette se connecte. Une installation
  existante garde `player1 = keyboard` : choisir « Touch screen » à la main.
- **2026-11-10** — À la demande de l'utilisateur (sur téléphone ou tablette,
  brancher une manette = jouer seul avec), **retour du masquage automatique**,
  en mieux : une manette qui se connecte et n'a pas d'emplacement à elle
  **prend l'emplacement de l'écran tactile** (`desktop_wpad.cpp` retire la
  manette virtuelle de cet emplacement) et joue à sa place ; les boutons
  tactiles se masquent et relâchent tout. Quand la manette part, la manette
  virtuelle reprend l'emplacement et les boutons reviennent. La manette
  virtuelle reste attachée pendant ce temps : son identifiant SDL, qui
  désigne l'emplacement réservé, ne change pas.
- **2026-11-10** — **Les contrôles tactiles répondent** (joueur 1 = Touch
  screen) : cinématique passée. Manquait le **pointeur de la Wiimote** pour
  les menus. Ajouté : un doigt posé hors des boutons et hors du coin du stick
  (quart inférieur gauche, 40 % × 55 %) pointe ; `TouchControls` l'envoie au
  jeu comme une souris (événements `SDL_EVENT_MOUSE_MOTION`, identifiant
  propre, pas `SDL_TOUCH_MOUSEID` que `desktop_wpad` ignore), à chaque image
  tant qu'il est posé ; A valide. `hardware_owner.cpp` active la souris pour
  le joueur 1 quand l'écran tactile est ce joueur et joue seul.
- **2026-11-10** — **Sortir du jeu** : bouton **HOME** ajouté aux contrôles
  tactiles (bouton Guide de la manette virtuelle = HOME de la Wiimote) : il
  ouvre le menu HOME de la Wii, dont « Menu Wii » éteint le jeu (chemin
  existant : `ArmNativeHardwareWindowClose`/`ConfigureNativeSTMPowerRemoval`,
  puis `ReopenLauncher`). `launcher.reopen_after_game` vaut désormais `true`
  par défaut sur iOS. Non vérifié : `ReopenLauncher` relance par `execv`
  (`launcher_reopen.cpp`) ; si iOS le refuse, l'app se ferme (il faut la
  rouvrir). Une installation existante garde `reopen_after_game = false` :
  l'activer dans Advanced. Si `execv` est refusé, prévoir un retour au
  launcher dans le même processus (difficile : le jeu ne se décharge pas).
- **2026-11-10** — Test : **pointage au doigt OK, jeu très fluide**, mais
  seuls le stick, A et B agissaient. Cause : SDL numérote les entrées d'une
  manette virtuelle dans l'ordre des bits de `button_mask`/`axis_mask` ; avec
  seulement les entrées utilisées, les numéros après B et le stick étaient
  décalés. Corrigé : masques complets (comme les manettes virtuelles de
  `desktop_wpad.cpp`).
- **2026-11-10** — L'utilisateur a synchronisé `stable` avec le projet
  d'origine (**Recharged 1.1.6** : joueurs en ligne, rapports de
  désynchronisation, corrections Wiimote) et l'a fusionné dans `ios-port`
  (`6900d32`, sans conflit). Rien de propre à macOS dans ces changements ; le
  compteur de joueurs en ligne passe par HTTP (libcurl absent sur iOS : inactif).
  La fusion via GitHub n'a pas déclenché le workflow : build lancé à la main
  (run `42830613803`). Penser à `git pull` avant de reprendre le travail.
- **2026-11-10** — Build 1.1.6 (`42830613803`) : les autres boutons
  marchent, mais **+ et HOME ne réagissent pas**, et **le pointeur revient au
  centre après quelques secondes**. Causes : (1) + et HOME étaient collés au
  bord supérieur ; avec une fenêtre sans bordure, SDL fait différer les gestes
  système à tous les bords (`preferredScreenEdgesDeferringSystemGestures`), et
  iOS livre alors en retard les touchers qui y commencent, souvent appui et
  relâchement d'un coup, entre deux images ; (2) le jeu n'écoute la souris
  que 3 s après son dernier mouvement, puis prend le pointeur de la manette
  tactile (centré, faute de stick droit). Corrigé : + / HOME / croix
  descendus loin du bord ; un toucher vu seulement entre deux images appuie
  quand même son bouton 3 images (`taps_`) ; la dernière position pointée est
  renvoyée comme souris à chaque image (le pointeur reste où le doigt l'a
  laissé).
- **2026-11-10** — Build `42834279809` : **pointeur OK** (reste en place),
  mais **+ et HOME toujours sans effet**. La piste du bord d'écran était
  fausse (les anciens journaux montraient déjà « first press of + »). À la
  lecture, le chemin Start/Guide → `PadHeld` → Wiimote virtuelle → WPAD est le
  même que pour A/B : pas d'explication trouvée. Ajouté au journal : la
  correspondance SDL de la manette virtuelle, ce que SDL lit pour chaque
  bouton tenu (`... held: SDL joystick button N = x, gamepad button = y`),
  l'attribution des emplacements (`Gamepad slot N: ..., + is ..., HOME is ...`)
  et le premier appui de chaque bouton Wii vu par `desktop_wpad`
  (`Gamepad ...: first press of Wii button ...`). Corrigé au passage : les
  gâchettes virtuelles reposaient à 0 (mi-course, à un cran du seuil) au lieu
  du minimum de l'axe.
- **2026-11-10** — Journal du build `42837031429` : **+ et HOME arrivent
  jusqu'aux boutons Wii de `desktop_wpad`** (« first press of Wii button + /
  HOME », SDL lit bien les boutons 5 et 6 de la manette virtuelle, le jeu l'a
  ouverte, emplacement 4 → canal Wii 0, + = start, HOME = guide). La souris du
  joueur 1 (`SetNativeWpadMousePlayer`) n'ajoute que A/B, ne retire rien ;
  aucun patch du portage ne filtre + ou HOME. Ajouté : trace du dernier maillon
  (`wpad_sdl.cpp`, « WPAD channel N: first report with buttons 0x… ») pour
  savoir si le rapport WPAD lu par le jeu contient + (0x0010) et HOME (0x8000).
  Question ouverte : + met-il en pause sur ordinateur avec une manette ?
- **2026-11-10** — L'utilisateur ne sait plus quel bouton met le jeu en pause
  ni si HOME permet d'en sortir. Ajoutés aux contrôles tactiles : **−** (Back),
  **1** (X/gauche) et **2** (Y/haut), selon la disposition par défaut : tous
  les boutons de la Wiimote sont maintenant à l'écran.
- **2026-11-10** — Journal du build `42839657837` : **tous les boutons
  tactiles arrivent dans le rapport WPAD du canal 0** (A 0x0800, B 0x0400,
  + 0x0010, − 0x1000, HOME 0x8000, 1 0x0200, 2 0x0100, croix, C 0x4000,
  Z 0x2000). **La pause de ce jeu est le bouton 1** (vérifié par
  l'utilisateur), pas +. Le menu HOME ne s'ouvre pas bien que le jeu reçoive
  HOME : question de jeu/portage, pas d'entrée (non élucidé ; à essayer hors
  match). Pour sortir sans en dépendre : bouton tactile **« Quitter »** (en
  haut à gauche) ; maintenu 1 s (anneau de progression), il envoie
  `SDL_EVENT_WINDOW_CLOSE_REQUESTED` à la fenêtre du jeu, que
  `hardware_owner.cpp` transmet au jeu comme le bouton d'alimentation de la
  Wii (extinction normale, puis `ReopenLauncher` si `reopen_after_game`).
- **2026-11-10** — À la demande de l'utilisateur, « Quitter » à maintenir
  remplacé par une **petite croix discrète** (en haut à gauche, « × » pâle,
  sans disque) qui ouvre une **fenêtre de confirmation** « Quitter la partie ? »
  (Annuler / Quitter) sur l'écran assombri. Pendant qu'elle est ouverte, rien
  n'atteint le jeu (boutons relâchés, stick au repos) ; seuls ses deux boutons
  répondent, à un nouveau toucher. Quitter envoie la même demande de fermeture
  de fenêtre que précédemment.
- **2026-11-10** — Test de la croix : affichage, Annuler et Quitter
  fonctionnent ; le jeu s'éteint proprement (« Session ended: original power
  off, reopening the launcher »), mais **iOS ne laisse pas l'app se relancer**
  (`execv` de `ReopenLauncher`) : l'app se ferme. Choix de l'utilisateur
  (« voie 1 ») : assumer que **Quitter ferme l'app**. La fenêtre dit « Quitter
  le jeu ? / L'application va se fermer. » ; `reopen_after_game` est masqué
  sur iOS, jamais armé (même si l'INI l'active) et de nouveau `false` par
  défaut. Revenir au launcher sans fermer l'app (« voie 2 ») demanderait de
  remettre le jeu à zéro dans le même processus : reporté à l'étape 4.
- **2026-11-10** — Vérifié : sur le dépôt d'origine, `main` = développement
  et cible des pull requests, `stable` = versions publiées. `main` a déjà
  5 commits après la 1.1.6 (Wiimotes inactives réveillées chaque seconde #161,
  dossiers de désynchronisation #160, correctif du rollback, libellés des
  profils de manette, journal de batterie des Wiimotes). Fichiers modifiés à
  la fois par `main` et `ios-port` : `src/launcher/main.cpp`,
  `src/runtime/original_game.cpp` (petits conflits possibles à la prochaine
  fusion). Recommandation : rester sur `stable`.
- **2026-11-10** — Branche **`main` copiée** dans le fork (`zozo787/recharged`,
  au commit `12fdc4d` du projet d'origine), pour une éventuelle pull request
  plus tard. Elle ne déclenche aucun workflow (`build.yml` : `stable` ;
  `ios.yml` : `ios-port`). La garder à jour avec « Sync fork » sur GitHub.
- **2026-11-10** — Vérification des autres plateformes (`build.yml` lancé à
  la main sur `ios-port`, run `42854445316`) : **Linux, Steam Deck, Windows et
  les tests de services Windows passent**, iOS aussi. Les jobs macOS n'ont
  jamais démarré, deux fois (« not acquired… capacity constraints » sur les
  runners Apple Silicon). Ajouté : `.github/workflows/macos-check.yml`, manuel,
  qui compile le programme macOS et un module de jeu sur `macos-15-intel`
  (Xcode 26.3, celui des releases) ; même code macOS qu'Apple Silicon.
- **2026-11-10** — **Contrôle macOS réussi** (run `42864448657`,
  `macos-15-intel`, Xcode 26.3, 1 h à froid) : le programme macOS, son
  `Recharged.app` et le module EU compilent avec les changements du portage ;
  `recharged --version` répond. Avec Linux, Steam Deck et Windows (run
  `42854445316`), **rien ne casse la 1.1.6 à la compilation**. Restent à
  lancer pour une pull request : `ctest --preset debug`.
- **2026-11-10** — L'utilisateur abandonne, faute de priorité, la pause
  « Dépliez pour continuer » au pliage et la pause en arrière-plan (prévues à
  l'étape 3 ; exigence de départ retirée).
- **2026-11-10** — **Finitions du launcher sur iOS** : masqués la carte
  « Screen » (mode fenêtré/plein écran, résolution de la fenêtre, écran), la
  carte « Updates », la carte « Discord », le réglage « Log window » et la
  carte « Wii Remote » (Wiimotes inaccessibles aux apps iOS) ; en-tête de la
  page Controls adapté (écran tactile ou manette Bluetooth) ; textes « NATIVE
  PC PORT » → « NATIVE PORT », « … natively on your PC » → « … natively on
  iPhone and iPad » (page Play, page About). La carte graphique était déjà
  masquée sur tous les Apple. Zones sûres reportées (voir « Reprendre ici »).
- **2026-11-10** — L'utilisateur précise avoir essayé l'iPad : **presque aucun
  problème**. Issue de proposition rédigée pour le projet d'origine (« iOS and
  iPadOS port », en anglais, port présenté comme en cours et à améliorer).
- **2026-11-10** — Issue publiée par l'utilisateur :
  [previouslevelgames/recharged#163](https://github.com/previouslevelgames/recharged/issues/163).
- **2026-11-10** — **Bouton HOME retiré** des contrôles tactiles. Raison :
  dans le dépôt d'origine aussi, le menu HOME n'est pas compilé dans les
  builds normaux. Son ouverture (`FrontEndTask::Run`, patch
  `0593-original-home-task-admission.patch`) dépend de l'option
  `MSCHARGED_DIAGNOSTIC_FRONTEND_HOME`, désactivée par défaut et activée par
  aucun preset ni workflow (y compris `main` d'origine). HOME (touche Home,
  Guide/PS) arrive donc au jeu et n'y fait rien, sur toutes les plateformes.
  − et + sont recentrés en haut, comme sur une Wiimote. À remettre si les
  mainteneurs activent le menu HOME (question possible sur l'issue #163).
- **2026-11-10** — Retour de l'utilisateur sur `5ad1f21` : placement de − et +
  correct, **confort des contrôles pas encore satisfaisant** (à reprendre).
  Le bouton « Objet » était mal nommé : **C utilise l'objet**, secouer le
  Nunchuk (RB) **change l'ordre des objets** (« Swap Items » dans
  `bootstrap/gamepad_actions.h`). Renommé **« Changer »** (sans accent : la
  police de l'overlay n'a que l'ASCII et ×).
- **2026-11-10** — **Éditeur de disposition des contrôles tactiles**, demandé
  par l'utilisateur : réglé **avant** la partie, dans le launcher
  (Controls → Players, bouton réglages à côté de « Touch screen »), avec
  **Reset** pour revenir à la disposition par défaut. Plein écran : chaque
  contrôle se déplace au doigt ; toucher un contrôle le sélectionne pour sa
  taille (− / +, 50 à 200 %) ; Cancel / Save. Enregistré dans
  `controls.touch_layout` (iOS seulement ; vide = défaut).
  Code partagé launcher/jeu : `src/bootstrap/touch_layout.{h,cpp}` (dans
  `charged_host`, C++17 pur) : chaque contrôle est ancré au bord le plus
  proche (ou à la ligne du centre), décalage en millièmes du petit côté de
  l'écran, pour garder la même distance au pouce sur iPhone plié, déplié ou
  iPad. La croix directionnelle et le stick se déplacent d'un bloc ; la zone
  où un doigt prend le stick suit le stick (et se retourne s'il passe à
  droite ou en haut). Disposition par défaut identique au pixel près à
  l'ancienne (vérifié par un test local). Une valeur abîmée est ignorée, sans
  erreur. Le journal de session note « default layout » ou « custom layout ».
- **2026-11-10** — À la demande de l'utilisateur, **rien du tactile n'est
  compilé hors iOS** : `src/bootstrap/touch_layout.cpp` (dans `charged_host`)
  et `src/runtime/touch_controls.cpp` (dans `charged_original_game_host`)
  sont ajoutés par `if(MSCHARGED_IOS)` ; leurs `#include` et le dessin des
  contrôles dans `host_overlay.cpp` sont sous `MSCHARGED_PLATFORM_IOS`.
- **2026-11-10** — Build `02502c9` (run 42907002019) : **éditeur de
  disposition validé** sur l'iPhone.
  À confirmer : la disposition enregistrée reprise en jeu, Reset.
- **2026-11-10** — **Vérification macOS réussie** après le tactile limité à
  iOS (run 42909894623, `61ac825`, 18 min avec le cache). Rappel : le bouton
  « Run workflow » n'existe que pour les workflows de la branche par défaut
  (`stable`) ; `macos-check.yml` se lance en modifiant ce fichier sur
  `ios-port`, et `ios.yml` ignore désormais ce fichier.
- **2026-11-10** — L'utilisateur confirme : disposition enregistrée reprise
  en jeu, Reset OK. Ajouts :
  - **Opacité des contrôles** (`controls.touch_opacity`, 20-100 %, iOS
    seulement), curseur « Opacity » dans l'éditeur, aperçu en direct ; Reset
    la remet à 100 %. Le jeu multiplie l'alpha de tous les contrôles (pas la
    fenêtre « Quitter le jeu ? »). Journal : « default/custom layout, N %
    opacity ».
  - **Zones sûres du launcher** : `UpdateSafeInsets` lit
    `SDL_GetWindowSafeArea` à chaque image (iOS ; 0 ailleurs, mise en page
    inchangée). La barre latérale garde sa couleur sous la marge gauche mais
    décale son contenu ; la page et la barre du bas s'arrêtent avant les
    marges droite et basse (la couleur de la barre va jusqu'au bord) ;
    l'illustration de la page Play va jusqu'au bord droit. `LogScreen` écrit
    la zone sûre dans le journal (« safe area x,y wxh »), pour connaître les
    marges réelles de chaque appareil.
  - Simulateur Xcode : reporté (bien plus tard).
- **2026-11-10** — Build `a829838` (run 42913930522) validé par
  l'utilisateur : opacité (éditeur et jeu), Reset, zones sûres du launcher.
  Marges réelles des appareils : à relever dans `Documents/logs/recharged.log`
  (lignes « Screen (…) … safe area »).
- **2026-11-10** — **Marges réelles de l'iPhone Duo** (iPhone19,4,
  iOS 27.2.1, journal du build `a829838`), en paysage :
  - plié (écran extérieur) : fenêtre 678x466 points, zone sûre 678x466 :
    **aucune marge** ;
  - déplié : fenêtre 951x669 points (2853x2007 pixels), zone sûre 0,0
    951x635 : **34 points en bas** (barre d'accueil), rien à gauche, à droite
    ni en haut.
  Le launcher remonte donc sa barre du bas de 34 points déplié. iPad : à
  relever.
- **2026-11-10** — « Added controller 'Nintendo Wii Remote' (vid 057e,
  pid 0306) » dans le journal sans rien de connecté : **normal**. C'est la
  Wiimote virtuelle que `platform/desktop_wpad.cpp` (`MakeDevice`) crée
  pour chaque manette qui joue, ici la manette tactile, afin que le jeu
  reçoive des rapports Wii. Elle est virtuelle (`SDL_IsJoystickVirtual`),
  donc `PhysicalGamepad` ne masque pas les contrôles tactiles pour elle.
- **2026-11-11** — Signalé par l'utilisateur : « Change... » (dossier du jeu)
  n'ouvre rien, erreur « This operation is not supported ». Cause : SDL n'a
  **aucune boîte de dialogue de fichiers sur iOS** (`src/dialog/dummy`) ; tous
  les choix de fichier ou de dossier du launcher étaient touchés. Et une app
  iOS ne garde pas l'accès à un fichier hors de son conteneur sans signets
  (security-scoped bookmarks), jugés trop fragiles (fichier déplacé, iCloud
  « optimiser le stockage », clé débranchée, réinstallations Sideloadly, et
  `nod`/le jeu lisent par chemin). **Option retenue par l'utilisateur :
  import par copie.**
  - `src/launcher/ios_document_picker.{h,mm}` (Objective-C++, ARC, iOS
    seulement ; UIKit et UniformTypeIdentifiers liés) :
    `UIDocumentPickerViewController` « as copy » (tout fichier, `UTTypeData`) ;
    iOS copie le fichier (et le télécharge d'iCloud si besoin) dans le dossier
    temporaire de l'app ; il est **déplacé** dans le dossier du jeu
    (`Documents/game` par défaut). Même nom et même taille déjà présents : le
    fichier existant est gardé ; sinon nom numéroté. Rappel au format des
    callbacks de SDL (`DiscChosen`).
  - « Choose file... » (image Wii) et le disque GameCube l'utilisent ; après un
    import, la bibliothèque est relue.
  - Dossiers (jeu, packs de textures, sauvegardes) : « Open in Files » /
    « Show » ouvre le dossier dans l'app Fichiers (`shareddocuments://`, que
    `FileUrl` donne désormais sur iOS ; les boutons « Open folder » marchent
    donc aussi), avec un message. Import Dolphin (sauvegarde et Miis) masqué
    sur iOS.
  - Limite : une image importée qui n'est pas Mario Strikers Charged reste
    dans le dossier (à supprimer dans Fichiers).
- **2026-11-11** — Build `071a389` : « Choose file... » montre un **écran
  blanc qui finit par se fermer** (le sélecteur d'iOS ne charge pas son
  contenu ; iOS ne renvoie aucune erreur à l'app). Changements :
  - le sélecteur s'ouvre **en place** (`asCopy:NO`) au lieu de « en copie » :
    l'app reçoit un accès temporaire (`startAccessingSecurityScopedResource`)
    et copie elle-même le fichier **en arrière-plan** (`NSFileCoordinator`,
    qui télécharge d'iCloud si besoin), puis répond sur le fil principal ;
  - fermeture par glissement ou par iOS traitée comme Annuler
    (`presentationControllerDidDismiss`) ;
  - **traces** dans le journal (« File picker: showing it over …, shown,
    picked …, copying …, copied to … / cancelled / closed ») pour savoir où
    ça s'arrête ;
  - le bouton affiche « Importing... » pendant le choix et la copie.
  Si l'écran blanc reste : regarder la console de l'iPhone sur le Mac (app
  Console, appareil branché, filtre « recharged » ou « DocumentManager »).
- **2026-11-11** — Journal du build `ca3a04e` : « File picker: showing it
  over SDL_uikitviewcontroller (window found) », « shown », puis rien : le
  sélecteur est bien présenté mais son contenu (servi par un autre processus
  d'iOS) ne se charge pas. Hypothèse : la boucle du launcher ne rend la main
  à la boucle d'iOS qu'un instant entre deux images (`UIKit_PumpEvents`).
  Changement : comme SDL pour ses boîtes de message
  (`SDL_uikitmessagebox.m`), `ShowImportFileDialog` fait tourner la boucle
  d'iOS (`NSRunLoop runMode:` par 0,1 s) **jusqu'à la fermeture du
  sélecteur** (choix, Annuler, glissement, ou fermeture par iOS, trace
  « closed by iOS ») ; la copie se fait ensuite en arrière-plan.
- **2026-11-11** — Build `4cc373f` : **le sélecteur n'est plus blanc**
  (la boucle d'iOS était bien en cause), on peut choisir l'image, mais rien
  ne se passe. Journal : « closed by iOS » puis « cancelled » **avant**
  « shown » : ma vérification de fermeture (`presentingViewController` nul)
  se déclenchait pendant la présentation, le délégué était libéré et le
  choix n'arrivait plus. Corrigé : la vérification n'agit qu'après la fin de
  la présentation (`presented`, mis dans le bloc de fin), ou si le
  sélecteur n'est jamais apparu au bout de 10 s (« never shown »).
- **2026-11-11** — Build `6e64657` (run 43029438837) : **import d'image
  validé par l'utilisateur** (Choose file... → sélecteur d'iOS → copie dans
  `Documents/game` → image sélectionnée). Restent à essayer : « Open in
  Files » / « Show » / « Open folder », et le disque GameCube.
- **2026-11-11** — Retour de l'utilisateur : Advanced → « Open folder » du
  fichier de réglages ouvre Fichiers mais **reste dans le dossier ouvert
  avant**. Revue des boutons de dossier : tous pointent dans `Documents`
  (jeu `Documents/game`, données et sauvegardes `Documents/data`, packs
  `Documents/texture_packs`, extractions `data/texture_dumps`, journaux
  `Documents/logs`, réglages `Documents`). Changement de forme du lien :
  `FileUrl` donne sur iOS le chemin normalisé en `/private/var/…` **terminé
  par « / »** pour un dossier, comme l'URL que donne iOS pour Documents ;
  chaque lien ouvert est écrit dans le journal (« Open in Files: … »).
  Disque GameCube : seulement pour Gameplay → **Super Team** (« Needs a Super
  Mario Strikers (GameCube) disc image »), bouton sous l'option une fois
  activée.
- **2026-11-11** — Issue #163 : questions du mainteneur (CI, adaptations,
  dépôt séparé) et réponse publiée : voir l'issue.
- **2026-11-11** — **Historique de `ios-port` réécrit** (poussé de force, à la
  demande du propriétaire du fork) : ce document est retiré de tous les
  anciens commits, puis réajouté dans sa version actuelle, sans détails
  personnels. Le code de chaque commit est inchangé ; « foldable iPhone »
  devient « iPhone Duo » dans les messages. Les références de commit de ce
  document pointent vers les nouveaux identifiants ; les noms d'`.ipa` et les
  numéros de runs gardent ceux d'avant la réécriture.
