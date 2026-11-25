# EnvFollowerFX

EnvFollowerFX is a stereo insert effect for Akai MPC standalone. It uses the
left input as an envelope detector to control the level of the right input,
then sends the processed carrier to both outputs (dual mono).

| Input | Purpose |
| --- | --- |
| Left | Detector / modulation source |
| Right | Carrier audio to process |
| Outputs 1 and 2 | Same processed carrier signal |

When Bypass is enabled, the original left and right inputs pass through to
their matching outputs unchanged.

## Controls

| Control | Range / behavior | Default |
| --- | --- | ---: |
| Attack | Envelope attack, 5–250 ms, logarithmic | 5 ms |
| Release | Envelope release, 30–1500 ms, logarithmic | 53.1 ms |
| Level | Detector sensitivity, 0.1x–2.5x, shown in dB | 8 dB |
| Amount | Envelope modulation depth, 0–100% | 100% |
| Offset | Base gain, 0–100% | 0% |
| Mix | Dry/wet balance, 0–100% | 100% |
| Smoothing | Gain smoothing, 0–20 ms | 0% |
| Bypass | Pass stereo inputs through unchanged | Off |

The envelope is derived from the absolute detector level with a square-root
response. The modulation gain is limited to 0–100%; this effect does not boost
the carrier above unity gain.

## Build from source

This repository contains the EnvFollowerFX project source and assets. Building
requires the MPC Plugin Builder SDK; the SDK framework is not copied into this
repository.

1. Clone the SDK repository.
2. Clone this repository into the SDK's `plugins/EnvFollowerFX/` directory.
3. From the SDK root, run:

   ```sh
   python tools/build.py plugins/EnvFollowerFX
   python tools/package.py plugins/EnvFollowerFX
   ```

The build produces native, ARM32, and ARM64 targets and validates the MPC
plugin folder. The package command writes an installable ZIP to
`plugins/EnvFollowerFX/dist/`.

The native host simulator can be used to test the plugin after building:

```sh
.sdk-tools/host_sim plugins/EnvFollowerFX/build/native/envfollowerfx.dll test --wav plugins/EnvFollowerFX/build/test.wav
```

On platforms where the native library has a different extension, use the
native library path printed by the build.

## Install on an MPC

The MPC must run MPC OS 3.10.1 or later. Copy the `PETZOO - VST - EnvFollowerFX`
folder from the ZIP into a drive's `VST2 Plugins` folder. On the MPC, enable
both **Enable Enhancements** and **External Plugin Support** under
**Preferences → Enhancements (Beta)**. The effect appears under External
Plugins and can be added as an insert effect.

## Development notes

- The plugin UID is `PZvE`. Do not change the UID or parameter IDs/order in a
  released project; existing MPC projects may depend on them.
- `skins/` contains the custom MPC touchscreen layout and its image assets.
- Build outputs and release ZIPs are generated locally and are excluded from
  source control.
- The latest checked build completed with no validator warnings, and the host
  simulator passed its 20 checks.

## License

EnvFollowerFX source and included project assets are licensed under the MIT
License; see [LICENSE](./LICENSE).

The MPC Plugin Builder SDK is a separate project and has its own license.
“MPC” and “Akai Professional” are trademarks of inMusic Brands, Inc.; “VST” is
a trademark of Steinberg Media Technologies GmbH. This project is not
affiliated with or endorsed by those companies.
