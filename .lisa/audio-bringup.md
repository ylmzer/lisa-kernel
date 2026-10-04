# Lisa audio bring-up

The user reports that calls establish on the `4083eb08` candidate but have
no sound. Read-only diagnostics show no ALSA sound cards and no PCM devices.
The machine driver remains deferred at PRI_MI2S_RX. The ADSP and WCD938x
codec initialize; this is not sufficient to register a usable sound card.

Two integration gaps are present:

- The build omitted TARGET_PRODUCT=lisa. The vendor Kbuild consequently
  skipped the existing TFA98xx amplifier directory and Lisa TDM machine
  definitions. The live device tree has two enabled `tfa,tfa98xx` devices
  at I2C addresses 0x34 and 0x35. The installed Image has no TFA probe symbol.
- The newer MI2S driver requires `qcom,msm-mi2s-lines` and separate direction
  device IDs. Lisa's live device tree has combined RX/TX interface IDs and
  separate `qcom,msm-mi2s-rx-lines` / `qcom,msm-mi2s-tx-lines` properties.
  MI2S platform devices exist but have no bound driver/components.

The Lisa build now supplies the real target product. Its combined MI2S path
uses the original GPL-2.0-only implementation from MiCode lisa-r-oss commit
`57ee2b068000ac79caa7c076d0d9bb1a9260e8a4`. The code is isolated in
`msm-dai-q6-mi2s-lisa.h`, selected only by CONFIG_TARGET_PRODUCT_LISA. Other
targets retain the newer separate-direction implementation. RX/TX line
masks, stream configuration and errors use actual device-tree values.

Vendor preflight and final Image verification require the TFA probe provider.
The existing device tree, ROM/vendor files and firmware remain untouched.
Full CI, sound-card registration, media playback, call audio and microphone
tests are still required before claiming audio works. Private device records
are not committed.

## First audio candidate observation

The user installed `8d9f78e6`. The boot banner matches its verified Image.
Both TFA9873 amplifiers probe successfully and all six combined MI2S
components register. The sound card now gets past the earlier missing
PRI_MI2S_RX component but remains deferred because the newer machine header
still refers to CPU DAI `msm-dai-q6-mi2s.6`, absent from the live DT's 0..5
interface namespace. No ALSA sound card is present yet.

The machine header now selects combined CPU names for Lisa's RX/TX pairs
while retaining the existing separate-direction names for other targets.
A preprocessor test reads the actual header and checks both contracts:
six shared full-duplex interfaces for Lisa and twelve unique direction DAIs
for the native path. A new combined audio/haptics build and device test are
required; amplifier detection alone does not establish working audio.

## Combined candidate verification

CI [37226269848](https://github.com/ylmzer/lisa-kernel/actions/runs/37226269848)
passed for source `dd06b1854478769c01ebf18656f89df6891e62d0`, including the
actual MI2S header test, vendor preflight, full Image and installer packaging.
The downloaded artifact digest and manifest checksums match. The installer
contains the verified Image and the SD copy matches the ZIP SHA-256:
`bcdbcd8d994d5294ff00bf25635ecfa7a20531466046e9f828aae47c9fdac1ec`.

After user installation, the boot banner matches the Image. Android boots,
`lahaina-yupikqrd-snd-card` registers and playback/capture PCM devices exist.
Both TFA devices complete DSP startup. The user then confirms media sound
from the speaker and two-way call audio, including microphone operation.
