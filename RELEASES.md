# Public Releases

This project ships with two public downloads:

## 1.0.0

Main public release. This is the default package people should install.

Use this when you want the updated launcher to open from Steam.

## Original

Restore package for the stock files.

Use this when you want to undo the launcher replacement and put the original Steam entry back.

## Install target

Both releases install into the game root and the modtools launcher folder:

- `Call of Duty Black Ops III\`
- `Call of Duty Black Ops III\modtools\`

## 1.0.0 contents

The public release should include only the launcher entry and its runtime:

- `modtools_launcher.bat`
- `modtools\modtools_launcher.bat`
- `modtools\ModLauncherCustomRuntime\ModLauncher_custom.exe`
- `modtools\ModLauncherCustomRuntime\D3Dcompiler_47.dll`
- `modtools\ModLauncherCustomRuntime\Qt6Core.dll`
- `modtools\ModLauncherCustomRuntime\Qt6Gui.dll`
- `modtools\ModLauncherCustomRuntime\Qt6Network.dll`
- `modtools\ModLauncherCustomRuntime\Qt6Svg.dll`
- `modtools\ModLauncherCustomRuntime\Qt6Widgets.dll`
- `modtools\ModLauncherCustomRuntime\steam_api64.dll`
- `modtools\ModLauncherCustomRuntime\steam_appid.txt`
- `modtools\ModLauncherCustomRuntime\generic\*`
- `modtools\ModLauncherCustomRuntime\iconengines\*`
- `modtools\ModLauncherCustomRuntime\imageformats\*`
- `modtools\ModLauncherCustomRuntime\networkinformation\*`
- `modtools\ModLauncherCustomRuntime\platforms\*`
- `modtools\ModLauncherCustomRuntime\styles\*`
- `modtools\ModLauncherCustomRuntime\tls\*`

After extraction, the installed layout should be:

- `Call of Duty Black Ops III\modtools_launcher.bat`
- `Call of Duty Black Ops III\modtools\modtools_launcher.bat`
- `Call of Duty Black Ops III\modtools\ModLauncherCustomRuntime\*`

## Original contents

The restore package should include the stock launcher entry only:

- `modtools_launcher.bat`

## Upload checklist

1. Publish the source to GitHub.
2. Create a GitHub release tagged `v1.0.0`.
3. Upload the `1.0.0` package as the main asset.
4. Upload the `Original` package as the restore asset.
5. Point the release notes at `README.md` for setup instructions.

## Recommended release text

The release description should say:

- `1.0.0` is the default public release.
- `Original` restores the stock launcher entry.
- Extract the package into `Call of Duty Black Ops III\modtools\` and overwrite the listed launcher files only.