#pragma once

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_video.h>

#include <string>

// iOS has no SDL file dialog (SDL_ShowOpenFileDialog answers "That operation
// is not supported"), and an app keeps no lasting access to a file elsewhere
// without security-scoped bookmarks. This shows iOS's own file picker
// instead and copies the chosen file into the app's own folders, where the
// launcher and the game can always read it (docs/IOS_PORT.md).
namespace mscharged::launcher
{
// Opens the picker over `window` for any file. The chosen file is copied,
// in the background (downloaded from iCloud first if needed), into
// `destination` (made when missing) under its own name; a file of that name
// and size already there is taken as the same one and kept, another gets a
// numbered name. `callback` then receives the copy's path the way an
// SDL_ShowOpenFileDialog callback does: a list with the path, an empty list
// when the picker is cancelled, or NULL with SDL_GetError() set on a failure.
// Call it on the main thread; the callback runs on it too. Each step goes to
// the session log ("File picker: ...").
void ShowImportFileDialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window,
                          const std::string& destination);
} // namespace mscharged::launcher
