# SoftFolders

**English** | [繁體中文（臺灣）](README.zh-TW.md)

A lightweight native Windows desktop extension for grouping application shortcuts into phone-style folders.

![Generated sample folder preview](docs/preview.png)

## Features

- Native desktop items that participate in Explorer's normal icon arrangement.
- A fixed **3 × 3 preview**, filled from the upper-left corner, left to right and top to bottom.
- Consistent icon sizes, original transparency, and balanced preview spacing.
- Single-click opening while the desktop extension is active.
- A transparent **10 × 6** expanded layout, without hover rectangles or an always-on-top panel.
- Show Desktop before expanding; restore the hidden windows when the folder closes.
- Double-click to launch, drag to reorder, and Escape or an empty-area click to close.
- Dragging a shortcut into a folder adds it without opening the folder or moving the original.
- System fonts and configurable text colors.

The extension is C++/Win32/COM. It does not require .NET, Electron, or a continuously running settings application. It still uses memory inside Explorer; opening a folder allocates additional drawing resources.

## Install

Supports Windows 10/11 x64.

1. Download **SoftFolders-Windows-x64.zip** from [Releases](https://github.com/Enchore/SoftFolders/releases/latest).
2. Extract it to a folder you intend to keep.
3. Run `SoftFoldersSetup.exe` and choose **安裝／更新擴充元件**.
4. Create a folder using the settings tool or **Desktop → New → 軟體夾**. Some Windows 11 context-menu entries appear under **Show more options**.
5. Drag application shortcuts into it.

Keep `SoftFoldersSetup.exe` and `SoftFoldersShell.dll` together. Registration is per user. At sign-in, the setup utility runs briefly to attach the desktop handler and then exits.

### Daily use

Single-click the desktop folder to expand it. Double-click an application to launch it. Right-click for adding/removing references, page navigation, and text-color options. The expanded view has 60 positions per page.

Folders are small Unicode `.softfolder` files containing paths and positions. Original applications and shortcuts remain in place. Nested SoftFolders are not supported.

### Uninstall and update

Run the settings utility and choose **解除安裝擴充元件**. Folder data and original applications are preserved. A loaded DLL may remain mapped into Explorer until sign-out or a normal Explorer restart. Uninstall and sign out before replacing or removing an in-use DLL; the tool does not forcibly restart Explorer.

## Build

Requirements: Windows x64, Visual Studio 2022 Build Tools (or Visual Studio) with **Desktop development with C++**, and a Windows SDK.

```powershell
.\build.cmd
```

The script locates Visual Studio with `vswhere` and writes the binaries to `build/bin`.

## Validate

Run the isolated icon-pipeline checks:

```powershell
.\tests\test.cmd
```

These checks generate synthetic icon fixtures with different transparent margins. They verify consistent visible sizes, transparency, and folder-preview rendering without installing the extension or reading personal desktop folders.

Additional interactive diagnostics are available on a normal Windows desktop:

```powershell
.\build\bin\SoftFoldersSetup.exe --verify
.\build\bin\SoftFoldersSetup.exe --status
```

`--verify-desktop` temporarily minimizes and restores windows to test the Show Desktop cycle. It requires a normal interactive desktop and should be run deliberately, after `--verify` has generated its fixture.

## Limitations

This is an experimental third-party Explorer extension, not a built-in Windows feature. Defects in an in-process extension can affect Explorer stability. Multi-monitor configurations with mixed DPI have not been exhaustively tested. The UI uses Traditional Chinese (Taiwan), and the binaries are unsigned.

## Source layout

| Path | Purpose |
| --- | --- |
| `src/shell.cpp` | COM handlers, registration, and lifecycle |
| `src/click.h` | Native desktop click routing |
| `src/view.h` | Transparent expanded view and interaction |
| `src/normalized_icons.h` | Original icon loading, alpha bounds, and normalization |
| `src/common.h` | Folder data and preview rendering |
| `src/setup.cpp` | One-time settings and command-line utility |
| `tests/test.cmd` | Isolated icon-pipeline checks |

Local folder data, diagnostic dumps, and third-party application icons are not included in this repository.

## License

[MIT](LICENSE) © 2026 Enchore.
